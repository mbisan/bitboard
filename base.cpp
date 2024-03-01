#include <string>
#include <cstdint>

#define KING 0
#define QUEEN 1
#define ROOK 2
#define BISHOP 3
#define KNIGHT 4
#define PAWN 5

typedef uint64_t Squares;

Squares notAfile = 0xefefefefefefefefULL;
Squares notHfile = 0xf7f7f7f7f7f7f7f7ULL;

Squares wPawnStart = 0x000000000000ff00ULL;
Squares bPawnStart = 0x00ff000000000000ULL;
Squares PawnMiddle = 0x0000ffffffff0000ULL;
Squares wPawnLast = bPawnStart;
Squares bPawnLast = wPawnStart;
Squares wPawnEP = 0x000000ff00000000ULL;
Squares bPawnEP = 0x00000000ff000000ULL;

Squares wLrookStart = 0x0000000000000001ULL;
Squares wRrookStart = 0x0000000000000080ULL;
Squares bLrookStart = 0x0100000000000000ULL;
Squares bRrookStart = 0x8000000000000000ULL;

Squares wLCastleSeen = 0x000000000000000eULL;
Squares wRCastleSeen = 0x0000000000000060ULL;
Squares bLCastleSeen = 0x0e00000000000000ULL;
Squares bRCastleSeen = 0x6000000000000000ULL;

