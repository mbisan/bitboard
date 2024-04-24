#include <string>
#include <cstdint>

#define KING 0
#define QUEEN 1
#define ROOK 2
#define BISHOP 3
#define KNIGHT 4
#define PAWN 5

typedef uint64_t Squares;

Squares notAfile = 0xfefefefefefefefeULL;
Squares notHfile = 0x7f7f7f7f7f7f7f7fULL;

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

struct GameState {
    bool ep;
    bool wL;
    bool wR;
    bool bL;
    bool bR;
    bool isWhite;

    constexpr GameState(bool ep_, bool wL_, bool wR_, bool bL_, bool bR_, bool isWhite_)
        : ep(ep_), wL(wL_), wR(wR_), bL(bL_), bR(bR_), isWhite(isWhite_) {}

    bool hasCastle() {
        if (isWhite) return wL || wR;
        else return bL || bR; 
    }

    bool enemyCastle() {
        if (isWhite) return bL || bR;
        else return wL || wR; 
    }

    uint8_t stateToInt(void) {
        return isWhite | ep << 1 | wL << 2 | wR << 3 | bL << 4 | bR << 5;
    }

    template<bool White>
    GameState kingMove() {
        if constexpr (White) return {false, false, false, bL, bR, false};
        else return {false, wL, wR, false, false, true};
    }

    template<bool White>
    GameState move() {
        if constexpr (White) return {false, wL, wR, bL, bR, false};
        else return {false, wL, wR, bL, bR, true};
    }

    template<bool White>
    GameState LRookMove() {
        if constexpr (White) return {false, false, wR, bL, bR, false};
        else return {false, wL, wR, false, bR, true};
    }

    template<bool White>
    GameState RRookMove() {
        if constexpr (White) return {false, wL, false, bL, bR, false};
        else return {false, wL, wR, bL, false, true};
    }

    template<bool White>
    GameState pawnPush() {
        if constexpr (White) return {true, wL, wR, bL, bR, false};
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

    Squares occupied() const { return k|q|r|b|n|p; }
};

struct Board {
    Pieces w; Pieces b; uint64_t ep; GameState state; 

    constexpr Board &operator=(const Board& board) {
        w = board.w; b = board.b; ep = board.ep; state = board.state;
        return *this;
    }

    template<int piece, bool isWhite>
    Board pieceMove(Squares move) { // piece moves without captures
        if constexpr (isWhite) {
            if constexpr (piece == KING) return {w.moveKing(move), b, 0, state.kingMove<isWhite>()};
            if constexpr (piece == QUEEN) return {w.moveQueen(move), b, 0, state.move<isWhite>()};
            if constexpr (piece == BISHOP) return {w.moveBishop(move), b, 0, state.move<isWhite>()};
            if constexpr (piece == KNIGHT) return {w.moveKnight(move), b, 0, state.move<isWhite>()};
            if constexpr (piece == PAWN) return {w.movePawn(move), b, 0, state.move<isWhite>()};

            if constexpr (piece == ROOK) {
                if (!state.hasCastle()) return {w.moveRook(move), b, 0, state.move<isWhite>()};
                else { // check if move and initial rook positions coincide to remove castling possibility
                    if (move & wLrookStart) return {w.moveRook(move), b, 0, state.LRookMove<isWhite>()};
                    else return {w.moveRook(move), b, 0, state.RRookMove<isWhite>()}; // if (move & wRrookStart)
                }
            }
        } else {
            if constexpr (piece == KING) return {w, b.moveKing(move), 0, state.kingMove<isWhite>()};
            if constexpr (piece == QUEEN) return {w, b.moveQueen(move), 0, state.move<isWhite>()};
            if constexpr (piece == BISHOP) return {w, b.moveBishop(move), 0, state.move<isWhite>()};
            if constexpr (piece == KNIGHT) return {w, b.moveKnight(move), 0, state.move<isWhite>()};
            if constexpr (piece == PAWN) return {w, b.movePawn(move), 0, state.move<isWhite>()};

            if constexpr (piece == ROOK) {
                if (!state.hasCastle()) return {w, b.moveRook(move), 0, state.move<isWhite>()};
                else {
                    if (move & bLrookStart) return {w, b.moveRook(move), 0, state.LRookMove<isWhite>()};
                    else return {w, b.moveRook(move), 0, state.RRookMove<isWhite>()}; //  (move & bRrookStart) only option left
                }
            }
        }
    }

    template<bool isWhite>
    Board pawnPush(Squares pawnSquare, Squares move) {
        if constexpr (isWhite) return {w.movePawn(move), b, pawnSquare, state.pawnPush<isWhite>()};
        else return {w, b.movePawn(move), pawnSquare, state.pawnPush<isWhite>()};
    }

