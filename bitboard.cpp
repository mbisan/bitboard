#include "display.cpp"

#include <chrono>

SliderLookup bishopLookup("./bishopLookup", bishoppositions);
SliderLookup rookLookup("./rookLookup", rookpositions);

Squares sliderBishop(Squares reachableIfNotBlocked, Squares occupied, uint64_t pieceIndex) {
    return bishopLookup.table[pieceIndex][_pext_u64(occupied, reachableIfNotBlocked)];
}

Squares sliderRook(Squares reachableIfNotBlocked, Squares occupied, uint64_t pieceIndex) {
    return rookLookup.table[pieceIndex][_pext_u64(occupied, reachableIfNotBlocked)];
}

struct isInCheckResult {
    int checkCount;
    Squares checkMask;
    Squares enemySeen;
    Squares kingNotReachable;
    Squares pinmaskHV;
    Squares pinmaskD;
    Squares enpassantpin;
    Squares enemySquares;
    Squares selfSquares;
    Squares occupied;
};

template<bool isWhite>
isInCheckResult isInCheck(const Pieces &selfPieces, const Pieces &enemyPieces, uint8_t enPassant) {

    Squares enemySquares = enemyPieces.occupied();
    Squares selfSquares = selfPieces.occupied();
    Squares occupiedSquares = enemySquares | selfSquares;

    Squares occupiedNotKing = occupiedSquares ^ selfPieces.king;

    uint64_t kingIndex = bitFromSquare(selfPieces.king);

    int checkcount = 0;

    Squares checkMask = 0;
    Squares enemySeen = 0;
    Squares kingNotReachable = 0;
    Squares pinmaskHV = 0;
    Squares pinmaskD = 0;
    Squares enpassantpin = 0;

    BitLoop(enemyPieces.rooks | enemyPieces.queens) {
        uint64_t pieceIndex = bitFromSquare(temp);
        enemySeen |= sliderRook(rookMoves[pieceIndex], occupiedSquares, pieceIndex);

        if (rookMoves[pieceIndex] & selfPieces.king) { // i.e. the enemy rook/queen can view the king (if the file/column is empty)

            //    path from king to enemy (inc)       exclude enemy
            Squares inbetween = (PinBetween[kingIndex][pieceIndex] ^ positionToBit[pieceIndex]) & occupiedNotKing;

            if (!inbetween) { // i.e. the enemy rook is not blocked by any piece
                checkMask |= PinBetween[kingIndex][pieceIndex];
                kingNotReachable |= CheckBetween[kingIndex][pieceIndex];
                checkcount++;
            } else { // there are pieces in the way
                Squares onePieceRemoved = _blsr_u64(inbetween);
                if (!onePieceRemoved) { // i.e. the enemy rook/queen is blocked by a single piece, then this piece is pinned
                    pinmaskHV |= PinBetween[kingIndex][pieceIndex];
                } else if (!_blsr_u64(onePieceRemoved)) { // there are two pieces in the way, therefore the only way for a piece to be pinned is that the piece is an enpassant pawn
                    if (enPassant && (pieceIndex/8 == kingIndex/8) && inbetween & selfPieces.pawns) { 
                        // there is enpassant pawn for enemy on the same file as the king AND the other piece is a self pawn
                        enpassantpin = inbetween & selfPieces.pawns; // i.e. mask the self pawn so that it cannot take enpassant
                    }
                }
            }
        }
    }

    BitLoop(enemyPieces.bishops | enemyPieces.queens) {
        uint64_t pieceIndex = bitFromSquare(temp);
        enemySeen |= sliderBishop(bishopMoves[pieceIndex], occupiedSquares, pieceIndex);

        if (bishopMoves[pieceIndex] & selfPieces.king) { // i.e. the enemy bishop/queen can view the king (if the diag is empty)

            Squares inbetween = (PinBetween[kingIndex][pieceIndex] ^ positionToBit[pieceIndex]) & occupiedNotKing;

            if (!inbetween) { // i.e. the enemy bishop is not blocked by any of our pieces
                checkMask |= PinBetween[kingIndex][pieceIndex];
                kingNotReachable |= CheckBetween[kingIndex][pieceIndex];
                checkcount++;
            } else {
                if (!_blsr_u64(inbetween)) { // there is only ONE piece in between
                    pinmaskD |= PinBetween[kingIndex][pieceIndex];
                }
            }
        }
    }

    BitLoop(enemyPieces.knights) {
        uint64_t pieceIndex = bitFromSquare(temp);
        enemySeen |= knightMoves[pieceIndex];

        if (knightMoves[pieceIndex] & selfPieces.king) { // i.e. the enemy knight can reach our king
            checkMask |= knightMoves[kingIndex] & enemyPieces.knights;
            checkcount++;
        }
    }

    BitLoop(enemyPieces.pawns) {
        uint64_t pieceIndex = bitFromSquare(temp);
        enemySeen |= pawnAttacks<!isWhite>()[pieceIndex];
        
        if (pawnAttacks<!isWhite>()[pieceIndex] & selfPieces.king) { // i.e. pawn attacking our king
            checkMask |= positionToBit[pieceIndex];
            checkcount++;
        }
    }

    enemySeen |= kingMoves[bitFromSquare(enemyPieces.king)];

    return {checkcount, checkMask, enemySeen, kingNotReachable | enemySeen, pinmaskHV, pinmaskD, enpassantpin, enemySquares, selfSquares, occupiedSquares};
}

