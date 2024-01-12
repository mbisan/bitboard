#include "display.cpp"

#include <chrono>

Squares occupied(const Pieces &pieces) {
    return (pieces.pawns|pieces.rooks|pieces.bishops|pieces.knights|pieces.queens|pieces.king);
}

Squares reachableNotBlocked(Squares reachableIfNotBlocked, Squares occupied, uint64_t pieceIndex) {
    BitLoop(reachableIfNotBlocked & occupied) {
        reachableIfNotBlocked &= attacking[pieceIndex][bitFromSquare(temp)];
    }
    return reachableIfNotBlocked;
}

Squares Seen(const Pieces &selfPieces, const Pieces &enemyPieces, bool isWhite) {

    Squares enemySquares = occupied(enemyPieces);
    Squares selfSquares = occupied(selfPieces);
    Squares occupiedSquares = enemySquares | selfSquares;

    Squares enemySeen = 0;

    BitLoop(enemyPieces.rooks | enemyPieces.queens) {
        uint64_t pieceIndex = bitFromSquare(temp);
        // Squares rookReachable = rookMoves[pieceIndex];
        // for(uint64_t temp2 = rookReachable & occupiedSquares; temp2; temp2 = _blsr_u64(temp2)) {
        //     rookReachable &= attacking[pieceIndex][bitFromSquare(temp2)];
        // }
        enemySeen |= reachableNotBlocked(rookMoves[pieceIndex], occupiedSquares, pieceIndex);
    }

    BitLoop(enemyPieces.bishops | enemyPieces.queens) {
        uint64_t pieceIndex = bitFromSquare(temp);
        // Squares bishopReachable = bishopMoves[pieceIndex];
        // for(uint64_t temp2 = bishopReachable & occupiedSquares; temp2; temp2 = _blsr_u64(temp2)) {
        //     bishopReachable &= attacking[pieceIndex][bitFromSquare(temp2)];
        // }
        enemySeen |= reachableNotBlocked(bishopMoves[pieceIndex], occupiedSquares, pieceIndex);
    }

    BitLoop(enemyPieces.knights) {
        enemySeen |= knightMoves[bitFromSquare(temp)];
    }

    BitLoop(enemyPieces.pawns) {
        enemySeen |= pawnAttacks(!isWhite)[bitFromSquare(temp)];
    }

    BitLoop(enemyPieces.king) {
        enemySeen |= kingMoves[bitFromSquare(temp)];
    }

    return enemySeen;
}

