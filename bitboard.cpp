#include "display.cpp"
#include "loadSliderLookup.cpp"

#include <chrono>

SliderLookup bishopLookup("./bishopLookup", bishoppositions);
SliderLookup rookLookup("./rookLookup", rookpositions);

Squares occupied(const Pieces &pieces) {
    return (pieces.pawns|pieces.rooks|pieces.bishops|pieces.knights|pieces.queens|pieces.king);
}

Squares reachableNotBlockedBishop(Squares reachableIfNotBlocked, Squares occupied, uint64_t pieceIndex) {
    return bishopLookup.table[pieceIndex][_pext_u64(occupied, reachableIfNotBlocked)];
}

Squares reachableNotBlockedRook(Squares reachableIfNotBlocked, Squares occupied, uint64_t pieceIndex) {
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

isInCheckResult isInCheck(const Pieces &selfPieces, const Pieces &enemyPieces, bool isWhite) {

    Squares enemySquares = occupied(enemyPieces);
    Squares selfSquares = occupied(selfPieces);
    Squares occupiedSquares = enemySquares | selfSquares;

    Squares occupiedNotKing = occupiedSquares ^ selfPieces.king;

    bit kingIndex = bitFromSquare(selfPieces.king);

    int checkcount = 0;

    Squares checkMask = 0;
    Squares enemySeen = 0;
    Squares kingNotReachable = 0;
    Squares pinmaskHV = 0;
    Squares pinmaskD = 0;
    Squares enpassantpin = 0;

    BitLoop(enemyPieces.rooks | enemyPieces.queens) {
        uint64_t pieceIndex = bitFromSquare(temp);
        enemySeen |= reachableNotBlockedRook(rookMoves[pieceIndex], occupiedSquares, pieceIndex);

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
                    if (enemyPieces.enPassant && (pieceIndex/8 == kingIndex/8) && inbetween & selfPieces.pawns) { 
                        // there is enpassant pawn for enemy on the same file as the king AND the other piece is a self pawn
                        enpassantpin = inbetween & selfPieces.pawns; // i.e. mask the self pawn so that it cannot take enpassant
                    }
                }
            }
        }
    }

    BitLoop(enemyPieces.bishops | enemyPieces.queens) {
        uint64_t pieceIndex = bitFromSquare(temp);
        enemySeen |= reachableNotBlockedBishop(bishopMoves[pieceIndex], occupiedSquares, pieceIndex);

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
        enemySeen |= pawnAttacks(!isWhite)[pieceIndex];
        
        if (pawnAttacks(!isWhite)[pieceIndex] & selfPieces.king) { // i.e. pawn attacking our king
            checkMask |= positionToBit[pieceIndex];
            checkcount++;
        }
    }

    enemySeen |= kingMoves[bitFromSquare(enemyPieces.king)];

    return {checkcount, checkMask, enemySeen, kingNotReachable | enemySeen, pinmaskHV, pinmaskD, enpassantpin, enemySquares, selfSquares, occupiedSquares};
}

Pieces removePiece(Pieces pieces, Squares removepositions) {
    Squares n_remove = ~removepositions;
    pieces.bishops &= n_remove; pieces.rooks &= n_remove; pieces.queens &= n_remove; 
    pieces.knights &= n_remove; pieces.pawns &= n_remove;
    return pieces;
}

struct move {
    uint16_t movedPiece; 
    // 0b000000_000000_000_0 
    // (fist bit to determine if this number is considered) (next 6 the initial position of the piece) (next 3 to determine piece type)
    // piece types: 1-King 2-Queen 3-Rook 4-Bishop 5-Knight 6-Pawn 0-No piece moved
    uint16_t movedPiece2; // for castles and enpassant
    uint8_t enpassant; // new enpassant status
    uint8_t castles; // new castles status
};