template<bool isWhite>
std::vector<Board> generateMoves(const Board &board) {
    std::vector<Board> newMoves;

    Pieces self, enemy;
    if constexpr (isWhite) {
        self = board.wP;
        enemy = board.bP;
    } else {
        self = board.bP;
        enemy = board.wP;
    }

    uint8_t enPassant = board.enPassant;

    // generate pinmask and checkmask
    isInCheckResult r = isInCheck<isWhite>(self, enemy, enPassant); 
    Squares pinmask = r.pinmaskD | r.pinmaskHV;
    Squares notSelf = ~r.selfSquares;
    uint64_t kingIndex = bitFromSquare(self.king);

    Squares esq = r.enemySquares;
    Squares nesq = ~esq;

    if (r.checkCount > 1) { // double check forces king to move
        // king can only move to squares not seen by enemy and not occupied by enemy pieces
        Squares kingReachable = kingMoves[kingIndex] & notSelf & ~r.enemySeen & ~r.kingNotReachable;

        BitLoop(kingReachable & esq) {            
            newMoves.push_back(board.kingMovesCapture<isWhite>(self.king, _blsi_u64(temp)));
        }

        BitLoop(kingReachable & nesq) {            
            newMoves.push_back(board.kingMoves<isWhite>(self.king, _blsi_u64(temp)));
        }

        if (newMoves.size() == 0) newMoves.push_back({Pieces(), Pieces(), 0, !isWhite, 0b10000000}); // checkmate
        return newMoves;
    } 
    if (r.checkCount == 0) r.checkMask = 0xffffffffffffffff; // if no checks are given, then all squares are available for all pieces

    Pieces selfPiecesNew;  

    // generate king moves
    Squares kingReachable = kingMoves[kingIndex] & notSelf & ~r.enemySeen & ~r.kingNotReachable;
    BitLoop(kingReachable & esq) {            
        newMoves.push_back(board.kingMovesCapture<isWhite>(self.king, _blsi_u64(temp)));
    }
    BitLoop(kingReachable & nesq) {            
        newMoves.push_back(board.kingMoves<isWhite>(self.king, _blsi_u64(temp)));
    }

    // generate rook/queen moves
    Squares notSelf_n_checkMask = notSelf & r.checkMask;
    
    // non-pinned rooks
    BitLoop(self.rooks & ~pinmask) {
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = sliderRook(rookMoves[pieceIndex], r.occupied, pieceIndex) & notSelf_n_checkMask;
        
        BitLoop2(reachable & esq) {
            newMoves.push_back(board.pieceMove<isWhite, 3>(_blsi_u64(temp), _blsi_u64(temp2)));
        }
        BitLoop2(reachable & nesq) {
            newMoves.push_back(board.pieceMoveCapture<isWhite, 3>(_blsi_u64(temp), _blsi_u64(temp2)));
        }
    }

    // non-pinned queens
    BitLoop(self.queens & ~pinmask) {
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = sliderRook(rookMoves[pieceIndex], r.occupied, pieceIndex) & notSelf_n_checkMask;
        
        BitLoop2(reachable & esq) {
            newMoves.push_back(board.pieceMove<isWhite, 2>(_blsi_u64(temp), _blsi_u64(temp2)));
        }
        BitLoop2(reachable & nesq) {
            newMoves.push_back(board.pieceMoveCapture<isWhite, 2>(_blsi_u64(temp), _blsi_u64(temp2)));
        }
    }

    // pinned rooks
    BitLoop(self.rooks & r.pinmaskHV) { // D-pinned rooks cannot move
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = sliderRook(rookMoves[pieceIndex], r.occupied, pieceIndex) & r.pinmaskHV & notSelf_n_checkMask;
        
        BitLoop2(reachable & esq) {
            newMoves.push_back(board.pieceMove<isWhite, 3>(_blsi_u64(temp), _blsi_u64(temp2)));
        }
        BitLoop2(reachable & nesq) {
            newMoves.push_back(board.pieceMoveCapture<isWhite, 3>(_blsi_u64(temp), _blsi_u64(temp2)));
        }
    }

    // pinned queens
    BitLoop(self.queens & r.pinmaskHV) {
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = sliderRook(rookMoves[pieceIndex], r.occupied, pieceIndex) & r.pinmaskHV & notSelf_n_checkMask;
        
        BitLoop2(reachable & esq) {
            newMoves.push_back(board.pieceMove<isWhite, 2>(_blsi_u64(temp), _blsi_u64(temp2)));
        }
        BitLoop2(reachable & nesq) {
            newMoves.push_back(board.pieceMoveCapture<isWhite, 2>(_blsi_u64(temp), _blsi_u64(temp2)));
        }
    }

    // generate bishop/queen moves
    
    // non-pinned bishops
    BitLoop(self.bishops & ~pinmask) {
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = sliderBishop(bishopMoves[pieceIndex], r.occupied, pieceIndex) & notSelf_n_checkMask;;

        BitLoop2(reachable & esq) {
            newMoves.push_back(board.pieceMove<isWhite, 4>(_blsi_u64(temp), _blsi_u64(temp2)));
        }
        BitLoop2(reachable & nesq) {
            newMoves.push_back(board.pieceMoveCapture<isWhite, 4>(_blsi_u64(temp), _blsi_u64(temp2)));
        }
    }

    // non-pinned queens
    BitLoop(self.queens & ~pinmask) {
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = sliderBishop(bishopMoves[pieceIndex], r.occupied, pieceIndex) & notSelf_n_checkMask;;

        BitLoop2(reachable & esq) {
            newMoves.push_back(board.pieceMove<isWhite, 2>(_blsi_u64(temp), _blsi_u64(temp2)));
        }
        BitLoop2(reachable & nesq) {
            newMoves.push_back(board.pieceMoveCapture<isWhite, 2>(_blsi_u64(temp), _blsi_u64(temp2)));
        }
    }

    // pinned bishops
    BitLoop((self.bishops) & r.pinmaskD) { // HV-pinned bishop cannot move
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = sliderBishop(bishopMoves[pieceIndex], r.occupied, pieceIndex) & r.pinmaskD & notSelf_n_checkMask;

        BitLoop2(reachable & esq) {
            newMoves.push_back(board.pieceMove<isWhite, 4>(_blsi_u64(temp), _blsi_u64(temp2)));
        }
        BitLoop2(reachable & nesq) {
            newMoves.push_back(board.pieceMoveCapture<isWhite, 4>(_blsi_u64(temp), _blsi_u64(temp2)));
        }
    }

    // pinned queens
    BitLoop(self.queens & r.pinmaskD) {
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = sliderBishop(bishopMoves[pieceIndex], r.occupied, pieceIndex) & r.pinmaskD & notSelf_n_checkMask;

        BitLoop2(reachable & esq) {
            newMoves.push_back(board.pieceMove<isWhite, 2>(_blsi_u64(temp), _blsi_u64(temp2)));
        }
        BitLoop2(reachable & nesq) {
            newMoves.push_back(board.pieceMoveCapture<isWhite, 2>(_blsi_u64(temp), _blsi_u64(temp2)));
        }
    }

    // generate knight moves
    BitLoop(self.knights & ~pinmask) { // pinned knight cannot move, so only consider unpinned knights
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = knightMoves[pieceIndex] & notSelf_n_checkMask;

        BitLoop2(reachable & esq) {
            newMoves.push_back(board.pieceMove<isWhite, 5>(_blsi_u64(temp), _blsi_u64(temp2)));
        }
        BitLoop2(reachable & nesq) {
            newMoves.push_back(board.pieceMoveCapture<isWhite, 5>(_blsi_u64(temp), _blsi_u64(temp2)));
        }
    }

    Squares enemy_n_checkMask = r.checkMask & r.enemySquares;

    // PINNED PAWNS
    BitLoop(self.pawns & pinmask & pawnStartFile<isWhite>()) { // starting file pinned pawns
        uint64_t pieceIndex = bitFromSquare(temp);

        Squares nocc_pinmask_checkmask = ~r.occupied & r.pinmaskHV & r.checkMask;
        if (positionToBit[pieceIndex + pawnAdvance<isWhite>()] & nocc_pinmask_checkmask) { // i.e. the square in front of the pawn is not occupied            
            newMoves.push_back(board.pawnForward<isWhite>(_blsi_u64(temp), positionToBit[pieceIndex + pawnAdvance<isWhite>()]));

            if (positionToBit[pieceIndex + 2 * pawnAdvance<isWhite>()] & nocc_pinmask_checkmask) { // i.e. 2 squares in front of pawn not occupied
                newMoves.push_back(board.pawnPush<isWhite>(_blsi_u64(temp), positionToBit[pieceIndex + 2 * pawnAdvance<isWhite>()]));
            }
        }
        Squares reachable = pawnAttacks<isWhite>()[pieceIndex] & r.pinmaskD & enemy_n_checkMask;

        BitLoop2(reachable) {
            newMoves.push_back(board.pawnCapture<isWhite>(_blsi_u64(temp), _blsi_u64(temp2)));
        }
    }

    BitLoop(self.pawns & pinmask & 0x0000ffffffff0000ULL) { // pinned pawns not in starting file or previous to end file
        uint64_t pieceIndex = bitFromSquare(temp);

        if (positionToBit[pieceIndex + pawnAdvance<isWhite>()] & ~r.occupied & r.pinmaskHV & r.checkMask) { // i.e. the square in front of the pawn is not occupied
            newMoves.push_back(board.pawnForward<isWhite>(_blsi_u64(temp), positionToBit[pieceIndex + pawnAdvance<isWhite>()]));
        }
        Squares reachable = pawnAttacks<isWhite>()[pieceIndex] & r.pinmaskD & enemy_n_checkMask;

        BitLoop2(reachable) {
            newMoves.push_back(board.pawnCapture<isWhite>(_blsi_u64(temp), _blsi_u64(temp2)));
        }
    }

    BitLoop(self.pawns & pinmask & pawnPEndFile<isWhite>()) { // pinned pawns in next to end file
        uint64_t pieceIndex = bitFromSquare(temp);

        if (positionToBit[pieceIndex + pawnAdvance<isWhite>()] & ~r.occupied & r.pinmaskHV & r.checkMask) { // i.e. the square in front of the pawn is not occupied
            newMoves.push_back(board.pawnPromote<isWhite, 2>(_blsi_u64(temp), positionToBit[pieceIndex + pawnAdvance<isWhite>()]));
            newMoves.push_back(board.pawnPromote<isWhite, 3>(_blsi_u64(temp), positionToBit[pieceIndex + pawnAdvance<isWhite>()]));
            newMoves.push_back(board.pawnPromote<isWhite, 4>(_blsi_u64(temp), positionToBit[pieceIndex + pawnAdvance<isWhite>()]));
            newMoves.push_back(board.pawnPromote<isWhite, 5>(_blsi_u64(temp), positionToBit[pieceIndex + pawnAdvance<isWhite>()]));
        }
        Squares reachable = pawnAttacks<isWhite>()[pieceIndex] & r.pinmaskD & enemy_n_checkMask;

        BitLoop2(reachable) {
            newMoves.push_back(board.pawnPromoteCapture<isWhite, 2>(_blsi_u64(temp), positionToBit[pieceIndex + pawnAdvance<isWhite>()]));
            newMoves.push_back(board.pawnPromoteCapture<isWhite, 3>(_blsi_u64(temp), positionToBit[pieceIndex + pawnAdvance<isWhite>()]));
            newMoves.push_back(board.pawnPromoteCapture<isWhite, 4>(_blsi_u64(temp), positionToBit[pieceIndex + pawnAdvance<isWhite>()]));
            newMoves.push_back(board.pawnPromoteCapture<isWhite, 5>(_blsi_u64(temp), positionToBit[pieceIndex + pawnAdvance<isWhite>()]));
        }
    }

    // UNPINNED PAWNS
    BitLoop(self.pawns & ~pinmask & pawnStartFile<isWhite>()) { // starting file unpinned pawns
        uint64_t pieceIndex = bitFromSquare(temp);

        Squares nocc_checkmask = ~r.occupied & r.checkMask;
        if (positionToBit[pieceIndex + pawnAdvance<isWhite>()] & nocc_checkmask) { // i.e. the square in front of the pawn is not occupied            
            newMoves.push_back(board.pawnForward<isWhite>(_blsi_u64(temp), positionToBit[pieceIndex + pawnAdvance<isWhite>()]));

            if (positionToBit[pieceIndex + 2 * pawnAdvance<isWhite>()] & nocc_checkmask) { // i.e. 2 squares in front of pawn not occupied
                newMoves.push_back(board.pawnPush<isWhite>(_blsi_u64(temp), positionToBit[pieceIndex + 2 * pawnAdvance<isWhite>()]));
            }
        }
        Squares reachable = pawnAttacks<isWhite>()[pieceIndex] & enemy_n_checkMask;

        BitLoop2(reachable) {
            newMoves.push_back(board.pawnCapture<isWhite>(_blsi_u64(temp), _blsi_u64(temp2)));
        }
    }

    BitLoop(self.pawns & ~pinmask & 0x0000ffffffff0000ULL) { // unpinned pawns not in starting file or previous to end file
        uint64_t pieceIndex = bitFromSquare(temp);

        if (positionToBit[pieceIndex + pawnAdvance<isWhite>()] & ~r.occupied & r.checkMask) { // i.e. the square in front of the pawn is not occupied
            newMoves.push_back(board.pawnForward<isWhite>(_blsi_u64(temp), positionToBit[pieceIndex + pawnAdvance<isWhite>()]));
        }
        Squares reachable = pawnAttacks<isWhite>()[pieceIndex] & enemy_n_checkMask;

        BitLoop2(reachable) {
            newMoves.push_back(board.pawnCapture<isWhite>(_blsi_u64(temp), _blsi_u64(temp2)));
        }
    }

    BitLoop(self.pawns & ~pinmask & pawnPEndFile<isWhite>()) { // pinned pawns in next to end file
        uint64_t pieceIndex = bitFromSquare(temp);

        if (positionToBit[pieceIndex + pawnAdvance<isWhite>()] & ~r.occupied & r.checkMask) { // i.e. the square in front of the pawn is not occupied
            newMoves.push_back(board.pawnPromote<isWhite, 2>(_blsi_u64(temp), positionToBit[pieceIndex + pawnAdvance<isWhite>()]));
            newMoves.push_back(board.pawnPromote<isWhite, 3>(_blsi_u64(temp), positionToBit[pieceIndex + pawnAdvance<isWhite>()]));
            newMoves.push_back(board.pawnPromote<isWhite, 4>(_blsi_u64(temp), positionToBit[pieceIndex + pawnAdvance<isWhite>()]));
            newMoves.push_back(board.pawnPromote<isWhite, 5>(_blsi_u64(temp), positionToBit[pieceIndex + pawnAdvance<isWhite>()]));
        }
        Squares reachable = pawnAttacks<isWhite>()[pieceIndex] & enemy_n_checkMask;

        BitLoop2(reachable) {
            newMoves.push_back(board.pawnPromoteCapture<isWhite, 2>(_blsi_u64(temp), positionToBit[pieceIndex + pawnAdvance<isWhite>()]));
            newMoves.push_back(board.pawnPromoteCapture<isWhite, 3>(_blsi_u64(temp), positionToBit[pieceIndex + pawnAdvance<isWhite>()]));
            newMoves.push_back(board.pawnPromoteCapture<isWhite, 4>(_blsi_u64(temp), positionToBit[pieceIndex + pawnAdvance<isWhite>()]));
            newMoves.push_back(board.pawnPromoteCapture<isWhite, 5>(_blsi_u64(temp), positionToBit[pieceIndex + pawnAdvance<isWhite>()]));
        }
    }

    if (enPassant) { // there is an enpassant pawn for the enemy
        Squares enemyEnPassant = (Squares) enPassant << enPassantShift<isWhite>();
        BitLoop(self.pawns & pawnEP<isWhite>() & ~r.pinmaskHV & ~r.enpassantpin) { // enpassant NON - ep-pinned pawns, non HV pinned
            uint64_t pieceIndex = bitFromSquare(temp);

            if (positionToBit[pieceIndex + 1] & enemyEnPassant) {
                // check if the taken piece is not D pinned
                if (enemyEnPassant & r.pinmaskD) continue;
                // self piece is part of pinD -> end square must be part of pinD
                if ((positionToBit[pieceIndex] & r.pinmaskD) && !(positionToBit[pieceIndex + pawnAdvance<isWhite>() + 1] & r.pinmaskD)) continue;

                newMoves.push_back(board.pawnEnPassantCapture<isWhite>(positionToBit[pieceIndex + pawnAdvance<isWhite>() + 1], positionToBit[pieceIndex + 1]));
            } 
            if (positionToBit[pieceIndex - 1] & enemyEnPassant) {
                // check if the taken piece is not D pinned
                if (enemyEnPassant & r.pinmaskD) continue;
                // self piece is part of pinD -> end square must be part of pinD
                if ((positionToBit[pieceIndex] & r.pinmaskD) && !(positionToBit[pieceIndex + pawnAdvance<isWhite>() - 1] & r.pinmaskD)) continue;

                newMoves.push_back(board.pawnEnPassantCapture<isWhite>(positionToBit[pieceIndex + pawnAdvance<isWhite>() - 1], positionToBit[pieceIndex - 1]));
            }
        }
    }

    if (board.castlesStatus & castlesL<isWhite>() && !r.checkCount) { // castles left -> king index is 4 and L rook 0 or 60 and 56 
        if (!(r.occupied & castleLocc<isWhite>()) && !(castleLcheck<isWhite>() & r.enemySeen)) { // turret and king can see each other AND squares are not seen by enemy
            newMoves.push_back(board.castlesMove<isWhite, true>());
        }
    }

    if (board.castlesStatus & castlesR<isWhite>() && !r.checkCount) { // castles right
        if (!(r.occupied & castleRcheck<isWhite>()) && !(castleRcheck<isWhite>() & r.enemySeen)) { // turret and king can see each other AND squares are not seen by enemy
            newMoves.push_back(board.castlesMove<isWhite, false>());
        }
    }

    if (newMoves.size() == 0) {
        if (r.checkCount) newMoves.push_back({Pieces(), Pieces(), 0, !isWhite, 0b10000000});
        else newMoves.push_back({Pieces(), Pieces(), 0, !isWhite, 0b01000000});
        return newMoves;
    }
    return newMoves;
}