isInCheckResult isInCheck(const Pieces &selfPieces, const Pieces &enemyPieces, bool isWhite) {

    Squares enemySquares = occupied(enemyPieces);
    Squares selfSquares = occupied(selfPieces);
    Squares occupiedSquares = enemySquares | selfSquares;

    Squares occupiedNotKing = occupiedSquares ^ selfPieces.king;
    Squares selfNotKing = selfSquares ^ selfPieces.king; // all self pieces that are not the king

    bit kingIndex = bitFromSquare(selfPieces.king);
    int kingFile = kingIndex/8;

    int checkcount = 0;

    Squares checkMask = 0;
    Squares pinmaskHV = 0;
    Squares pinmaskD = 0;
    Squares enpassantpin = 0;

    BitLoop(enemyPieces.rooks | enemyPieces.queens) {
        uint64_t pieceIndex = bitFromSquare(temp);
        if (rookMoves[pieceIndex] & selfPieces.king) { // i.e. the enemy rook/queen can view the king (if the file/column is empty)

            //    path from king to enemy (inc)       exclude enemy
            Squares inbetween = (PinBetween[kingIndex][pieceIndex] ^ positionToBit[pieceIndex]) & occupiedNotKing;

            if (!inbetween) { // i.e. the enemy rook is not blocked by any piece
                checkMask |= PinBetween[kingIndex][pieceIndex];
                checkcount++;
            } else { // there are pieces in the way
                Squares inbetweenPieces = inbetween & occupiedSquares;
                Squares onePieceRemoved = _blsr_u64(inbetweenPieces);
                if (!onePieceRemoved) { // i.e. the enemy rook/queen is blocked by a single piece, then this piece is pinned
                    pinmaskHV |= PinBetween[kingIndex][pieceIndex];
                } else if (!_blsr_u64(onePieceRemoved)) { // there are two pieces in the way, therefore the only way for a piece to be pinned is that the piece is an enpassant pawn
                    if (enemyPieces.enPassant && (int)pieceIndex/8 == kingFile && inbetweenPieces & selfPieces.pawns) { 
                        // there is enpassant pawn for enemy on the same file as the king AND the other piece is a self pawn
                        enpassantpin = inbetweenPieces & selfPieces.pawns; // i.e. mask the self pawn so that it cannot take enpassant
                    }
                }
            }
        }
    }

    BitLoop(enemyPieces.bishops | enemyPieces.queens) {
        uint64_t pieceIndex = bitFromSquare(temp);
        if (bishopMoves[pieceIndex] & selfPieces.king) { // i.e. the enemy bishop/queen can view the king (if the diag is empty)

            Squares inbetween = (PinBetween[kingIndex][pieceIndex] ^ positionToBit[pieceIndex]) & occupiedNotKing;

            if (!inbetween) { // i.e. the enemy bishop is not blocked by any of our pieces
                checkMask |= PinBetween[kingIndex][pieceIndex];
                checkcount++;
            } else {
                Squares inBetweenEnemy = inbetween & enemySquares;
                if (!inBetweenEnemy) { // i.e. the enemy bishop/queen is blocked only by self pieces
                    if (!_blsr_u64(inbetween & selfSquares)) { // there is only ONE self piece in between
                        pinmaskD |= PinBetween[kingIndex][pieceIndex];
                    }
                }
            }
        }
    }

    BitLoop(enemyPieces.knights) {
        uint64_t pieceIndex = bitFromSquare(temp);
        if (knightMoves[pieceIndex] & selfPieces.king) { // i.e. the enemy knight can reach our king
            Squares temp = (knightMoves[kingIndex] & enemyPieces.knights);
            checkMask |= temp;
            checkcount++;
        }
    }

    BitLoop(enemyPieces.pawns) {
        uint64_t pieceIndex = bitFromSquare(temp);
        if (pawnAttacks(!isWhite)[pieceIndex] & selfPieces.king) { // i.e. pawn attacking our king
            checkMask |= positionToBit[pieceIndex];
            checkcount++;
        }
    }

    return {checkcount, checkMask, 0, pinmaskHV, pinmaskD, enpassantpin};
}

Pieces removePiece(Pieces pieces, Squares removepositions) {
    Squares n_remove = ~removepositions;
    pieces.bishops &= n_remove; pieces.rooks &= n_remove; pieces.queens &= n_remove; 
    pieces.knights &= n_remove; pieces.pawns &= n_remove;
    return pieces;
}