Board applyMove(Board board, const move &m) {

    Pieces *self, *enemy;
    if (board.isWhite) {
        self = &board.whitePieces;
        enemy = &board.blackPieces;
    } else {
        enemy = &board.whitePieces;
        self = &board.blackPieces;
    }

    self->enPassant = m.enpassant;
    self->castles = m.castles;

    board.isWhite = !board.isWhite;
    Squares finalpos = positionToBit[m.movedPiece >> 10];
    Squares nfinalpos = ~positionToBit[m.movedPiece >> 10];

    switch ((m.movedPiece & 0b0000000000001110U) >> 1)
    {
    case 0U:
        if (m.castles & 0b00110000U) {board.gameResult = 0b10U; return board;} // checkmate
        else if (m.castles & 0b11000000U) {board.gameResult = 0b01U; return board;} // draw
        break;
    case 1U: // king move
        self->king ^= positionToBit[(m.movedPiece & 0b0000001111110000U) >> 4] | finalpos;
        enemy->queens &= nfinalpos; enemy->rooks &= nfinalpos; enemy->bishops &= nfinalpos;
        enemy->knights &= nfinalpos; enemy->pawns &= nfinalpos;

        // handle casles
        if (m.movedPiece2) {
            self->rooks ^= positionToBit[(m.movedPiece2 & 0b0000001111110000U) >> 4] | positionToBit[m.movedPiece2 >> 10];
        }
        break;
    case 2U: // queen move
        self->queens ^= positionToBit[(m.movedPiece & 0b0000001111110000U) >> 4] | finalpos;
        enemy->queens &= nfinalpos; enemy->rooks &= nfinalpos; enemy->bishops &= nfinalpos;
        enemy->knights &= nfinalpos; enemy->pawns &= nfinalpos;
        break;
    case 3U: // rook move
        self->rooks ^= positionToBit[(m.movedPiece & 0b0000001111110000U) >> 4] | finalpos;
        enemy->queens &= nfinalpos; enemy->rooks &= nfinalpos; enemy->bishops &= nfinalpos;
        enemy->knights &= nfinalpos; enemy->pawns &= nfinalpos;
        break;
    case 4U: // bishop move
        self->bishops ^= positionToBit[(m.movedPiece & 0b0000001111110000U) >> 4] | finalpos;
        enemy->queens &= nfinalpos; enemy->rooks &= nfinalpos; enemy->bishops &= nfinalpos;
        enemy->knights &= nfinalpos; enemy->pawns &= nfinalpos;
        break;
    case 5U: // knight move
        self->knights ^= positionToBit[(m.movedPiece & 0b0000001111110000U) >> 4] | finalpos;
        enemy->queens &= nfinalpos; enemy->rooks &= nfinalpos; enemy->bishops &= nfinalpos;
        enemy->knights &= nfinalpos; enemy->pawns &= nfinalpos;
        break;
    case 6U: // pawn move
        self->pawns ^= positionToBit[(m.movedPiece & 0b0000001111110000U) >> 4] | finalpos;
        enemy->queens &= nfinalpos; enemy->rooks &= nfinalpos; enemy->bishops &= nfinalpos;
        enemy->knights &= nfinalpos; enemy->pawns &= nfinalpos;

        // handle enpassant
        if (m.movedPiece2) {
            enemy->pawns &= ~positionToBit[(m.movedPiece2 & 0b0000001111110000U) >> 4];
        }
        break;
    case 7U: // promotion
        self->pawns ^= positionToBit[(m.movedPiece & 0b0000001111110000U) >> 4];
        enemy->queens &= nfinalpos; enemy->rooks &= nfinalpos; enemy->bishops &= nfinalpos;
        enemy->knights &= nfinalpos; enemy->pawns &= nfinalpos;

        switch ((m.movedPiece2 & 0b0000000000001110U) >> 1)
        {
        case 2U:
            self->queens ^= positionToBit[m.movedPiece >> 10];
            break;
        case 3U:
            self->rooks ^= positionToBit[m.movedPiece >> 10];
            break;
        case 4U:
            self->bishops ^= positionToBit[m.movedPiece >> 10];
            break;
        case 5U:
            self->knights ^= positionToBit[m.movedPiece >> 10];
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    return board;
}

uint16_t createPieceMove(uint8_t pieceType, uint8_t initSquare, uint8_t finalSquare, bool isWhite) { // takes index as input
    return (pieceType << 1) | (initSquare << 4) | (finalSquare << 10) | (isWhite ? 1U : 0U);
}

template<bool isWhite>
std::vector<move> generateMovesShort(const Board &board) {
    std::vector<move> newMoves;

    Pieces self, enemy;

    if (isWhite) {
        self = board.whitePieces;
        enemy = board.blackPieces;
    } else {
        self = board.blackPieces;
        enemy = board.whitePieces;
    }

    uint8_t enPassant = enemy.enPassant;

    // generate pinmask and checkmask
    isInCheckResult r = isInCheck(self, enemy, isWhite); 
    Squares pinmask = r.pinmaskD | r.pinmaskHV;
    Squares notSelf = ~r.selfSquares;
    uint64_t kingIndex = bitFromSquare(self.king);

    if (r.checkCount > 1) { // double check forces king to move
        // king can only move to squares not seen by enemy and not occupied by enemy pieces
        Squares kingReachable = kingMoves[kingIndex] & notSelf & ~r.enemySeen & ~r.kingNotReachable;

        BitLoop(kingReachable) {            
            newMoves.push_back({createPieceMove(1, kingIndex, bitFromSquare(temp), isWhite), 0U, 0U, 0U});
        }

        if (newMoves.size() == 0) newMoves.push_back({0U, 0U, 0U, 0b00110000U}); // checkmate

        return newMoves;
    } 
    if (r.checkCount == 0) r.checkMask = 0xffffffffffffffff; // if no checks are given, then all squares are available for all pieces

    Pieces selfPiecesNew;  

    // generate king moves
    Squares kingReachable = kingMoves[kingIndex] & notSelf & ~r.enemySeen & ~r.kingNotReachable;
    BitLoop(kingReachable) {
        newMoves.push_back({createPieceMove(1, kingIndex, bitFromSquare(temp), isWhite), 0U, 0U, 0U});
    }

    // generate rook/queen moves
    Squares notSelf_n_checkMask = notSelf & r.checkMask;
    
    // non-pinned rooks
    BitLoop(self.rooks & ~pinmask) {
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = reachableNotBlockedRook(rookMoves[pieceIndex], r.occupied, pieceIndex) & notSelf_n_checkMask;
        
        uint8_t castles_result = self.castles;
        if (pieceIndex == (isWhite ? 0 : 56)) castles_result &= 0b10; // moving L Rook so cannot L castle
        if (pieceIndex == (isWhite ? 7 : 63)) castles_result &= 0b01; // moving R Rook so cannot R castle

        BitLoop2(reachable) {
            newMoves.push_back({createPieceMove(3, pieceIndex, bitFromSquare(temp2), isWhite), 0U, 0U, castles_result});
        }
    }

    // non-pinned queens
    BitLoop(self.queens & ~pinmask) {
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = reachableNotBlockedRook(rookMoves[pieceIndex], r.occupied, pieceIndex) & notSelf_n_checkMask;
        
        BitLoop2(reachable) {
            newMoves.push_back({createPieceMove(2, pieceIndex, bitFromSquare(temp2), isWhite), 0U, 0U, self.castles});
        }
    }

    // pinned rooks
    BitLoop(self.rooks & r.pinmaskHV) { // D-pinned rooks cannot move
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = reachableNotBlockedRook(rookMoves[pieceIndex], r.occupied, pieceIndex) & r.pinmaskHV & notSelf_n_checkMask;
        
        uint8_t castles_result = self.castles;
        if (pieceIndex == (isWhite ? 0 : 56)) castles_result &= 0b10; // moving L Rook so cannot L castle
        if (pieceIndex == (isWhite ? 7 : 63)) castles_result &= 0b01; // moving R Rook so cannot R castle

        BitLoop2(reachable) {
            newMoves.push_back({createPieceMove(3, pieceIndex, bitFromSquare(temp2), isWhite), 0U, 0U, castles_result});
        }
    }

    // pinned queens
    BitLoop(self.queens & r.pinmaskHV) {
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = reachableNotBlockedRook(rookMoves[pieceIndex], r.occupied, pieceIndex) & r.pinmaskHV & notSelf_n_checkMask;
        
        BitLoop2(reachable) {
            newMoves.push_back({createPieceMove(2, pieceIndex, bitFromSquare(temp2), isWhite), 0U, 0U, self.castles});
        }
    }

    // generate bishop/queen moves
    
    // non-pinned bishops
    BitLoop(self.bishops & ~pinmask) {
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = reachableNotBlockedBishop(bishopMoves[pieceIndex], r.occupied, pieceIndex) & notSelf_n_checkMask;;

        BitLoop2(reachable) {
            newMoves.push_back({createPieceMove(4, pieceIndex, bitFromSquare(temp2), isWhite), 0U, 0U, self.castles});
        }
    }

    // non-pinned queens
    BitLoop(self.queens & ~pinmask) {
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = reachableNotBlockedBishop(bishopMoves[pieceIndex], r.occupied, pieceIndex) & notSelf_n_checkMask;;

        BitLoop2(reachable) {
            newMoves.push_back({createPieceMove(2, pieceIndex, bitFromSquare(temp2), isWhite), 0U, 0U, self.castles});
        }
    }

    // pinned bishops
    BitLoop((self.bishops) & r.pinmaskD) { // HV-pinned bishop cannot move
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = reachableNotBlockedBishop(bishopMoves[pieceIndex], r.occupied, pieceIndex) & r.pinmaskD & notSelf_n_checkMask;

        BitLoop2(reachable) {
            newMoves.push_back({createPieceMove(4, pieceIndex, bitFromSquare(temp2), isWhite), 0U, 0U, self.castles});
        }
    }

    // pinned queens
    BitLoop(self.queens & r.pinmaskD) {
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = reachableNotBlockedBishop(bishopMoves[pieceIndex], r.occupied, pieceIndex) & r.pinmaskD & notSelf_n_checkMask;

        BitLoop2(reachable) {
            newMoves.push_back({createPieceMove(2, pieceIndex, bitFromSquare(temp2), isWhite), 0U, 0U, self.castles});
        }
    }

    // generate knight moves
    BitLoop(self.knights & ~pinmask) { // pinned knight cannot move, so only consider unpinned knights
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = knightMoves[pieceIndex] & notSelf_n_checkMask;

        BitLoop2(reachable) {
            newMoves.push_back({createPieceMove(5, pieceIndex, bitFromSquare(temp2), isWhite), 0U, 0U, self.castles});
        }
    }

    Squares enemy_n_checkMask = r.checkMask & r.enemySquares;

    // PAWNS
    BitLoop(self.pawns & pinmask & pawnStartFile(isWhite)) { // starting file pinned pawns
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = 0;

        if (positionToBit[pieceIndex + pawnAdvace(isWhite)] & ~r.occupied) { // i.e. the square in front of the pawn is not occupied
            reachable |= positionToBit[pieceIndex + pawnAdvace(isWhite)] & r.pinmaskHV & r.checkMask; // i.e. the newposition is available only if it stays in the pinmask

            if (positionToBit[pieceIndex + 2 * pawnAdvace(isWhite)] & ~r.occupied & r.pinmaskHV & r.checkMask) { // i.e. 2 squares in front of pawn not occupied
                newMoves.push_back(
                    {createPieceMove(6, pieceIndex, pieceIndex + 2 * pawnAdvace(isWhite), isWhite), 0U, 
                    (uint8_t)(positionToBit[pieceIndex] >> (isWhite ? 8 : 48)) , self.castles});
            }
        }
        reachable |= pawnAttacks(isWhite)[pieceIndex] & r.pinmaskD & enemy_n_checkMask;

        BitLoop2(reachable) {
            newMoves.push_back({createPieceMove(6, pieceIndex, bitFromSquare(temp2), isWhite), 0U, 0U, self.castles});
        }
    }

    BitLoop(self.pawns & pinmask & ~pawnStartFile(isWhite)) { // pinned pawns not in starting file
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = 0;
        if (positionToBit[pieceIndex + pawnAdvace(isWhite)] & ~r.occupied) { // i.e. the square in front of the pawn is not occupied
            reachable |= positionToBit[pieceIndex + pawnAdvace(isWhite)] & r.pinmaskHV & r.checkMask; // i.e. the newposition is available only if it stays in the pinmask
        }
        reachable |= pawnAttacks(isWhite)[pieceIndex] & r.pinmaskD & enemy_n_checkMask;

        BitLoop2(reachable) {
            Squares finalIndex = bitFromSquare(temp2);

            if (positionToBit[finalIndex] & pawnEndFile(isWhite)) { // pawn promotion
                newMoves.push_back( // promote to queen
                    {createPieceMove(7, pieceIndex, finalIndex, isWhite), createPieceMove(2, 0, finalIndex, isWhite), 0U, self.castles});

                newMoves.push_back( // promote to rook
                    {createPieceMove(7, pieceIndex, finalIndex, isWhite), createPieceMove(3, 0, finalIndex, isWhite), 0U, self.castles});

                newMoves.push_back( // promote to bishop
                    {createPieceMove(7, pieceIndex, finalIndex, isWhite), createPieceMove(4, 0, finalIndex, isWhite), 0U, self.castles});

                newMoves.push_back( // promote to knight
                    {createPieceMove(7, pieceIndex, finalIndex, isWhite), createPieceMove(5, 0, finalIndex, isWhite), 0U, self.castles});
            } else {
                newMoves.push_back({createPieceMove(6, pieceIndex, finalIndex, isWhite), 0U, 0U, self.castles});
            }
        }
    }

    BitLoop(self.pawns & ~pinmask & pawnStartFile(isWhite)) { // starting file NON - pinned pawns
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = 0;

        if (positionToBit[pieceIndex + pawnAdvace(isWhite)] & ~r.occupied) { // i.e. the square in front of the pawn is not occupied
            reachable |= positionToBit[pieceIndex + pawnAdvace(isWhite)] & r.checkMask; // i.e. the newposition is available only if it stays in the pinmask

            if (positionToBit[pieceIndex + 2 * pawnAdvace(isWhite)] & ~r.occupied & r.checkMask) { // i.e. 2 squares in front of pawn not occupied
                newMoves.push_back(
                    {createPieceMove(6, pieceIndex, pieceIndex + 2 * pawnAdvace(isWhite), isWhite), 0U, 
                    (uint8_t)(positionToBit[pieceIndex] >> (isWhite ? 8 : 48)), self.castles});
            }
        }
        reachable |= pawnAttacks(isWhite)[pieceIndex] & enemy_n_checkMask;

        BitLoop2(reachable) {
            newMoves.push_back({createPieceMove(6, pieceIndex, bitFromSquare(temp2), isWhite), 0U, 0U, self.castles});
        }
    }

    BitLoop(self.pawns & ~pinmask & ~pawnStartFile(isWhite)) { // non pinned pawns not in starting file
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = 0;
        if (positionToBit[pieceIndex + pawnAdvace(isWhite)] & ~r.occupied) { // i.e. the square in front of the pawn is not occupied
            reachable |= positionToBit[pieceIndex + pawnAdvace(isWhite)] & r.checkMask;
        }
        reachable |= pawnAttacks(isWhite)[pieceIndex] & enemy_n_checkMask;

        BitLoop2(reachable) {
            Squares finalIndex = bitFromSquare(temp2);

            if (positionToBit[finalIndex] & pawnEndFile(isWhite)) { // pawn promotion
                newMoves.push_back( // promote to queen
                    {createPieceMove(7, pieceIndex, finalIndex, isWhite), createPieceMove(2, 0, finalIndex, isWhite), 0U, self.castles});

                newMoves.push_back( // promote to rook
                    {createPieceMove(7, pieceIndex, finalIndex, isWhite), createPieceMove(3, 0, finalIndex, isWhite), 0U, self.castles});

                newMoves.push_back( // promote to bishop
                    {createPieceMove(7, pieceIndex, finalIndex, isWhite), createPieceMove(4, 0, finalIndex, isWhite), 0U, self.castles});

                newMoves.push_back( // promote to knight
                    {createPieceMove(7, pieceIndex, finalIndex, isWhite), createPieceMove(5, 0, finalIndex, isWhite), 0U, self.castles});
            } else {
                newMoves.push_back({createPieceMove(6, pieceIndex, finalIndex, isWhite), 0U, 0U, self.castles});
            }
        }
    }

    if (enPassant) { // there is an enpassant pawn for the enemy
        Squares enemyEnPassant = (Squares) enPassant << (isWhite ? 32 : 24);
        BitLoop(self.pawns & pawnEP(isWhite) & ~r.pinmaskHV & ~r.enpassantpin) { // enpassant NON - ep-pinned pawns, non HV pinned
            uint64_t pieceIndex = bitFromSquare(temp);

            if (positionToBit[pieceIndex + 1] & enemyEnPassant) {
                // check if the taken piece is not D pinned
                if (enemyEnPassant & r.pinmaskD) continue;
                // self piece is part of pinD -> end square must be part of pinD
                if ((positionToBit[pieceIndex] & r.pinmaskD) && !(positionToBit[pieceIndex + pawnAdvace(isWhite) + 1] & r.pinmaskD)) continue;

                newMoves.push_back( // enpassant capture
                    {createPieceMove(6, pieceIndex, pieceIndex + pawnAdvace(isWhite) + 1, isWhite), createPieceMove(6, pieceIndex + 1, 0, !isWhite), 0U, self.castles});
            } 
            if (positionToBit[pieceIndex - 1] & enemyEnPassant) {
                // check if the taken piece is not D pinned
                if (enemyEnPassant & r.pinmaskD) continue;
                // self piece is part of pinD -> end square must be part of pinD
                if ((positionToBit[pieceIndex] & r.pinmaskD) && !(positionToBit[pieceIndex + pawnAdvace(isWhite) - 1] & r.pinmaskD)) continue;

                newMoves.push_back( // enpassant capture
                    {createPieceMove(6, pieceIndex, pieceIndex + pawnAdvace(isWhite) - 1, isWhite), createPieceMove(6, pieceIndex - 1, 0, !isWhite), 0U, self.castles});
            }
        }
    }

    if (self.castles & 1U && !r.checkCount) { // castles left -> king index is 4 and L rook 0 or 60 and 56 
        if (!(r.occupied & castleLocc(isWhite)) && !(castleLcheck(isWhite) & r.enemySeen)) { // turret and king can see each other AND squares are not seen by enemy
            newMoves.push_back( // castles left
                {createPieceMove(1, isWhite ? 4 : 60, isWhite ? 2 : 58, isWhite), createPieceMove(3, isWhite ? 0 : 56, isWhite ? 3 : 59, isWhite), 0U, 0U});
        }
    }

    if (self.castles & 2U && !r.checkCount) { // castles right
        if (!(r.occupied & castleRcheck(isWhite)) && !(castleRcheck(isWhite) & r.enemySeen)) { // turret and king can see each other AND squares are not seen by enemy
            newMoves.push_back( // castles right
                {createPieceMove(1, isWhite ? 4 : 60, isWhite ? 6 : 62, isWhite), createPieceMove(3, isWhite ? 7 : 63, isWhite ? 4 : 61, isWhite), 0U, 0U});
        }
    }

    if (newMoves.size() == 0) {
        newMoves.push_back({0, 0, 0, (r.checkCount ? (uint8_t) 0b00110000U : (uint8_t)0b11000000U)}); // checkmate or draw
        return newMoves;
    }
    return newMoves;
}

uint64_t traverseWithMoves(const Board &initialPosition, int depth) {
    if (depth==0) {
        return 1ULL;
    }

    uint64_t initialMoveCount = 0;

    if (initialPosition.isWhite) {
        auto moves = generateMovesShort<true>(initialPosition);

        for (auto &m : moves) {
            // std::cout << boardToStr(position) << std::endl;
            if (!(m.castles & 0xf0)) initialMoveCount += traverseWithMoves(applyMove(initialPosition, m), depth-1);
            // else initialMoveCount++;
        }

        return initialMoveCount;
    }
    else {
        auto moves = generateMovesShort<false>(initialPosition);

        for (auto &m : moves) {
            // std::cout << boardToStr(position) << std::endl;
            if (!(m.castles & 0xf0)) initialMoveCount += traverseWithMoves(applyMove(initialPosition, m), depth-1);
            // else initialMoveCount++;
        }

        return initialMoveCount;
    }
}

template<bool isWhite>
std::vector<Board> generateMoves(const Board &board) {
    std::vector<Board> newMoves;

    Pieces self, enemy;

    if (isWhite) {
        self = board.whitePieces;
        enemy = board.blackPieces;
    } else {
        self = board.blackPieces;
        enemy = board.whitePieces;
    }

    uint8_t enPassant = enemy.enPassant;
    enemy.enPassant = 0; // restore enpassant status for enemy

    // generate pinmask and checkmask
    isInCheckResult r = isInCheck(self, enemy, isWhite); 
    Squares pinmask = r.pinmaskD | r.pinmaskHV;
    Squares notSelf = ~r.selfSquares;
    uint64_t kingIndex = bitFromSquare(self.king);

    if (r.checkCount > 1) { // double check forces king to move
        // king can only move to squares not seen by enemy and not occupied by enemy pieces
        Squares kingReachable = kingMoves[kingIndex] & notSelf & ~r.enemySeen & ~r.kingNotReachable;
        Pieces selfPiecesNew = self; selfPiecesNew.king ^= self.king; selfPiecesNew.castles = 0;

        BitLoop(kingReachable) {
            Squares finalsquare = _blsi_u64(temp);
            Pieces enemyPiecesNew = removePiece(enemy, finalsquare);
            selfPiecesNew.king ^= finalsquare;
            
            if (isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !isWhite, (uint8_t) 0});
            else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !isWhite, (uint8_t) 0});

            selfPiecesNew.king ^= finalsquare;
        }

        if (newMoves.size() == 0) {
            if (isWhite) newMoves.push_back({self, enemy, isWhite, (uint8_t) (r.checkCount ? 0b1 : 0b10)});
            else newMoves.push_back({enemy, self, isWhite, (uint8_t) (r.checkCount ? 0b1 : 0b10)});
            return newMoves;
        }
        return newMoves;
    } 
    if (r.checkCount == 0) r.checkMask = 0xffffffffffffffff; // if no checks are given, then all squares are available for all pieces

    Pieces selfPiecesNew;  

    // generate king moves
    Squares kingReachable = kingMoves[kingIndex] & notSelf & ~r.enemySeen & ~r.kingNotReachable;
    // std::cout << "KingMoves" << std::endl; display_int64(kingReachable);
    selfPiecesNew = self; selfPiecesNew.king ^= self.king; selfPiecesNew.castles = 0;
    BitLoop(kingReachable) {
        Squares finalsquare = _blsi_u64(temp);
        Pieces enemyPiecesNew = removePiece(enemy, finalsquare);
        selfPiecesNew.king ^= finalsquare;

        if (isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !isWhite, (uint8_t) 0});
        else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !isWhite, (uint8_t) 0});

        selfPiecesNew.king ^= finalsquare;
    }

    // generate rook/queen moves
    Squares notSelf_n_checkMask = notSelf & r.checkMask;
    
    // non-pinned rooks
    BitLoop(self.rooks & ~pinmask) {
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = reachableNotBlockedRook(rookMoves[pieceIndex], r.occupied, pieceIndex) & notSelf_n_checkMask;
        // std::cout << "Non-Pinned rooks" << std::endl; display_int64(reachable);
        selfPiecesNew = self; selfPiecesNew.rooks ^= positionToBit[pieceIndex];
        
        if (pieceIndex == (isWhite ? 0 : 56)) selfPiecesNew.castles &= 0b10; // moving L Rook so cannot L castle
        if (pieceIndex == (isWhite ? 7 : 63)) selfPiecesNew.castles &= 0b01; // moving R Rook so cannot R castle

        BitLoop2(reachable) {
            Squares finalsquare = _blsi_u64(temp2);
            Pieces enemyPiecesNew = removePiece(enemy, finalsquare);
            selfPiecesNew.rooks ^= finalsquare;

            if (isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !isWhite, (uint8_t) 0});
            else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !isWhite, (uint8_t) 0});

            selfPiecesNew.rooks ^= finalsquare;
        }
    }

    // non-pinned queens
    BitLoop(self.queens & ~pinmask) {
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = reachableNotBlockedRook(rookMoves[pieceIndex], r.occupied, pieceIndex) & notSelf_n_checkMask;
        // std::cout << "Non-Pinned rooks" << std::endl; display_int64(reachable);
        selfPiecesNew = self; selfPiecesNew.queens ^= positionToBit[pieceIndex];
        
        BitLoop2(reachable) {
            Squares finalsquare = _blsi_u64(temp2);
            Pieces enemyPiecesNew = removePiece(enemy, finalsquare);
            selfPiecesNew.queens ^= finalsquare;

            if (isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !isWhite, (uint8_t) 0});
            else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !isWhite, (uint8_t) 0});

            selfPiecesNew.queens ^= finalsquare;
        }
    }

    // pinned rooks
    BitLoop(self.rooks & r.pinmaskHV) { // D-pinned rooks cannot move
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = reachableNotBlockedRook(rookMoves[pieceIndex], r.occupied, pieceIndex) & r.pinmaskHV & notSelf_n_checkMask;
        // std::cout << "Pinned rooks" << std::endl; display_int64(reachable);
        selfPiecesNew = self; selfPiecesNew.rooks ^= positionToBit[pieceIndex];
        
        if (pieceIndex == (isWhite ? 0 : 56)) selfPiecesNew.castles &= 0b10; // moving L Rook so cannot L castle
        if (pieceIndex == (isWhite ? 7 : 63)) selfPiecesNew.castles &= 0b01; // moving R Rook so cannot R castle

        BitLoop2(reachable) {
            Squares finalsquare = _blsi_u64(temp2);
            Pieces enemyPiecesNew = removePiece(enemy, finalsquare);
            selfPiecesNew.rooks ^= finalsquare;

            if (isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !isWhite, (uint8_t) 0});
            else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !isWhite, (uint8_t) 0});

            selfPiecesNew.rooks ^= finalsquare;
        }
    }

    // pinned queens
    BitLoop(self.queens & r.pinmaskHV) {
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = reachableNotBlockedRook(rookMoves[pieceIndex], r.occupied, pieceIndex) & r.pinmaskHV & notSelf_n_checkMask;
        // std::cout << "Pinned rooks" << std::endl; display_int64(reachable);
        selfPiecesNew = self; selfPiecesNew.queens ^= positionToBit[pieceIndex];
        
        BitLoop2(reachable) {
            Squares finalsquare = _blsi_u64(temp2);
            Pieces enemyPiecesNew = removePiece(enemy, finalsquare);
            selfPiecesNew.queens ^= finalsquare;

            if (isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !isWhite, (uint8_t) 0});
            else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !isWhite, (uint8_t) 0});

            selfPiecesNew.queens ^= finalsquare;
        }
    }

    // generate bishop/queen moves
    
    // non-pinned bishops
    BitLoop(self.bishops & ~pinmask) {
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = reachableNotBlockedBishop(bishopMoves[pieceIndex], r.occupied, pieceIndex) & notSelf_n_checkMask;;
        // std::cout << "Non-pinned bishops" << std::endl; display_int64(reachable);
        selfPiecesNew = self; selfPiecesNew.bishops ^= positionToBit[pieceIndex];

        BitLoop2(reachable) {
            Squares finalsquare = _blsi_u64(temp2);
            Pieces enemyPiecesNew = removePiece(enemy, finalsquare);
            selfPiecesNew.bishops ^= finalsquare;

            if (isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !isWhite, (uint8_t) 0});
            else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !isWhite, (uint8_t) 0});

            selfPiecesNew.bishops ^= finalsquare;
        }
    }

    // non-pinned queens
    BitLoop(self.queens & ~pinmask) {
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = reachableNotBlockedBishop(bishopMoves[pieceIndex], r.occupied, pieceIndex) & notSelf_n_checkMask;;
        // std::cout << "Non-pinned bishops" << std::endl; display_int64(reachable);
        selfPiecesNew = self; selfPiecesNew.queens ^= positionToBit[pieceIndex];

        BitLoop2(reachable) {
            Squares finalsquare = _blsi_u64(temp2);
            Pieces enemyPiecesNew = removePiece(enemy, finalsquare);
            selfPiecesNew.queens ^= finalsquare;

            if (isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !isWhite, (uint8_t) 0});
            else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !isWhite, (uint8_t) 0});

            selfPiecesNew.queens ^= finalsquare;
        }
    }

    // pinned bishops
    BitLoop((self.bishops) & r.pinmaskD) { // HV-pinned bishop cannot move
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = reachableNotBlockedBishop(bishopMoves[pieceIndex], r.occupied, pieceIndex) & r.pinmaskD & notSelf_n_checkMask;
        // std::cout << "Pinned bishops" << std::endl; display_int64(reachable);
        selfPiecesNew = self; selfPiecesNew.bishops ^= positionToBit[pieceIndex];

        BitLoop2(reachable) {
            Squares finalsquare = _blsi_u64(temp2);
            Pieces enemyPiecesNew = removePiece(enemy, finalsquare);
            selfPiecesNew.bishops ^= finalsquare;

            if (isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !isWhite, (uint8_t) 0});
            else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !isWhite, (uint8_t) 0});

            selfPiecesNew.bishops ^= finalsquare;
        }
    }

    // pinned queens
    BitLoop(self.queens & r.pinmaskD) {
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = reachableNotBlockedBishop(bishopMoves[pieceIndex], r.occupied, pieceIndex) & r.pinmaskD & notSelf_n_checkMask;
        // std::cout << "Pinned bishops" << std::endl; display_int64(reachable);
        selfPiecesNew = self; selfPiecesNew.queens ^= positionToBit[pieceIndex];

        BitLoop2(reachable) {
            Squares finalsquare = _blsi_u64(temp2);
            Pieces enemyPiecesNew = removePiece(enemy, finalsquare);
            selfPiecesNew.queens ^= finalsquare;

            if (isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !isWhite, (uint8_t) 0});
            else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !isWhite, (uint8_t) 0});

            selfPiecesNew.queens ^= finalsquare;
        }
    }

    // generate knight moves
    BitLoop(self.knights & ~pinmask) { // pinned knight cannot move, so only consider unpinned knights
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = knightMoves[pieceIndex] & notSelf_n_checkMask;
        // std::cout << "Non-pinned Knights" << std::endl; display_int64(reachable);
        selfPiecesNew = self; selfPiecesNew.knights ^= positionToBit[pieceIndex];

        BitLoop2(reachable) {
            Squares finalsquare = _blsi_u64(temp2);
            Pieces enemyPiecesNew = removePiece(enemy, finalsquare);
            selfPiecesNew.knights ^= finalsquare;

            if (isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !isWhite, (uint8_t) 0});
            else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !isWhite, (uint8_t) 0});

            selfPiecesNew.knights ^= finalsquare;
        }
    }

    Squares enemy_n_checkMask = r.checkMask & r.enemySquares;

    // PAWNS
    BitLoop(self.pawns & pinmask & pawnStartFile(isWhite)) { // starting file pinned pawns
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = 0;

        selfPiecesNew = self; selfPiecesNew.pawns ^= positionToBit[pieceIndex];    

        if (positionToBit[pieceIndex + pawnAdvace(isWhite)] & ~r.occupied) { // i.e. the square in front of the pawn is not occupied
            reachable |= positionToBit[pieceIndex + pawnAdvace(isWhite)] & r.pinmaskHV & r.checkMask; // i.e. the newposition is available only if it stays in the pinmask

            if (positionToBit[pieceIndex + 2 * pawnAdvace(isWhite)] & ~r.occupied & r.pinmaskHV & r.checkMask) { // i.e. 2 squares in front of pawn not occupied
                // reachable |= positionToBit[pieceIndex + 2 * pawnAdvace(isWhite)] & r.pinmaskHV & r.checkMask;

                // handle enpassant adding position different
                Squares finalsquare = positionToBit[pieceIndex + 2 * pawnAdvace(isWhite)];
                Pieces enemyPiecesNew = removePiece(enemy, finalsquare);
                selfPiecesNew.pawns ^= finalsquare;
                selfPiecesNew.enPassant = finalsquare >> (isWhite ? 24 : 32);

                if (isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !isWhite, (uint8_t) 0});
                else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !isWhite, (uint8_t) 0});

                selfPiecesNew.pawns ^= finalsquare;
                selfPiecesNew.enPassant = 0;
            }
        }
        reachable |= pawnAttacks(isWhite)[pieceIndex] & r.pinmaskD & enemy_n_checkMask;
        // std::cout << "Starting file pinned pawns" << std::endl; display_int64(reachable);
        selfPiecesNew = self; selfPiecesNew.pawns ^= positionToBit[pieceIndex];

        BitLoop2(reachable) {
            Squares finalsquare = _blsi_u64(temp2);
            Pieces enemyPiecesNew = removePiece(enemy, finalsquare);
            selfPiecesNew.pawns ^= finalsquare;

            if (isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !isWhite, (uint8_t) 0});
            else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !isWhite, (uint8_t) 0});

            selfPiecesNew.pawns ^= finalsquare;
        }
    }

    BitLoop(self.pawns & pinmask & ~pawnStartFile(isWhite)) { // pinned pawns not in starting file
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = 0;
        if (positionToBit[pieceIndex + pawnAdvace(isWhite)] & ~r.occupied) { // i.e. the square in front of the pawn is not occupied
            reachable |= positionToBit[pieceIndex + pawnAdvace(isWhite)] & r.pinmaskHV & r.checkMask; // i.e. the newposition is available only if it stays in the pinmask
        }
        reachable |= pawnAttacks(isWhite)[pieceIndex] & r.pinmaskD & enemy_n_checkMask;
        // std::cout << "NOT Starting file pinned pawns" << std::endl; display_int64(reachable);
        selfPiecesNew = self; selfPiecesNew.pawns ^= positionToBit[pieceIndex];

        BitLoop2(reachable) {
            Squares finalsquare = _blsi_u64(temp2);
            Pieces enemyPiecesNew = removePiece(enemy, finalsquare);

            if (finalsquare & pawnEndFile(isWhite)) { // pawn promotion
                // promote to queen
                selfPiecesNew.queens ^= finalsquare;
                if (isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !isWhite, (uint8_t) 0});
                else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !isWhite, (uint8_t) 0});
                selfPiecesNew.queens ^= finalsquare;

                // promote to rook
                selfPiecesNew.rooks ^= finalsquare;
                if (isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !isWhite, (uint8_t) 0});
                else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !isWhite, (uint8_t) 0});
                selfPiecesNew.rooks ^= finalsquare;

                // promote to bishop
                selfPiecesNew.bishops ^= finalsquare;
                if (isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !isWhite, (uint8_t) 0});
                else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !isWhite, (uint8_t) 0});
                selfPiecesNew.bishops ^= finalsquare;

                // promote to knight
                selfPiecesNew.knights ^= finalsquare;
                if (isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !isWhite, (uint8_t) 0});
                else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !isWhite, (uint8_t) 0});
                selfPiecesNew.knights ^= finalsquare;
            } else {
                selfPiecesNew.pawns ^= finalsquare;

                if (isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !isWhite, (uint8_t) 0});
                else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !isWhite, (uint8_t) 0});

                selfPiecesNew.pawns ^= finalsquare;
            }
        }
    }

    BitLoop(self.pawns & ~pinmask & pawnStartFile(isWhite)) { // starting file NON - pinned pawns
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = 0;

        selfPiecesNew = self; selfPiecesNew.pawns ^= positionToBit[pieceIndex];

        if (positionToBit[pieceIndex + pawnAdvace(isWhite)] & ~r.occupied) { // i.e. the square in front of the pawn is not occupied
            reachable |= positionToBit[pieceIndex + pawnAdvace(isWhite)] & r.checkMask; // i.e. the newposition is available only if it stays in the pinmask

            if (positionToBit[pieceIndex + 2 * pawnAdvace(isWhite)] & ~r.occupied & r.checkMask) { // i.e. 2 squares in front of pawn not occupied
                // reachable |= positionToBit[pieceIndex + 2 * pawnAdvace(isWhite)] & r.checkMask;

                // handle enpassant adding position different
                Squares finalsquare = positionToBit[pieceIndex + 2 * pawnAdvace(isWhite)];
                Pieces enemyPiecesNew = removePiece(enemy, finalsquare);
                selfPiecesNew.pawns ^= finalsquare;
                selfPiecesNew.enPassant = finalsquare >> (isWhite ? 24 : 32);

                if (isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !isWhite, (uint8_t) 0});
                else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !isWhite, (uint8_t) 0});

                selfPiecesNew.pawns ^= finalsquare;
                selfPiecesNew.enPassant = 0;
            }
        }
        reachable |= pawnAttacks(isWhite)[pieceIndex] & enemy_n_checkMask;
        // std::cout << "Starting file NON pinned pawns" << std::endl; display_int64(reachable);

        BitLoop2(reachable) {
            Squares finalsquare = _blsi_u64(temp2);
            Pieces enemyPiecesNew = removePiece(enemy, finalsquare);
            selfPiecesNew.pawns ^= finalsquare;

            if (isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !isWhite, (uint8_t) 0});
            else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !isWhite, (uint8_t) 0});

            selfPiecesNew.pawns ^= finalsquare;
        }
    }

    BitLoop(self.pawns & ~pinmask & ~pawnStartFile(isWhite)) { // non pinned pawns not in starting file
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = 0;
        if (positionToBit[pieceIndex + pawnAdvace(isWhite)] & ~r.occupied) { // i.e. the square in front of the pawn is not occupied
            reachable |= positionToBit[pieceIndex + pawnAdvace(isWhite)] & r.checkMask;
        }
        reachable |= pawnAttacks(isWhite)[pieceIndex] & enemy_n_checkMask;
        // std::cout << "NON Starting file NON pinned pawns" << std::endl; display_int64(reachable);
        selfPiecesNew = self; selfPiecesNew.pawns ^= positionToBit[pieceIndex];

        BitLoop2(reachable) {
            Squares finalsquare = _blsi_u64(temp2);
            Pieces enemyPiecesNew = removePiece(enemy, finalsquare);

            if (finalsquare & pawnEndFile(isWhite)) { // pawn promotion
                // promote to queen
                selfPiecesNew.queens ^= finalsquare;
                if (isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !isWhite, (uint8_t) 0});
                else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !isWhite, (uint8_t) 0});
                selfPiecesNew.queens ^= finalsquare;

                // promote to rook
                selfPiecesNew.rooks ^= finalsquare;
                if (isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !isWhite, (uint8_t) 0});
                else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !isWhite, (uint8_t) 0});
                selfPiecesNew.rooks ^= finalsquare;

                // promote to bishop
                selfPiecesNew.bishops ^= finalsquare;
                if (isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !isWhite, (uint8_t) 0});
                else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !isWhite, (uint8_t) 0});
                selfPiecesNew.bishops ^= finalsquare;

                // promote to knight
                selfPiecesNew.knights ^= finalsquare;
                if (isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !isWhite, (uint8_t) 0});
                else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !isWhite, (uint8_t) 0});
                selfPiecesNew.knights ^= finalsquare;
            } else {
                selfPiecesNew.pawns ^= finalsquare;

                if (isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !isWhite, (uint8_t) 0});
                else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !isWhite, (uint8_t) 0});

                selfPiecesNew.pawns ^= finalsquare;
            }
        }
    }

    if (enPassant) { // there is an enpassant pawn for the enemy
        Squares enemyEnPassant = (Squares) enPassant << (isWhite ? 32 : 24);
        BitLoop(self.pawns & pawnEP(isWhite) & ~r.pinmaskHV & ~r.enpassantpin) { // enpassant NON - ep-pinned pawns, non HV pinned
            uint64_t pieceIndex = bitFromSquare(temp);

            if (positionToBit[pieceIndex + 1] & enemyEnPassant) {
                // check if the taken piece is not D pinned
                if (enemyEnPassant & r.pinmaskD) continue;
                // self piece is part of pinD -> end square must be part of pinD
                if ((positionToBit[pieceIndex] & r.pinmaskD) && !(positionToBit[pieceIndex + pawnAdvace(isWhite) + 1] & r.pinmaskD)) continue;
                // std::cout << "Can take enpassant to square" << (int) pieceIndex + pawnAdvace(isWhite) + 1 << std::endl;

                selfPiecesNew = self; selfPiecesNew.pawns ^= positionToBit[pieceIndex];

                Squares finalsquare = positionToBit[pieceIndex + pawnAdvace(isWhite) + 1];
                Pieces enemyPiecesNew = removePiece(enemy, enemyEnPassant);
                selfPiecesNew.pawns ^= finalsquare;

                if (isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !isWhite, (uint8_t) 0});
                else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !isWhite, (uint8_t) 0});

            } 
            if (positionToBit[pieceIndex - 1] & enemyEnPassant) {
                // check if the taken piece is not D pinned
                if (enemyEnPassant & r.pinmaskD) continue;
                // self piece is part of pinD -> end square must be part of pinD
                if ((positionToBit[pieceIndex] & r.pinmaskD) && !(positionToBit[pieceIndex + pawnAdvace(isWhite) - 1] & r.pinmaskD)) continue;
                // std::cout << "Can take enpassant to square" << (int) pieceIndex + pawnAdvace(isWhite) - 1 << std::endl;

                selfPiecesNew = self; selfPiecesNew.pawns ^= positionToBit[pieceIndex];

                Squares finalsquare = positionToBit[pieceIndex + pawnAdvace(isWhite) - 1];
                Pieces enemyPiecesNew =removePiece(enemy, enemyEnPassant);
                selfPiecesNew.pawns ^= finalsquare;

                if (isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !isWhite, (uint8_t) 0});
                else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !isWhite, (uint8_t) 0});

            }
        }
    }

    if (self.castles & 1U && !r.checkCount) { // castles left
        if (!(r.occupied & castleLocc(isWhite)) && !(castleLcheck(isWhite) & r.enemySeen)) { // turret and king can see each other AND squares are not seen by enemy
            // std::cout << "Can CastleL" << std::endl;
            selfPiecesNew = self; selfPiecesNew.castles = 0;
            selfPiecesNew.king ^= (isWhite ? 0x0000000000000014ULL : 0x1400000000000000ULL);
            selfPiecesNew.rooks ^= (isWhite ? 0x0000000000000009ULL : 0x0900000000000000ULL);

            if (isWhite) newMoves.push_back({selfPiecesNew, enemy, !isWhite, (uint8_t) 0});
            else newMoves.push_back({enemy, selfPiecesNew, !isWhite, (uint8_t) 0});
        }
    }

    if (self.castles & 2U && !r.checkCount) { // castles right
        if (!(r.occupied & castleRcheck(isWhite)) && !(castleRcheck(isWhite) & r.enemySeen)) { // turret and king can see each other AND squares are not seen by enemy
            // std::cout << "Can CastleR" << std::endl;
            selfPiecesNew = self; selfPiecesNew.castles = 0;
            selfPiecesNew.king ^= (isWhite ? 0x0000000000000050ULL : 0x5000000000000000ULL);
            selfPiecesNew.rooks ^= (isWhite ? 0x00000000000000a0ULL : 0xa000000000000000ULL);

            if (isWhite) newMoves.push_back({selfPiecesNew, enemy, !isWhite, (uint8_t) 0});
            else newMoves.push_back({enemy, selfPiecesNew, !isWhite, (uint8_t) 0});
        }
    }

    if (newMoves.size() == 0) {
        if (isWhite) newMoves.push_back({self, enemy, isWhite, (uint8_t) (r.checkCount ? 0b1 : 0b10)});
        else newMoves.push_back({enemy, self, isWhite, (uint8_t) (r.checkCount ? 0b1 : 0b10)});
        return newMoves;
    }
    return newMoves;
}