uint64_t traverse(const Board &initialPosition, int depth) {
    if (depth == 0) return 1ULL;

    uint64_t initialMoveCount = 0;

    if (initialPosition.isWhite) {
        auto initialmoves = generateMoves<true>(initialPosition);

        for (auto &position : initialmoves) {
            // std::cout << boardToStr(position) << std::endl;
            if (!(position.castlesStatus & 0b11000000)) initialMoveCount += traverse(position, depth-1);
            // else initialMoveCount++;
        }

        return initialMoveCount;
    }
    else {
        auto initialmoves = generateMoves<false>(initialPosition);

        for (auto &position : initialmoves) {
            // std::cout << boardToStr(position) << std::endl;
            if (!(position.castlesStatus & 0b11000000)) initialMoveCount += traverse(position, depth-1);
            // else initialMoveCount++;
        }

        return initialMoveCount;
    }
}


int main(void) {

    Board b = positionToBoard("RNBQKBNR/PPPPPPPP/8/8/8/8/pppppppp/rnbqkbnr/-/W");
    b.isWhite = true; b.castlesStatus = 0b1111;

    std::cout << boardToStr(b) << std::endl;

    auto start_time = std::chrono::high_resolution_clock::now();

    std::cout << "Depth 1: " << (unsigned long) traverse(b, 1) << std::endl;
    std::cout << "Depth 2: " << (unsigned long) traverse(b, 2) << std::endl;
    std::cout << "Depth 3: " << (unsigned long) traverse(b, 3) << std::endl;
    std::cout << "Depth 4: " << (unsigned long) traverse(b, 4) << std::endl;
    std::cout << "Depth 5: " << (unsigned long) traverse(b, 5) << std::endl;
    // std::cout << "Depth 6: " << (unsigned long) traverse(b, 6) << std::endl;
    // std::cout << "Depth 7: " << (unsigned long) traverse(b, 7) << std::endl;

    auto end_time = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    std::cout << "Execution time: " << duration.count() << " microseconds" << std::endl;
    return 0;
}