    template<int piece, bool isWhite>
    Board pawnPromote(Squares pawnSquare, Squares move) {
        if constexpr (isWhite) {
            Pieces wn = w.movePawn(pawnSquare);
            if constexpr (piece == QUEEN) return {wn.moveQueen(move), b, 0, state.move<isWhite>()};
            if constexpr (piece == BISHOP) return {wn.moveBishop(move), b, 0, state.move<isWhite>()};
            if constexpr (piece == KNIGHT) return {wn.moveKnight(move), b, 0, state.move<isWhite>()};
            if constexpr (piece == ROOK) return {wn.movePawn(move), b, 0, state.move<isWhite>()};
        } else {
            Pieces bn = b.movePawn(pawnSquare);
            if constexpr (piece == QUEEN) return {w, bn.moveQueen(move), 0, state.move<isWhite>()};
            if constexpr (piece == BISHOP) return {w, bn.moveBishop(move), 0, state.move<isWhite>()};
            if constexpr (piece == KNIGHT) return {w, bn.moveKnight(move), 0, state.move<isWhite>()};
            if constexpr (piece == ROOK) return {w, bn.movePawn(move), 0, state.move<isWhite>()};
        }
    }

    template<int piece, bool isWhite>
    Board pawnPromoteCapture(Squares pawnSquare, Squares move) {
        if constexpr (isWhite) {
            Pieces wn = w.movePawn(pawnSquare);
            if constexpr (piece == QUEEN) return {wn.moveQueen(move), b.remove(move), 0, state.move<isWhite>()};
            if constexpr (piece == BISHOP) return {wn.moveBishop(move), b.remove(move), 0, state.move<isWhite>()};
            if constexpr (piece == KNIGHT) return {wn.moveKnight(move), b.remove(move), 0, state.move<isWhite>()};
            if constexpr (piece == ROOK) return {wn.movePawn(move), b.remove(move), 0, state.move<isWhite>()};
        } else {
            Pieces bn = b.movePawn(pawnSquare);
            if constexpr (piece == QUEEN) return {w.remove(move), bn.moveQueen(move), 0, state.move<isWhite>()};
            if constexpr (piece == BISHOP) return {w.remove(move), bn.moveBishop(move), 0, state.move<isWhite>()};
            if constexpr (piece == KNIGHT) return {w.remove(move), bn.moveKnight(move), 0, state.move<isWhite>()};
            if constexpr (piece == ROOK) return {w.remove(move), bn.movePawn(move), 0, state.move<isWhite>()};
        }
    }

    template<int piece, bool isWhite>
    Board pieceMoveCapture(Squares move) { // piece moves with captures
        Squares notmove = ~move;
        if constexpr (isWhite) {
            if constexpr (piece == KING) return {w.moveKing(move), b.remove(notmove), 0, state.kingMove<isWhite>()};
            if constexpr (piece == QUEEN) return {w.moveQueen(move), b.remove(notmove), 0, state.move<isWhite>()};
            if constexpr (piece == BISHOP) return {w.moveBishop(move), b.remove(notmove), 0, state.move<isWhite>()};
            if constexpr (piece == KNIGHT) return {w.moveKnight(move), b.remove(notmove), 0, state.move<isWhite>()};
            if constexpr (piece == PAWN) return {w.movePawn(move), b.remove(notmove), 0, state.move<isWhite>()};

            if constexpr (piece == ROOK) {
                if (!state.hasCastle()) return {w.moveRook(move), b.remove(notmove), 0, state.move<isWhite>()};
                else {
                    if (move & bLrookStart) return {w.moveRook(move), b.remove(notmove), 0, state.LRookMove<isWhite>()};
                    else return {w.moveRook(move), b.remove(notmove), 0, state.RRookMove<isWhite>()}; // if (move & bRrookStart)
                }
            }
        } else {
            if constexpr (piece == KING) return {w.remove(notmove), b.moveKing(move), 0, state.kingMove<isWhite>()};
            if constexpr (piece == QUEEN) return {w.remove(notmove), b.moveQueen(move), 0, state.move<isWhite>()};
            if constexpr (piece == BISHOP) return {w.remove(notmove), b.moveBishop(move), 0, state.move<isWhite>()};
            if constexpr (piece == KNIGHT) return {w.remove(notmove), b.moveKnight(move), 0, state.move<isWhite>()};
            if constexpr (piece == PAWN) return {w.remove(notmove), b.movePawn(move), 0, state.move<isWhite>()};

            if constexpr (piece == ROOK) {
                if (!state.hasCastle()) return {w.remove(notmove), b.moveRook(move), 0, state.move<isWhite>()};
                else {
                    if (move & bLrookStart) return {w.remove(notmove), b.moveRook(move), 0, state.LRookMove<isWhite>()};
                    else return {w.remove(notmove), b.moveRook(move), 0, state.RRookMove<isWhite>()}; // if (move & bRrookStart)
                }
            }
        }
    }

    template<bool isWhite>
    Board castleL() {
        if constexpr (isWhite) {
            Pieces wn = w.moveKing(0x0000000000000014ULL);
            return {wn.moveRook(0x0000000000000009ULL), b, 0, state.kingMove<isWhite>()};
        } else {
            Pieces bn = b.moveKing(0x1400000000000000ULL);
            return {w, bn.moveRook(0x0900000000000000ULL), 0, state.kingMove<isWhite>()};
        }
    }

    template<bool isWhite>
    Board castleR() {
        if constexpr (isWhite) {
            Pieces wn = w.moveKing(0x0000000000000050ULL);
            return {wn.moveRook(0x00000000000000a0ULL), b, 0, state.kingMove<isWhite>()};
        } else {
            Pieces bn = b.moveKing(0x5000000000000000ULL);
            return {w, bn.moveRook(0xa000000000000000ULL), 0, state.kingMove<isWhite>()};
        }
    }
};