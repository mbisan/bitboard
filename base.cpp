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

const struct GameState {
    uint8_t gamestate;

    bool hasEnPassant() const { return gamestate & 0b00100000; }
    bool gameEnded() const { return gamestate & 0b11000000; }
    bool isWhite() const { return gamestate & 0b00000001; }
    bool hasLCastle() const { 
        if (isWhite()) return gamestate & 0b00000010; 
        return gamestate & 0b00001000;
    }
    bool hasRCastle() const { 
        if (isWhite()) return gamestate & 0b00000100; 
        return gamestate & 0b00010000;
    }

    bool hasCastle() const { 
        if (isWhite()) return gamestate & 0b00000110; 
        return gamestate & 0b00011000;
    }

    GameState next() { return {gamestate ^ 0b00000001}; }
    GameState kingMove() {         
        if (isWhite()) return {gamestate & 0b11111000}; // last bit set to 0 since its black's turn 
        return {(gamestate & 0b11100111) ^ 0b00000001};
    }
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
        if (state.isWhite()) {
            if (piece == KING) return {w.moveKing(move), b, 0, state.kingMove()};
            if (piece == QUEEN) return {w.moveQueen(move), b, 0, state.move()};
            if (piece == BISHOP) return {w.moveBishop(move), b, 0, state.move()};
            if (piece == KNIGHT) return {w.moveKnight(move), b, 0, state.move()};
            if (piece == PAWN) return {w.movePawn(move), b, 0, state.move()};

            if (piece == ROOK && !state.hasCastle()) return {w, b.moveRook(move), 0, state.move()};
            else { // check if move and initial rook positions coincide to remove castleng possibility

            }
        } else if (!state.isWhite() && !state.hasCastle()) {
            if (piece == KING) return {w, b.moveKing(move), 0, state.kingMove()};
            if (piece == QUEEN) return {w, b.moveQueen(move), 0, state.move()};
            if (piece == BISHOP) return {w, b.moveBishop(move), 0, state.move()};
            if (piece == KNIGHT) return {w, b.moveKnight(move), 0, state.move()};
            if (piece == PAWN) return {w, b.movePawn(move), 0, state.move()};

            if (piece == ROOK && !state.hasCastle()) return {w, b.moveRook(move), 0, state.move()};
            else {

            }
        }
    }
};