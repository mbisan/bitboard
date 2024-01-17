#include <string>
#include <cstdint>

#define KING 0
#define QUEEN 1
#define ROOK 2
#define BISHOP 3
#define KNIGHT 4
#define PAWN 5

typedef uint64_t Squares;

Squares notAfile;
Squares notHfile;

Squares wPawnStart;
Squares bPawnStart;
Squares wPawnMiddle;
Squares bPawnMiddle;
Squares wPawnLast;
Squares bPawnLast;
Squares wPawnEP;
Squares bPawnEP;

Squares wLrookStart = 0x0000000000000001ULL;
Squares wRrookStart = 0x0000000000000080ULL;
Squares bLrookStart = 0x0100000000000000ULL;
Squares bRrookStart = 0x8000000000000000ULL;

const struct GameState {
    bool ep;
    bool wL;
    bool wR;
    bool bL;
    bool wL;
    bool isWhite;

};

struct Pieces {
    Squares k; Squares q; Squares r; Squares b; Squares n; Squares p;

    Pieces moveKing(Squares move) { return {k ^ move, q, r, b, n, p}; } // move contains the initial and final bits of the piece
    Pieces moveQueen(Squares move) { return {k, q ^ move, r, b, n, p}; } 
    Pieces moveRook(Squares move) { return {k, q, r ^ move, b, n, p}; } 
    Pieces moveBishop(Squares move) { return {k, q, r, b ^ move, n, p}; } 
    Pieces moveKnight(Squares move) { return {k, q, r, b, n ^ move, p}; } 
    Pieces movePawn(Squares move) { return {k, q, r, b, n, p ^ move}; }

    Pieces remove(Squares pos) { return {k, q & pos, r & pos, b & pos, n & pos, p & pos}; } // pos contains ones except on the removed piece
};

struct Board {
    Pieces w; Pieces b; uint64_t ep; GameState state; 
    // gamestate bits from left to right (isWhite: 1 bit) (castles w/b l/r: 4 bits) (enpassant: 1 bit) (draw: 1 bit) (chechmate: 1 bit)
    // masks to extract gamestate: isWhite: 0b00000001 castles: 0b00011110 enpassant: 0b00100000 game_ended: 0b11000000

    template<GameState state, int piece>
    Board pieceMove(Squares move) {
        if constexpr (state.isWhite) {
            if constexpr (piece == KING) return {w.moveKing(move), b, 0, state.kingMove()};
            if constexpr (piece == QUEEN) return {w.moveQueen(move), b, 0, state.move()};
            if constexpr (piece == BISHOP) return {w.moveBishop(move), b, 0, state.move()};
            if constexpr (piece == KNIGHT) return {w.moveKnight(move), b, 0, state.move()};
            if constexpr (piece == PAWN) return {w.movePawn(move), b, 0, state.move()};

            if constexpr (piece == ROOK && !state.hasCastle()) return {w, b.moveRook(move), 0, state.move()};
            else { // check if move and initial rook positions coincide to remove castling possibility
                if (move & wLrookStart) return {w, b.moveRook(move), 0, state.LRookMove()};
                if (move & wRrookStart) return {w, b.moveRook(move), 0, state.RRookMove()};
            }
        } else if (!state.isWhite && !state.hasCastle()) {
            if constexpr (piece == KING) return {w, b.moveKing(move), 0, state.kingMove()};
            if constexpr (piece == QUEEN) return {w, b.moveQueen(move), 0, state.move()};
            if constexpr (piece == BISHOP) return {w, b.moveBishop(move), 0, state.move()};
            if constexpr (piece == KNIGHT) return {w, b.moveKnight(move), 0, state.move()};
            if constexpr (piece == PAWN) return {w, b.movePawn(move), 0, state.move()};

            if constexpr (piece == ROOK && !state.hasCastle()) return {w, b.moveRook(move), 0, state.move()};
            else {
                if (move & bLrookStart) return {w, b.moveRook(move), 0, state.LRookMove()};
                if (move & bRrookStart) return {w, b.moveRook(move), 0, state.RRookMove()};
            }
        }
    }

    template<GameState state>
    Board pawnPush(Squares pawnSquare, Squares move) {
        if constexpr (state.isWhite) return {w.movePawn(move), b, pawnSquare, state.pawnPush()};
        else 
    }
};