const struct GameState {
    const bool ep;
    const bool wL;
    const bool wR;
    const bool bL;
    const bool bR;
    const bool isWhite;

    constexpr bool hasCastle() {
        if (isWhite) return wL || wR;
        else return bL || bR; 
    }

    constexpr bool enemyCastle() {
        if (isWhite) return bL || bR;
        else return wL || wR; 
    }

    constexpr GameState kingMove() {
        if (isWhite) return {false, false, false, bL, bR, false};
        else return {false, wL, wR, false, false, true};
    }

    constexpr GameState move() {
        if (isWhite) return {false, wL, wR, bL, bR, false};
        else return {false, wL, wR, bL, bR, true};
    }

    constexpr GameState LRookMove() {
        if (isWhite) return {false, false, wR, bL, bR, false};
        else return {false, wL, wR, false, bR, true};
    }

    constexpr GameState RRookMove() {
        if (isWhite) return {false, wL, false, bL, bR, false};
        else return {false, wL, wR, bL, false, true};
    }

    constexpr GameState pawnPush() {
        if (isWhite) return {true, wL, wR, bL, bR, false};
        else return {true, wL, wR, bL, bR, true};
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

    Squares occupied() { return k|q|r|b|n|p; }
};

struct Board {
    Pieces w; Pieces b; uint64_t ep; GameState state; 

    template<GameState state>
    Squares occupied() {
        if constexpr (state.isWhite) return w.occupied();
        else return b.occupied();
    }

    template<GameState state>
    Squares enemy() {
        if constexpr (state.isWhite) return b.occupied();
        else return w.occupied();
    }

    template<GameState state, int piece>
    Board pieceMove(Squares move) { // piece moves without captures
        if constexpr (state.isWhite) {
            if constexpr (piece == KING) return {w.moveKing(move), b, 0, state.kingMove()};
            if constexpr (piece == QUEEN) return {w.moveQueen(move), b, 0, state.move()};
            if constexpr (piece == BISHOP) return {w.moveBishop(move), b, 0, state.move()};
            if constexpr (piece == KNIGHT) return {w.moveKnight(move), b, 0, state.move()};
            if constexpr (piece == PAWN) return {w.movePawn(move), b, 0, state.move()};

            if constexpr (piece == ROOK) {
                if constexpr (!state.hasCastle()) return {w, b.moveRook(move), 0, state.move()};
                else { // check if move and initial rook positions coincide to remove castling possibility
                    if (move & wLrookStart) return {w, b.moveRook(move), 0, state.LRookMove()};
                    if (move & wRrookStart) return {w, b.moveRook(move), 0, state.RRookMove()};
                }
            }
        } else if (!state.isWhite && !state.hasCastle()) {
            if constexpr (piece == KING) return {w, b.moveKing(move), 0, state.kingMove()};
            if constexpr (piece == QUEEN) return {w, b.moveQueen(move), 0, state.move()};
            if constexpr (piece == BISHOP) return {w, b.moveBishop(move), 0, state.move()};
            if constexpr (piece == KNIGHT) return {w, b.moveKnight(move), 0, state.move()};
            if constexpr (piece == PAWN) return {w, b.movePawn(move), 0, state.move()};

            if constexpr (piece == ROOK) {
                if constexpr (!state.hasCastle()) return {w, b.moveRook(move), 0, state.move()};
                else {
                    if (move & bLrookStart) return {w, b.moveRook(move), 0, state.LRookMove()};
                    if (move & bRrookStart) return {w, b.moveRook(move), 0, state.RRookMove()};
                }
            }
        }
    }

    template<GameState state>
    Board pawnPush(Squares pawnSquare, Squares move) {
        if constexpr (state.isWhite) return {w.movePawn(move), b, pawnSquare, state.pawnPush()};
        else return {w, b.movePawn(move), pawnSquare, state.pawnPush()}
    }

    template<GameState state, int piece>
    Board pawnPromote(Squares pawnSquare, Squares move) {
        if constexpr (state.isWhite) {
            Pieces wn = w.movePawn(pawnSquare);
            if constexpr (piece == QUEEN) return {wn.moveQueen(move), b, 0, state.move()};
            if constexpr (piece == BISHOP) return {wn.moveBishop(move), b, 0, state.move()};
            if constexpr (piece == KNIGHT) return {wn.moveKnight(move), b, 0, state.move()};
            if constexpr (piece == ROOK) return {wn.movePawn(move), b, 0, state.move()};
        } else {
            Pieces bn = b.movePawn(pawnSquare);
            if constexpr (piece == QUEEN) return {w, bn.moveQueen(move), 0, state.move()};
            if constexpr (piece == BISHOP) return {w, bn.moveBishop(move), 0, state.move()};
            if constexpr (piece == KNIGHT) return {w, bn.moveKnight(move), 0, state.move()};
            if constexpr (piece == ROOK) return {w, bn.movePawn(move), 0, state.move()};
        }
    }

    template<GameState state, int piece>
    Board pawnPromoteCapture(Squares pawnSquare, Squares move) {
        if constexpr (state.isWhite) {
            Pieces wn = w.movePawn(pawnSquare);
            if constexpr (piece == QUEEN) return {wn.moveQueen(move), b.remove(move), 0, state.move()};
            if constexpr (piece == BISHOP) return {wn.moveBishop(move), b.remove(move), 0, state.move()};
            if constexpr (piece == KNIGHT) return {wn.moveKnight(move), b.remove(move), 0, state.move()};
            if constexpr (piece == ROOK) return {wn.movePawn(move), b.remove(move), 0, state.move()};
        } else {
            Pieces bn = b.movePawn(pawnSquare);
            if constexpr (piece == QUEEN) return {w.remove(move), bn.moveQueen(move), 0, state.move()};
            if constexpr (piece == BISHOP) return {w.remove(move), bn.moveBishop(move), 0, state.move()};
            if constexpr (piece == KNIGHT) return {w.remove(move), bn.moveKnight(move), 0, state.move()};
            if constexpr (piece == ROOK) return {w.remove(move), bn.movePawn(move), 0, state.move()};
        }
    }

    template<GameState state, int piece>
    Board pieceMoveCapture(Squares move) { // piece moves with captures
        Squares notmove = ~move;
        if constexpr (state.isWhite) {
            if constexpr (piece == KING) return {w.moveKing(move), b.remove(notmove), 0, state.kingMove()};
            if constexpr (piece == QUEEN) return {w.moveQueen(move), b.remove(notmove), 0, state.move()};
            if constexpr (piece == BISHOP) return {w.moveBishop(move), b.remove(notmove), 0, state.move()};
            if constexpr (piece == KNIGHT) return {w.moveKnight(move), b.remove(notmove), 0, state.move()};
            if constexpr (piece == PAWN) return {w.movePawn(move), b.remove(notmove), 0, state.move()};

            if constexpr (piece == ROOK) {
                if constexpr (!state.hasCastle()) return {w.moveRook(move), b.remove(notmove), 0, state.move()};
                else {
                    if (move & bLrookStart) return {w.moveRook(move), b.remove(notmove), 0, state.LRookMove()};
                    if (move & bRrookStart) return {w.moveRook(move), b.remove(notmove), 0, state.RRookMove()};
                }
            }
        } else if (!state.isWhite && !state.hasCastle()) {
            if constexpr (piece == KING) return {w.remove(notmove), b.moveKing(move), 0, state.kingMove()};
            if constexpr (piece == QUEEN) return {w.remove(notmove), b.moveQueen(move), 0, state.move()};
            if constexpr (piece == BISHOP) return {w.remove(notmove), b.moveBishop(move), 0, state.move()};
            if constexpr (piece == KNIGHT) return {w.remove(notmove), b.moveKnight(move), 0, state.move()};
            if constexpr (piece == PAWN) return {w.remove(notmove), b.movePawn(move), 0, state.move()};

            if constexpr (piece == ROOK) {
                if constexpr (!state.hasCastle()) return {w.remove(notmove), b.moveRook(move), 0, state.move()};
                else {
                    if (move & bLrookStart) return {w.remove(notmove), b.moveRook(move), 0, state.LRookMove()};
                    if (move & bRrookStart) return {w.remove(notmove), b.moveRook(move), 0, state.RRookMove()};
                }
            }
        }
    }

    template<GameState state>
    Board castleL() {
        if constexpr (state.isWhite) {
            Pieces wn = w.moveKing(0x0000000000000014ULL);
            return {wn.moveRook(0x0000000000000009ULL), b, 0, state.kingMove()};
        } else {
            Pieces bn = b.moveKing(0x1400000000000000ULL);
            return {wn.moveRook(0x0900000000000000ULL), b, 0, state.kingMove()};
        }
    }

    template<GameState state>
    Board castleR() {
        if constexpr (state.isWhite) {
            Pieces wn = w.moveKing(0x0000000000000050ULL);
            return {wn.moveRook(0x00000000000000a0ULL), b, 0, state.kingMove()};
        } else {
            Pieces bn = b.moveKing(0x5000000000000000ULL);
            return {wn.moveRook(0xa000000000000000ULL), b, 0, state.kingMove()};
        }
    }
};