std::vector<Board> generateMoves(const Board board) {
    std::vector<Board> newMoves;

    const Pieces *self, *enemy;

    if (board.isWhite) {
        self = &board.whitePieces;
        enemy = &board.blackPieces;
    } else {
        self = &board.blackPieces;
        enemy = &board.whitePieces;
    }

    // generate pinmask and checkmask
    isInCheckResult r = isInCheck(*self, *enemy, board.isWhite); 
    Squares pinmask = r.pinmaskD | r.pinmaskHV;
    Squares enemySeen = Seen(*self, *enemy, board.isWhite);
    // Squares selfSeen = Seen(*enemy, *self, !board.isWhite);

    Squares selfPieces = occupied(*self);
    Squares notSelf = ~selfPieces;
    Squares enemyPieces = occupied(*enemy);
    Squares alloccupied = selfPieces | enemyPieces;

    uint64_t kingIndex = bitFromSquare(self->king);

    if (r.checkCount > 1) { // double check forces king to move
        // king can only move to squares not seen by enemy and not occupied by enemy pieces
        Squares kingReachable = kingMoves[kingIndex] & notSelf & ~enemySeen;
        Pieces selfPiecesNew = *self; selfPiecesNew.king ^= self->king; selfPiecesNew.castles = 0;

        BitLoop(kingReachable) {
            Squares finalsquare = _blsi_u64(temp);
            Pieces enemyPiecesNew = removePiece(*enemy, finalsquare);
            selfPiecesNew.king ^= finalsquare;
            
            if (board.isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !board.isWhite, (uint8_t) 0});
            else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !board.isWhite, (uint8_t) 0});

            selfPiecesNew.king ^= finalsquare;
        }

        if (newMoves.size() == 0) {
            if (board.isWhite) newMoves.push_back({*self, *enemy, board.isWhite, (uint8_t) (r.checkCount ? 0b1 : 0b10)});
            else newMoves.push_back({*enemy, *self, board.isWhite, (uint8_t) (r.checkCount ? 0b1 : 0b10)});
            return newMoves;
        }
        return newMoves;
    } 
    if (r.checkCount == 0) r.checkMask = 0xffffffffffffffff; // if no checks are given, then all squares are available for all pieces

    Pieces selfPiecesNew;  

    // generate king moves
    Squares kingReachable = kingMoves[kingIndex] & notSelf & ~enemySeen;
    // std::cout << "KingMoves" << std::endl; display_int64(kingReachable);
    selfPiecesNew = *self; selfPiecesNew.king ^= self->king; selfPiecesNew.castles = 0;
    BitLoop(kingReachable) {
        Squares finalsquare = _blsi_u64(temp);
        Pieces enemyPiecesNew = removePiece(*enemy, finalsquare);
        selfPiecesNew.king ^= finalsquare;

        if (board.isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !board.isWhite, (uint8_t) 0});
        else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !board.isWhite, (uint8_t) 0});

        selfPiecesNew.king ^= finalsquare;
    }

    // generate rook/queen moves
    
    // non-pinned rooks
    BitLoop(self->rooks & ~pinmask) {
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = reachableNotBlocked(rookMoves[pieceIndex], alloccupied, pieceIndex) & notSelf & r.checkMask;
        // std::cout << "Non-Pinned rooks" << std::endl; display_int64(reachable);
        selfPiecesNew = *self; selfPiecesNew.rooks ^= positionToBit[pieceIndex];
        
        if (pieceIndex == (board.isWhite ? 0 : 56)) selfPiecesNew.castles &= 0b10; // moving L Rook so cannot L castle
        if (pieceIndex == (board.isWhite ? 7 : 63)) selfPiecesNew.castles &= 0b01; // moving R Rook so cannot R castle

        BitLoop2(reachable) {
            Squares finalsquare = _blsi_u64(temp2);
            Pieces enemyPiecesNew = removePiece(*enemy, finalsquare);
            selfPiecesNew.rooks ^= finalsquare;

            if (board.isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !board.isWhite, (uint8_t) 0});
            else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !board.isWhite, (uint8_t) 0});

            selfPiecesNew.rooks ^= finalsquare;
        }
    }

    // non-pinned queens
    BitLoop(self->queens & ~pinmask) {
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = reachableNotBlocked(rookMoves[pieceIndex], alloccupied, pieceIndex) & notSelf & r.checkMask;
        // std::cout << "Non-Pinned rooks" << std::endl; display_int64(reachable);
        selfPiecesNew = *self; selfPiecesNew.queens ^= positionToBit[pieceIndex];
        
        BitLoop2(reachable) {
            Squares finalsquare = _blsi_u64(temp2);
            Pieces enemyPiecesNew = removePiece(*enemy, finalsquare);
            selfPiecesNew.queens ^= finalsquare;

            if (board.isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !board.isWhite, (uint8_t) 0});
            else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !board.isWhite, (uint8_t) 0});

            selfPiecesNew.queens ^= finalsquare;
        }
    }

    // pinned rooks
    BitLoop(self->rooks & pinmask) {
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = reachableNotBlocked(rookMoves[pieceIndex], alloccupied, pieceIndex) & r.pinmaskHV & notSelf & r.checkMask;
        // std::cout << "Pinned rooks" << std::endl; display_int64(reachable);
        selfPiecesNew = *self; selfPiecesNew.rooks ^= positionToBit[pieceIndex];
        
        if (pieceIndex == (board.isWhite ? 0 : 56)) selfPiecesNew.castles &= 0b10; // moving L Rook so cannot L castle
        if (pieceIndex == (board.isWhite ? 7 : 63)) selfPiecesNew.castles &= 0b01; // moving R Rook so cannot R castle

        BitLoop2(reachable) {
            Squares finalsquare = _blsi_u64(temp2);
            Pieces enemyPiecesNew = removePiece(*enemy, finalsquare);
            selfPiecesNew.rooks ^= finalsquare;

            if (board.isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !board.isWhite, (uint8_t) 0});
            else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !board.isWhite, (uint8_t) 0});

            selfPiecesNew.rooks ^= finalsquare;
        }
    }

    // pinned queens
    BitLoop(self->queens & pinmask) {
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = reachableNotBlocked(rookMoves[pieceIndex], alloccupied, pieceIndex) & r.pinmaskHV & notSelf & r.checkMask;
        // std::cout << "Pinned rooks" << std::endl; display_int64(reachable);
        selfPiecesNew = *self; selfPiecesNew.queens ^= positionToBit[pieceIndex];
        
        BitLoop2(reachable) {
            Squares finalsquare = _blsi_u64(temp2);
            Pieces enemyPiecesNew = removePiece(*enemy, finalsquare);
            selfPiecesNew.queens ^= finalsquare;

            if (board.isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !board.isWhite, (uint8_t) 0});
            else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !board.isWhite, (uint8_t) 0});

            selfPiecesNew.queens ^= finalsquare;
        }
    }

    // generate bishop/queen moves
    
    // non-pinned bishops
    BitLoop(self->bishops & ~pinmask) {
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = reachableNotBlocked(bishopMoves[pieceIndex], alloccupied, pieceIndex) & notSelf & r.checkMask;;
        // std::cout << "Non-pinned bishops" << std::endl; display_int64(reachable);
        selfPiecesNew = *self; selfPiecesNew.bishops ^= positionToBit[pieceIndex];

        BitLoop2(reachable) {
            Squares finalsquare = _blsi_u64(temp2);
            Pieces enemyPiecesNew = removePiece(*enemy, finalsquare);
            selfPiecesNew.bishops ^= finalsquare;

            if (board.isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !board.isWhite, (uint8_t) 0});
            else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !board.isWhite, (uint8_t) 0});

            selfPiecesNew.bishops ^= finalsquare;
        }
    }

    // non-pinned queens
    BitLoop(self->queens & ~pinmask) {
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = reachableNotBlocked(bishopMoves[pieceIndex], alloccupied, pieceIndex) & notSelf & r.checkMask;;
        // std::cout << "Non-pinned bishops" << std::endl; display_int64(reachable);
        selfPiecesNew = *self; selfPiecesNew.queens ^= positionToBit[pieceIndex];

        BitLoop2(reachable) {
            Squares finalsquare = _blsi_u64(temp2);
            Pieces enemyPiecesNew = removePiece(*enemy, finalsquare);
            selfPiecesNew.queens ^= finalsquare;

            if (board.isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !board.isWhite, (uint8_t) 0});
            else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !board.isWhite, (uint8_t) 0});

            selfPiecesNew.queens ^= finalsquare;
        }
    }

    // pinned bishops
    BitLoop((self->bishops | self->queens) & pinmask) {
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = reachableNotBlocked(bishopMoves[pieceIndex], alloccupied, pieceIndex) & r.pinmaskD & notSelf & r.checkMask;
        // std::cout << "Pinned bishops" << std::endl; display_int64(reachable);
        selfPiecesNew = *self; selfPiecesNew.bishops ^= positionToBit[pieceIndex];

        BitLoop2(reachable) {
            Squares finalsquare = _blsi_u64(temp2);
            Pieces enemyPiecesNew = removePiece(*enemy, finalsquare);
            selfPiecesNew.bishops ^= finalsquare;

            if (board.isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !board.isWhite, (uint8_t) 0});
            else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !board.isWhite, (uint8_t) 0});

            selfPiecesNew.bishops ^= finalsquare;
        }
    }

    // pinned queens
    BitLoop(self->queens & pinmask) {
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = reachableNotBlocked(bishopMoves[pieceIndex], alloccupied, pieceIndex) & r.pinmaskD & notSelf & r.checkMask;
        // std::cout << "Pinned bishops" << std::endl; display_int64(reachable);
        selfPiecesNew = *self; selfPiecesNew.queens ^= positionToBit[pieceIndex];

        BitLoop2(reachable) {
            Squares finalsquare = _blsi_u64(temp2);
            Pieces enemyPiecesNew = removePiece(*enemy, finalsquare);
            selfPiecesNew.queens ^= finalsquare;

            if (board.isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !board.isWhite, (uint8_t) 0});
            else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !board.isWhite, (uint8_t) 0});

            selfPiecesNew.queens ^= finalsquare;
        }
    }

    // generate knight moves
    BitLoop(self->knights & ~pinmask) { // pinned knight cannot move, so only consider unpinned knights
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = knightMoves[pieceIndex] & notSelf & r.checkMask;
        // std::cout << "Non-pinned Knights" << std::endl; display_int64(reachable);
        selfPiecesNew = *self; selfPiecesNew.knights ^= positionToBit[pieceIndex];

        BitLoop2(reachable) {
            Squares finalsquare = _blsi_u64(temp2);
            Pieces enemyPiecesNew = removePiece(*enemy, finalsquare);
            selfPiecesNew.knights ^= finalsquare;

            if (board.isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !board.isWhite, (uint8_t) 0});
            else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !board.isWhite, (uint8_t) 0});

            selfPiecesNew.knights ^= finalsquare;
        }
    }

    // PAWNS
    BitLoop(self->pawns & pinmask & pawnStartFile(board.isWhite)) { // starting file pinned pawns
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = 0;
        if (positionToBit[pieceIndex + pawnAdvace(board.isWhite)] & ~alloccupied) { // i.e. the square in front of the pawn is not occupied
            reachable |= positionToBit[pieceIndex + pawnAdvace(board.isWhite)] & r.pinmaskHV & r.checkMask; // i.e. the newposition is available only if it stays in the pinmask

            if (positionToBit[pieceIndex + 2 * pawnAdvace(board.isWhite)] & ~alloccupied) { // i.e. 2 squares in front of pawn not occupied
                reachable |= positionToBit[pieceIndex + 2 * pawnAdvace(board.isWhite)] & r.pinmaskHV & r.checkMask;
            }

        }
        reachable |= pawnAttacks(board.isWhite)[pieceIndex] & r.pinmaskD & r.checkMask & enemyPieces;
        // std::cout << "Starting file pinned pawns" << std::endl; display_int64(reachable);
        selfPiecesNew = *self; selfPiecesNew.pawns ^= positionToBit[pieceIndex];

        BitLoop2(reachable) {
            Squares finalsquare = _blsi_u64(temp2);
            Pieces enemyPiecesNew = removePiece(*enemy, finalsquare);
            selfPiecesNew.pawns ^= finalsquare;

            if (board.isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !board.isWhite, (uint8_t) 0});
            else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !board.isWhite, (uint8_t) 0});

            selfPiecesNew.pawns ^= finalsquare;
        }
    }

    BitLoop(self->pawns & pinmask & ~pawnStartFile(board.isWhite)) { // pinned pawns not in starting file
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = 0;
        if (positionToBit[pieceIndex + pawnAdvace(board.isWhite)] & ~alloccupied) { // i.e. the square in front of the pawn is not occupied
            reachable |= positionToBit[pieceIndex + pawnAdvace(board.isWhite)] & r.pinmaskHV & r.checkMask; // i.e. the newposition is available only if it stays in the pinmask
        }
        reachable |= pawnAttacks(board.isWhite)[pieceIndex] & r.pinmaskD & r.checkMask & enemyPieces;
        // std::cout << "NOT Starting file pinned pawns" << std::endl; display_int64(reachable);
        selfPiecesNew = *self; selfPiecesNew.pawns ^= positionToBit[pieceIndex];

        BitLoop2(reachable) {
            Squares finalsquare = _blsi_u64(temp2);
            Pieces enemyPiecesNew = removePiece(*enemy, finalsquare);
            selfPiecesNew.pawns ^= finalsquare;

            if (board.isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !board.isWhite, (uint8_t) 0});
            else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !board.isWhite, (uint8_t) 0});

            selfPiecesNew.pawns ^= finalsquare;
        }
    }

    BitLoop(self->pawns & ~pinmask & pawnStartFile(board.isWhite)) { // starting file NON - pinned pawns
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = 0;
        if (positionToBit[pieceIndex + pawnAdvace(board.isWhite)] & ~alloccupied) { // i.e. the square in front of the pawn is not occupied
            reachable |= positionToBit[pieceIndex + pawnAdvace(board.isWhite)] & r.checkMask; // i.e. the newposition is available only if it stays in the pinmask

            if (positionToBit[pieceIndex + 2 * pawnAdvace(board.isWhite)] & ~alloccupied) { // i.e. 2 squares in front of pawn not occupied
                reachable |= positionToBit[pieceIndex + 2 * pawnAdvace(board.isWhite)] & r.checkMask;
            }

        }
        reachable |= pawnAttacks(board.isWhite)[pieceIndex] & r.checkMask & enemyPieces;
        // std::cout << "Starting file NON pinned pawns" << std::endl; display_int64(reachable);
        selfPiecesNew = *self; selfPiecesNew.pawns ^= positionToBit[pieceIndex];

        BitLoop2(reachable) {
            Squares finalsquare = _blsi_u64(temp2);
            Pieces enemyPiecesNew = removePiece(*enemy, finalsquare);
            selfPiecesNew.pawns ^= finalsquare;

            if (board.isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !board.isWhite, (uint8_t) 0});
            else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !board.isWhite, (uint8_t) 0});

            selfPiecesNew.pawns ^= finalsquare;
        }
    }

    BitLoop(self->pawns & ~pinmask & ~pawnStartFile(board.isWhite)) { // non pinned pawns not in starting file
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares reachable = 0;
        if (positionToBit[pieceIndex + pawnAdvace(board.isWhite)] & ~alloccupied) { // i.e. the square in front of the pawn is not occupied
            reachable |= positionToBit[pieceIndex + pawnAdvace(board.isWhite)] & r.checkMask;
        }
        reachable |= pawnAttacks(board.isWhite)[pieceIndex] & r.checkMask & enemyPieces;
        // std::cout << "NON Starting file NON pinned pawns" << std::endl; display_int64(reachable);
        selfPiecesNew = *self; selfPiecesNew.pawns ^= positionToBit[pieceIndex];

        BitLoop2(reachable) {
            Squares finalsquare = _blsi_u64(temp2);
            Pieces enemyPiecesNew = removePiece(*enemy, finalsquare);
            selfPiecesNew.pawns ^= finalsquare;

            if (board.isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !board.isWhite, (uint8_t) 0});
            else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !board.isWhite, (uint8_t) 0});

            selfPiecesNew.pawns ^= finalsquare;
        }
    }

    if (enemy->enPassant) { // there is an enpassant pawn for the enemy
        Squares enemyEnPassant = (Squares) enemy->enPassant << (board.isWhite ? 32 : 24);
        BitLoop(self->pawns & ~r.enpassantpin & pawnEP(board.isWhite) & ~r.pinmaskHV) { // enpassant NON - ep-pinned pawns, non HV pinned
            uint64_t pieceIndex = bitFromSquare(temp); int piececol = pieceIndex % 8;

            if (positionToBit[pieceIndex + 1] & enemyEnPassant) {
                // check if the taken piece is not D pinned
                if (enemyEnPassant & r.pinmaskD) continue;
                // self piece is part of pinD -> end square must be part of pinD
                if ((positionToBit[pieceIndex] & r.pinmaskD) && !(positionToBit[pieceIndex + pawnAdvace(board.isWhite) + 1] & r.pinmaskD)) continue;
                // std::cout << "Can take enpassant to square" << (int) pieceIndex + pawnAdvace(board.isWhite) + 1 << std::endl;

                selfPiecesNew = *self; selfPiecesNew.pawns ^= positionToBit[pieceIndex];
                Pieces enemyPiecesNew = *enemy;

                Squares finalsquare = positionToBit[pieceIndex + pawnAdvace(board.isWhite) + 1];
                removePiece(enemyPiecesNew, enemyEnPassant);
                selfPiecesNew.pawns ^= finalsquare;

                if (board.isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !board.isWhite, (uint8_t) 0});
                else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !board.isWhite, (uint8_t) 0});

            } else if (positionToBit[pieceIndex - 1] & enemyEnPassant) {
                // check if the taken piece is not D pinned
                if (enemyEnPassant & r.pinmaskD) continue;
                // self piece is part of pinD -> end square must be part of pinD
                if ((positionToBit[pieceIndex] & r.pinmaskD) && !(positionToBit[pieceIndex + pawnAdvace(board.isWhite) - 1] & r.pinmaskD)) continue;
                // std::cout << "Can take enpassant to square" << (int) pieceIndex + pawnAdvace(board.isWhite) - 1 << std::endl;

                selfPiecesNew = *self; selfPiecesNew.pawns ^= positionToBit[pieceIndex];
                Pieces enemyPiecesNew = *enemy;

                Squares finalsquare = positionToBit[pieceIndex + pawnAdvace(board.isWhite) - 1];
                removePiece(enemyPiecesNew, enemyEnPassant);
                selfPiecesNew.pawns ^= finalsquare;

                if (board.isWhite) newMoves.push_back({selfPiecesNew, enemyPiecesNew, !board.isWhite, (uint8_t) 0});
                else newMoves.push_back({enemyPiecesNew, selfPiecesNew, !board.isWhite, (uint8_t) 0});

            }
        }
    }

    if (self->castles & 1U && !r.checkCount) { // castles left
        if (!(alloccupied & castleLocc(board.isWhite)) && !(castleLcheck(board.isWhite) & enemySeen)) { // turret and king can see each other AND squares are not seen by enemy
            // std::cout << "Can CastleL" << std::endl;
            selfPiecesNew = *self; selfPiecesNew.castles = 0;
            selfPiecesNew.king ^= (board.isWhite ? 0x0000000000000014ULL : 0x1400000000000000ULL);
            selfPiecesNew.rooks ^= (board.isWhite ? 0x0000000000000009ULL : 0x0900000000000000ULL);

            if (board.isWhite) newMoves.push_back({selfPiecesNew, *enemy, !board.isWhite, (uint8_t) 0});
            else newMoves.push_back({*enemy, selfPiecesNew, !board.isWhite, (uint8_t) 0});
        }
    }

    if (self->castles & 2U && !r.checkCount) { // castles right
        if (!(alloccupied & castleRcheck(board.isWhite)) && !(castleRcheck(board.isWhite) & enemySeen)) { // turret and king can see each other AND squares are not seen by enemy
            // std::cout << "Can CastleR" << std::endl;
            selfPiecesNew = *self; selfPiecesNew.castles = 0;
            selfPiecesNew.king ^= (board.isWhite ? 0x0000000000000050ULL : 0x5000000000000000ULL);
            selfPiecesNew.rooks ^= (board.isWhite ? 0x00000000000000a0ULL : 0xa000000000000000ULL);

            if (board.isWhite) newMoves.push_back({selfPiecesNew, *enemy, !board.isWhite, (uint8_t) 0});
            else newMoves.push_back({*enemy, selfPiecesNew, !board.isWhite, (uint8_t) 0});
        }
    }

    // TODO promotion

    if (newMoves.size() == 0) {
        if (board.isWhite) newMoves.push_back({*self, *enemy, board.isWhite, (uint8_t) (r.checkCount ? 0b1 : 0b10)});
        else newMoves.push_back({*enemy, *self, board.isWhite, (uint8_t) (r.checkCount ? 0b1 : 0b10)});
        return newMoves;
    }
    return newMoves;
}

