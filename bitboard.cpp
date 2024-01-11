#include "display.cpp"

Squares occupied(const Pieces &pieces) {
    return (pieces.pawns|pieces.rooks|pieces.bishops|pieces.knights|pieces.queens|pieces.king);
}

Squares Seen(const Pieces &selfPieces, const Pieces &enemyPieces, bool isWhite) {

    Squares enemySquares = occupied(enemyPieces);
    Squares selfSquares = occupied(selfPieces);
    Squares occupiedSquares = enemySquares | selfSquares;

    Squares enemySeen = 0;

    BitLoop(enemyPieces.rooks | enemyPieces.queens) {
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares rookReachable = rookMoves[pieceIndex];
        for(uint64_t temp2 = rookReachable & occupiedSquares; temp2; temp2 = _blsr_u64(temp2)) {
            rookReachable &= attacking[pieceIndex][bitFromSquare(temp2)];
        }
        enemySeen |= rookReachable;
    }

    BitLoop(enemyPieces.bishops | enemyPieces.queens) {
        uint64_t pieceIndex = bitFromSquare(temp);
        Squares bishopReachable = bishopMoves[pieceIndex];
        for(uint64_t temp2 = bishopReachable & occupiedSquares; temp2; temp2 = _blsr_u64(temp2)) {
            bishopReachable &= attacking[pieceIndex][bitFromSquare(temp2)];
        }
        enemySeen |= bishopReachable;
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
    Squares occupiedSquares = enemySquares & selfSquares;

    Squares occupiedNotKing = occupiedSquares ^ selfPieces.king;
    Squares selfNotKing = selfSquares ^ selfPieces.king; // all self pieces that are not the king

    bit kingIndex = bitFromSquare(selfPieces.king);

    int checkcount = 0;

    Squares checkmask = 0;
    Squares pinmaskHV = 0;
    Squares pinmaskD = 0;

    // TODO the case of enpassant pinned pawn
    BitLoop(enemyPieces.rooks | enemyPieces.queens) {
        uint64_t pieceIndex = bitFromSquare(temp);
        if (rookMoves[pieceIndex] & selfPieces.king) { // i.e. the enemy rook/queen can view the king (if the file/column is empty)

            //    path from king to enemy (inc)       exclude enemy
            Squares inbetween = (PinBetween[kingIndex][pieceIndex] ^ positionToBit[pieceIndex]) & occupiedNotKing;
            std::cout << "IB" << std::endl;
            display_int64(inbetween);
            if (!inbetween) { // i.e. the enemy rook is not blocked by any piece
                checkmask |= CheckBetween[kingIndex][pieceIndex];
                checkcount++;
            } else {
                Squares inbetweenEnemy = inbetween & enemySquares;
                if (!inbetweenEnemy) { // i.e. the enemy rook/queen is not blocked by enemy pieces
                    if (!_blsr_u64(inbetween & selfSquares)) { // there is only ONE self piece in between
                        pinmaskHV |= PinBetween[kingIndex][pieceIndex];
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
                checkmask |= CheckBetween[kingIndex][pieceIndex];
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
            checkmask |= knightMoves[kingIndex] & enemyPieces.knights; // add the knight to the checkmask
            checkcount++;
        }
    }

    BitLoop(enemyPieces.pawns) {
        uint64_t pieceIndex = bitFromSquare(temp);
        if (pawnAttacks(!isWhite)[pieceIndex] & selfPieces.king) { // i.e. pawn attacking our king
            checkmask |= selfPieces.king | positionToBit[pieceIndex];
            checkcount++;
        }
    }

    return {checkcount, checkmask, pinmaskHV, pinmaskD};
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
    Squares selfSeen = Seen(*enemy, *self, !board.isWhite);

    Squares selfPieces = occupied(*self);
    Squares enemyPieces = occupied(*enemy);
    Squares alloccupied = selfPieces | enemyPieces;

    if (r.checkCount > 1) { // double check forces king to move
        // king can only move to squares not seen by enemy and not occupied by enemy pieces
        Squares kingReachable = kingMoves[bitFromSquare(self->king)] & ~selfPieces & ~enemySeen;
        BitLoop(kingReachable) {
            newMoves.push_back(board);
        }

        return newMoves;
    } 
    if (r.checkCount == 0) r.checkmask = 0xffffffffffffffff; // if no checks are given, then all squares are available for all pieces

    // generate king moves
    Squares kingReachable = kingMoves[bitFromSquare(self->king)] & ~selfPieces & ~enemySeen;
    BitLoop(kingReachable) {
        newMoves.push_back(board);
    }

    // generate rook/queen moves
    
    // non-pinned rooks
    BitLoop((self->rooks | self->queens) & ~pinmask) {
        uint64_t pieceIndex = bitFromSquare(temp);
        for(uint64_t temp2 = rookMoves[pieceIndex] & selfSeen & ~selfPieces & r.checkmask; temp2; temp2 = _blsr_u64(temp2)) {
            newMoves.push_back(board);
        }
    }

    // pinned rooks
    BitLoop((self->rooks | self->queens) & pinmask) {
        uint64_t pieceIndex = bitFromSquare(temp);
        for(uint64_t temp2 = rookMoves[pieceIndex] & r.pinmaskHV & selfSeen & ~selfPieces & r.checkmask; temp2; temp2 = _blsr_u64(temp2)) {
            newMoves.push_back(board);
        }
    }

    // generate bishop/queen moves
    
    // non-pinned bishops
    BitLoop((self->bishops | self->queens) & ~pinmask) {
        uint64_t pieceIndex = bitFromSquare(temp);
        for(uint64_t temp2 = bishopMoves[pieceIndex] & selfSeen & ~selfPieces & r.checkmask; temp2; temp2 = _blsr_u64(temp2)) {
            newMoves.push_back(board);
        }
    }

    // pinned bishops
    BitLoop((self->bishops | self->queens) & pinmask) {
        uint64_t pieceIndex = bitFromSquare(temp);
        for(uint64_t temp2 = bishopMoves[pieceIndex] & r.pinmaskD & selfSeen & ~selfPieces & r.checkmask; temp2; temp2 = _blsr_u64(temp2)) {
            newMoves.push_back(board);
        }
    }

    // generate knight moves
    BitLoop(self->knights & ~pinmask) { // pinned knight cannot move, so only consider unpinned knights
        uint64_t pieceIndex = bitFromSquare(temp);
        for(uint64_t temp2 = knightMoves[pieceIndex] & ~selfPieces & r.checkmask; temp2; temp2 = _blsr_u64(temp2)) {
            newMoves.push_back(board);
        }
    }

    // generate pawn moves
    BitLoop(self->pawns & pinmask) { // pinned pawns
        uint64_t pieceIndex = bitFromSquare(temp);
        // advancing pawn 2 squares
        if (positionToBit[pieceIndex + pawnAdvace(board.isWhite)] & ~alloccupied) {
            if (positionToBit[pieceIndex + pawnAdvace(board.isWhite)] & ~alloccupied & r.checkmask & r.pinmaskHV) {
                newMoves.push_back(board);
            }
            if ((pieceIndex%8 == pawnInitial(board.isWhite)) && 
                (positionToBit[pieceIndex + 2 * pawnAdvace(board.isWhite)] & ~alloccupied & r.checkmask & r.pinmaskHV)) {
                newMoves.push_back(board);
            }
        }

        for(uint64_t temp2 = pawnAttacks(board.isWhite)[pieceIndex] & enemyPieces & r.checkmask & r.pinmaskD; temp2; temp2 = _blsr_u64(temp2)) {
            newMoves.push_back(board);
        }
    }

    BitLoop(self->pawns & pinmask) { // non-pinned pawns
        uint64_t pieceIndex = bitFromSquare(temp);
        // advancing pawn 2 squares
        if (positionToBit[pieceIndex + pawnAdvace(board.isWhite)] & ~alloccupied) { // piece blocking path
            if (positionToBit[pieceIndex + pawnAdvace(board.isWhite)] & ~alloccupied & r.checkmask) { // advance 1 square
                newMoves.push_back(board);
            }
            if ((pieceIndex%8 == pawnInitial(board.isWhite)) && 
                (positionToBit[pieceIndex + 2 * pawnAdvace(board.isWhite)] & ~alloccupied & r.checkmask)) { // advance 2 squares
                newMoves.push_back(board);
            }
        }

        for(uint64_t temp2 = pawnAttacks(board.isWhite)[pieceIndex] & enemyPieces & r.checkmask; temp2; temp2 = _blsr_u64(temp2)) {
            newMoves.push_back(board);
        }
    }

    // TODO en-passant

    // TODO castles

    return newMoves;
}

int main(void) {

    // Board b = positionToBoard("RNBQKBNR/PPPPPPPP/8/8/8/8/pppppppp/rnbqkbnr/-/W");
    Board b = positionToBoard("RNB1KBNR/PPPPPPPP/4Q3/8/8/8/pppppppp/rnbqkbnr/-/W");

    std::cout << boardToStr(b) << std::endl;

    display_int64(Seen(b.whitePieces, b.blackPieces, true));

    isInCheckResult r = isInCheck(b.blackPieces, b.whitePieces, false);
    std::cout << (int) r.checkCount << std::endl;
    display_int64(r.pinmaskHV);

    return 0;
}