uint64_t traverse(const Board &initialPosition, int depth) {
    if (depth==1) {
        if (initialPosition.isWhite) {
            auto moves = generateMoves<true>(initialPosition);
            return moves.size();
        } else {
            auto moves = generateMoves<false>(initialPosition);
            return moves.size();
        }
    }

    uint64_t initialMoveCount = 0;

    if (initialPosition.isWhite) {
        auto initialmoves = generateMoves<true>(initialPosition);

        for (auto &position : initialmoves) {
            // std::cout << boardToStr(position) << std::endl;
            if (!position.gameResult) initialMoveCount += traverse(position, depth-1);
            else initialMoveCount++;
        }

        return initialMoveCount;
    }
    else {
        auto initialmoves = generateMoves<false>(initialPosition);

        for (auto &position : initialmoves) {
            // std::cout << boardToStr(position) << std::endl;
            if (!position.gameResult) initialMoveCount += traverse(position, depth-1);
            else initialMoveCount++;
        }

        return initialMoveCount;
    }
}


int main(void) {
    // Board a = positionToBoard("RNBQKBNR/PPPP1PPP/8/8/4P3/p6n/1ppppppp/rnbqkb1r/-/W");
    // a.isWhite = false;

    // Board a = positionToBoard("4RK2/q3B3/5P1Q/4N3/8/2p3r1/pR3pp1/r5k1/-/W"); // r5k1/pR3pp1/2p3r1/8/4N3/5P1Q/q3B3/4RK2 b - - 2 29
    // a.isWhite = false; a.whitePieces.castles = 0; a.blackPieces.castles = 0;

    // Board a = positionToBoard("4RK2/q3B3/5P1Q/4N3/8/2p3r1/pR3pp1/1r4k1/-/W"); // r5k1/pR3pp1/2p3r1/8/4N3/5P1Q/q3B3/4RK2 b - - 2 29
    // a.isWhite = true; a.whitePieces.castles = 0; a.blackPieces.castles = 0;

    // Board a = positionToBoard("4RK2/q3B3/5P1Q/4N3/8/2p3r1/p4Rp1/1r4k1/-/W"); // r5k1/pR3pp1/2p3r1/8/4N3/5P1Q/q3B3/4RK2 b - - 2 29
    // a.isWhite = false; a.whitePieces.castles = 0; a.blackPieces.castles = 0;

    // Board a = positionToBoard("4RK2/13B3/5P1Q/4N3/8/2p3r1/p4qp1/1r4k1/-/W"); // r5k1/pR3pp1/2p3r1/8/4N3/5P1Q/q3B3/4RK2 b - - 2 29
    // a.isWhite = true; a.whitePieces.castles = 0; a.blackPieces.castles = 0;

    // Board a = positionToBoard("4RK2/8/5P11/2B1N3/2p5/211Q1r1/p4qp1/1r4k1/-/W"); // r5k1/pR3pp1/2p3r1/8/4N3/5P1Q/q3B3/4RK2 b - - 2 29
    // a.isWhite = false; a.whitePieces.castles = 0; a.blackPieces.castles = 0;

    // b8d8
    Board a = positionToBoard("4RK2/8/5P2/2B1N3/2p5/6r1/P4qp1/2Qr2k1/-/W"); // r5k1/pR3pp1/2p3r1/8/4N3/5P1Q/q3B3/4RK2 b - - 2 29
    a.isWhite = true; a.whitePieces.castles = 0; a.blackPieces.castles = 0;

    // Board a = positionToBoard("4RK2/q3B3/5P1Q/4N3/2p5/213r1/pR3pp1/r5k1/-/W"); // r5k1/pR3pp1/2p3r1/8/4N3/5P1Q/q3B3/4RK2 b - - 2 29
    // a.isWhite = true; a.whitePieces.castles = 0; a.blackPieces.castles = 0;

    Board b = positionToBoard("RNBQKBNR/PPPPPPPP/8/8/8/8/pppppppp/rnbqkbnr/-/W");
    // // Board b = positionToBoard("4K3/3p4/8/8/8/8/8/k7/-/W");
    b.isWhite = true;

    // Board b = positionToBoard("RNBQKBNR/1PPPPPPP/8/8/P16/n7/pppppppp/r1bqkbnr/-/W");
    // // // Board b = positionToBoard("4K3/3p4/8/8/8/8/8/k7/-/W");
    // b.isWhite = false; //b.blackPieces.enPassant = 0b00000010;

    std::cout << boardToStr(b) << std::endl;

    auto start_time = std::chrono::high_resolution_clock::now();

    std::cout << "Depth 1: " << (unsigned long) traverseWithMoves(b, 1) << std::endl;
    std::cout << "Depth 2: " << (unsigned long) traverseWithMoves(b, 2) << std::endl;
    std::cout << "Depth 3: " << (unsigned long) traverseWithMoves(b, 3) << std::endl;
    std::cout << "Depth 4: " << (unsigned long) traverseWithMoves(b, 4) << std::endl;
    std::cout << "Depth 5: " << (unsigned long) traverseWithMoves(b, 5) << std::endl;
    std::cout << "Depth 6: " << (unsigned long) traverseWithMoves(b, 6) << std::endl;
    std::cout << "Depth 7: " << (unsigned long) traverseWithMoves(b, 7) << std::endl;

    // std::cout << "Depth 1: " << (unsigned long) traverse(b, 1) << std::endl;
    // std::cout << "Depth 2: " << (unsigned long) traverse(b, 2) << std::endl;
    // std::cout << "Depth 3: " << (unsigned long) traverse(b, 3) << std::endl;
    // std::cout << "Depth 4: " << (unsigned long) traverse(b, 4) << std::endl;
    // std::cout << "Depth 5: " << (unsigned long) traverse(b, 5) << std::endl;
    // std::cout << "Depth 6: " << (unsigned long) traverse(b, 6) << std::endl;
    // std::cout << "Depth 7: " << (unsigned long) traverse(b, 7) << std::endl;
    // Board b = positionToBoard("RNB1KBNR/PPPPPPPP/413/8/6B1/51N1/ppppkppp/rnbq1bnr/-/W");
    // enpassant pinned
    // Board b = positionToBoard("K1R5/P15P/8/QPpP3k/2P5/7b/6p1/1r1q4/-/W");
    // Board b = positionToBoard("K1R5/PP5P/8/8/Q2r3k/7b/6p1/1r1q4/-/W");
    // Board b = positionToBoard("K1R5/PP5P/7P/7P/Q213k/7b/5np1/1r1q4/-/W");


    // isInCheckResult r = isInCheck(b.blackPieces, b.whitePieces, false);

    // std::cout << "White starts: " << traverse(b, 4) << std::endl;
    // b.isWhite = false;
    // for (int i=0; i<10000000; i++) {
    //     auto moves = generateMoves(b);
    // }
    // std::cout << "Depth 1: " << traverse(a, 1) << std::endl;
    // std::cout << "Depth 2: " << traverse(a, 2) << std::endl;
    // std::cout << "Depth 3: " << traverse(a, 3) << std::endl;
    // std::cout << "Depth 4: " << traverse(a, 4) << std::endl;
    // std::cout << "Depth 5: " << traverse(a, 5) << std::endl;
    // std::cout << "Depth 6: " << traverse(a, 6) << std::endl;
    auto end_time = std::chrono::high_resolution_clock::now();

    // auto moves = generateMovesShort<false>(b);
    // std::cout << moves[6].movedPiece << std::endl;
    // std::cout << (int)moves[6].enpassant << std::endl;
    // // auto b2 = applyMove(b, moves[0]);
    // // std::cout << boardToStr(b2) << std::endl;
    // // if (!b2.isWhite) std::cout << "Black" << std::endl;
    // // auto moves2 = generateMovesShort<false>(b2);

    // for (auto pos : moves) {
    //     std::cout << traverseWithMoves(applyMove(b, pos), 1) << std::endl;
    //     std::cout << boardToStr(applyMove(b, pos)) << std::endl;
    //     // std::cout << (int)((pos.movedPiece & 0b1110)>>1) << " " << (int)((pos.movedPiece & 0b1111110000)>>4) << " " << (int)(pos.movedPiece>>10) << std::endl;
    // }
    // // std::cout << std::endl;
    // std::cout << moves.size() << std::endl;
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    std::cout << "Execution time: " << duration.count() << " microseconds" << std::endl;
    return 0;
}