int traverse(const Board &initialPosition, int depth) {
    if (depth==0) return 1;

    int initialMoveCount = 0;

    auto initialmoves = generateMoves(initialPosition);

    for (auto position : initialmoves) {
        // std::cout << boardToStr(position) << std::endl;
        initialMoveCount += traverse(position, depth-1);
        if (position.gameResult > 0) std::cout << boardToStr(position) << std::endl;
    }

    return initialMoveCount;
}


int main(void) {

    Board b = positionToBoard("RNBQKBNR/PPPPPPPP/8/8/8/8/pppppppp/rnbqkbnr/-/W");
    // Board b = positionToBoard("4K3/3p4/8/8/8/8/8/k7/-/W");
    b.isWhite = true;
    // Board b = positionToBoard("RNB1KBNR/PPPPPPPP/413/8/6B1/51N1/ppppkppp/rnbq1bnr/-/W");
    // enpassant pinned
    // Board b = positionToBoard("K1R5/P15P/8/QPpP3k/2P5/7b/6p1/1r1q4/-/W");
    // Board b = positionToBoard("K1R5/PP5P/8/8/Q2r3k/7b/6p1/1r1q4/-/W");
    // Board b = positionToBoard("K1R5/PP5P/7P/7P/Q213k/7b/5np1/1r1q4/-/W");

    std::cout << boardToStr(b) << std::endl;

    isInCheckResult r = isInCheck(b.blackPieces, b.whitePieces, false);

    auto start_time = std::chrono::high_resolution_clock::now();
    std::cout << traverse(b, 3) << std::endl;
    // b.isWhite = false;
    // for (int i=0; i<10000000; i++) {
    //     auto moves = generateMoves(b);
    // }
    auto end_time = std::chrono::high_resolution_clock::now();

    // auto moves = generateMoves(b);
    // for (auto pos : moves) {
    //     std::cout << boardToStr(pos) << std::endl;
    // }
    // std::cout << moves.size() << std::endl;
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    std::cout << "Execution time: " << duration.count() << " microseconds" << std::endl;
    return 0;
}