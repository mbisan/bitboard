#include <iostream>
#include <immintrin.h>

#include "lookup.cpp"

// _blsr_u64(X) https://www.felixcloutier.com/x86/blsr sets the lowest set bit to 0 i.e. _blsr_u64(0b100100) = 0b100000
// _tzcnt_u64(X) https://www.felixcloutier.com/x86/blsr counts number of trailing zeros i.e. _tzcnt_u64(0b100100) = 2

// for(; X; X=_blsr_u64(X)) std::cout << _tzcnt_u64(X) << ", "; 0b1100100 > outputs 2, 5, 6

#define BitLoop(x) for(uint64_t temp=x; temp; temp=_blsr_u64(temp))
#define bitFromSquare(x) _tzcnt_u64(x)

struct Pieces {
    Squares pawns, rooks, bishops, knights, queens, king = 0;
    bool enPassant = false;
    bool castleR = true;
    bool castleL = true;
    // bit pawnsID[64], rooksID[64], bishopsID[64], knightsID[64], queensID[64] {0};
};

struct Board {
    Pieces whitePieces, blackPieces;
};

Squares occupied(const Pieces &pieces) {
    return (pieces.pawns|pieces.rooks|pieces.bishops|pieces.knights|pieces.queens|positionToBit[pieces.king]);
}

Squares occupied(const Board &board) {
    return occupied(board.whitePieces) | occupied(board.blackPieces);
}

Squares KingOnSight(Board &board, bool isWhite) {
    if (isWhite) { // i.e. white to move, pieces of the enemy that "see" the white king
        Squares enemySquares = occupied(board.blackPieces);
        Squares selfSquares = occupied(board.whitePieces);

        Squares relevantSquares = 0; // fill this with the squares that sliding pieces of the enemy reach king

        BitLoop(board.blackPieces.rooks) {
            uint64_t pieceIndex = bitFromSquare(temp);
            if (rookMoves[pieceIndex] & board.whitePieces.king) { // i.e. the enemy rook can view the king (if the file/column is empty)
                relevantSquares |= lookupSliding[bitFromSquare(board.whitePieces.king)][pieceIndex];
            }
        }

        BitLoop(board.blackPieces.bishops) {
            uint64_t pieceIndex = bitFromSquare(temp);
            if (bishopMoves[pieceIndex] & board.whitePieces.king) { // i.e. the enemy rook can view the king (if the diag is empty)
                relevantSquares |= lookupSliding[bitFromSquare(board.whitePieces.king)][pieceIndex];
            }
        }

        BitLoop(board.blackPieces.queens) {
            uint64_t pieceIndex = bitFromSquare(temp);
            if (queenMoves[pieceIndex] & board.whitePieces.king) {
                relevantSquares |= lookupSliding[bitFromSquare(board.whitePieces.king)][pieceIndex];
            }
        }

        return relevantSquares;
    } else {

    }
}

int main(void) {

    BitLoop(0b100100110101ULL) {
        std::cout << (int) bitFromSquare(temp) << ", ";
    } std::cout << std::endl;

    Squares a;
    std::cout << "Hi" << std::endl;
    return 0;
}