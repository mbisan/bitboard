#include "lookup.cpp"

#include <iostream>
#include <immintrin.h>
#include <array>
#include <vector>
#include <string>

typedef uint64_t Squares; // occupied squares have bits set to one

template<bool isWhite>
constexpr int pawnInitial() {
    return (isWhite ? 1 : 6);
}

template<bool isWhite>
constexpr int pawnAdvance() {
    return (isWhite ? 8 : -8);
}

template<bool isWhite>
constexpr uint64_t pawnAdvanceShift(uint64_t pawn) {
    if (isWhite) return pawn<<8;
    else return pawn>>8;
}

template<bool isWhite>
constexpr int enPassantFile() {
    return (isWhite ? 4 : 2);
}

template<bool isWhite>
constexpr uint64_t pawnStartFile() {
    return (isWhite ? 0x000000000000ff00ULL : 0x00ff000000000000ULL);
}

template<bool isWhite>
constexpr uint64_t startFile() {
    return (isWhite ? 0x00000000000000ffULL : 0xff00000000000000ULL);
}

template<bool isWhite>
constexpr uint64_t pawnEP() {
    return (isWhite ? 0x000000ff00000000ULL : 0x00000000ff000000ULL);
}

template<bool isWhite>
constexpr uint64_t pawnPEndFile() {
    return (isWhite ? 0x00ff000000000000ULL : 0x00000000000ff00ULL);
}

template<bool isWhite>
constexpr uint64_t castleLocc() {
    return (isWhite ? 0x000000000000000eULL : 0x0e00000000000000ULL);
}

template<bool isWhite>
constexpr uint64_t castleLcheck() {
    return (isWhite ? 0x000000000000000cULL : 0x0c00000000000000ULL);
}

template<bool isWhite>
constexpr uint64_t castleRcheck() {
    return (isWhite ? 0x0000000000000060ULL : 0x6000000000000000ULL);
}

template<bool isWhite>
constexpr uint64_t *pawnAttacks() {
    return (isWhite ? wpawnAttacks : bpawnAttacks);
}

template<bool isWhite>
constexpr int enPassantShift() {
    return (isWhite ? 32 : 24);
}

template<bool isWhite>
constexpr int castlesL() {
    return (isWhite ? 0b1 : 0b100);
}

template<bool isWhite>
constexpr int castlesR() {
    return (isWhite ? 0b10 : 0b1000);
}

// _blsr_u64(X) https://www.felixcloutier.com/x86/blsr sets the lowest set bit to 0 i.e. _blsr_u64(0b100100) = 0b100000
// _tzcnt_u64(X) https://www.felixcloutier.com/x86/blsr counts number of trailing zeros i.e. _tzcnt_u64(0b100100) = 2

// for(; X; X=_blsr_u64(X)) std::cout << _tzcnt_u64(X) << ", "; 0b1100100 > outputs 2, 5, 6

#define BitLoop(x) for(uint64_t temp=x; temp; temp=_blsr_u64(temp))
#define BitLoop2(x) for(uint64_t temp2=x; temp2; temp2=_blsr_u64(temp2))
#define bitFromSquare(x) _tzcnt_u64(x)

struct Pieces {
    Squares pawns = 0;
    Squares rooks = 0;
    Squares bishops = 0;
    Squares knights = 0;
    Squares queens = 0;
    Squares king = 0;

    Squares occupied() const {return (pawns|rooks|bishops|knights|queens|king);}
};

struct Board {
    Pieces wP;
    Pieces bP;
    bool isWhite;
    uint8_t enPassant;
    uint8_t castlesStatus; // first bit for white L castle second for white R, 3rd and 4th for black L and R

    template<bool white>
    Board pawnForward(Squares initial, Squares final) const {
        if constexpr (white) {
            return {
                {wP.pawns ^ (initial | final), wP.rooks, wP.bishops, wP.knights, wP.queens, wP.king},
                {bP.pawns, bP.rooks, bP.bishops, bP.knights, bP.queens, bP.king}, !white,
                0, castlesStatus
                };
        } else {
            return {
                {wP.pawns, wP.rooks, wP.bishops, wP.knights, wP.queens, wP.king},
                {bP.pawns ^ (initial | final), bP.rooks, bP.bishops, bP.knights, bP.queens, bP.king}, !white,
                0, castlesStatus
                };
        }
    }

    template<bool white>
    Board pawnPush(Squares initial, Squares final) const {
        if constexpr (white) {
            return {
                {wP.pawns ^ (initial | final), wP.rooks, wP.bishops, wP.knights, wP.queens, wP.king},
                {bP.pawns, bP.rooks, bP.bishops, bP.knights, bP.queens, bP.king},  !white,
                (uint8_t) (initial >> 8), castlesStatus
                };
        } else {
            return {
                {wP.pawns, wP.rooks, wP.bishops, wP.knights, wP.queens, wP.king},
                {bP.pawns ^ (initial | final), bP.rooks, bP.bishops, bP.knights, bP.queens, bP.king}, !white,
                (uint8_t) (initial >> 48), castlesStatus
                };
        }
    }

    template<bool white>
    Board pawnCapture(Squares initial, Squares final) const {
        Squares nfinal = ~final;
        if constexpr (white) {
            return {
                {wP.pawns ^ (initial | final), wP.rooks, wP.bishops, wP.knights, wP.queens, wP.king},
                {bP.pawns & nfinal, bP.rooks & nfinal, bP.bishops & nfinal, bP.knights & nfinal, bP.queens & nfinal, bP.king}, !white,
                0, castlesStatus
                };
        } else {
            return {
                {wP.pawns & nfinal, wP.rooks & nfinal, wP.bishops & nfinal, wP.knights & nfinal, wP.queens & nfinal, wP.king}, 
                {bP.pawns ^ (initial | final), bP.rooks, bP.bishops, bP.knights, bP.queens, bP.king}, !white,
                0, castlesStatus
                };
        }
    }

    template<bool white, int piece>
    Board pawnPromote(Squares initial, Squares final) const {
        if constexpr (piece == 2) { // queen
            if constexpr (white) {
                return {
                    {wP.pawns ^ initial, wP.rooks, wP.bishops, wP.knights, wP.queens ^ final, wP.king},
                    {bP.pawns, bP.rooks, bP.bishops, bP.knights, bP.queens, bP.king}, !white,
                    0, castlesStatus
                    };
            } else {
                return {
                    {wP.pawns, wP.rooks, wP.bishops, wP.knights, wP.queens, wP.king},
                    {bP.pawns ^ initial, bP.rooks, bP.bishops, bP.knights, bP.queens ^ final, bP.king}, !white,
                    0, castlesStatus
                    };
            }
        }
        if constexpr (piece == 3) { // rook
            if constexpr (white) {
                return {
                    {wP.pawns ^ initial, wP.rooks ^ final, wP.bishops, wP.knights, wP.queens, wP.king},
                    {bP.pawns, bP.rooks, bP.bishops, bP.knights, bP.queens, bP.king}, !white,
                    0, castlesStatus
                    };
            } else {
                return {
                    {wP.pawns, wP.rooks, wP.bishops, wP.knights, wP.queens, wP.king},
                    {bP.pawns ^ initial, bP.rooks ^ final, bP.bishops, bP.knights, bP.queens, bP.king}, !white,
                    0, castlesStatus
                    };
            }
        }
        if constexpr (piece == 4) { // bishop
            if constexpr (white) {
                return {
                    {wP.pawns ^ initial, wP.rooks, wP.bishops ^ final, wP.knights, wP.queens, wP.king},
                    {bP.pawns, bP.rooks, bP.bishops, bP.knights, bP.queens, bP.king}, !white,
                    0, castlesStatus
                    };
            } else {
                return {
                    {wP.pawns, wP.rooks, wP.bishops, wP.knights, wP.queens, wP.king},
                    {bP.pawns ^ initial, bP.rooks, bP.bishops ^ final, bP.knights, bP.queens, bP.king}, !white,
                    0, castlesStatus
                    };
            }
        }
        if constexpr (piece == 5) { // knight
            if constexpr (white) {
                return {
                    {wP.pawns ^ initial, wP.rooks, wP.bishops, wP.knights ^ final, wP.queens, wP.king},
                    {bP.pawns, bP.rooks, bP.bishops, bP.knights, bP.queens, bP.king}, !white,
                    0, castlesStatus
                    };
            } else {
                return {
                    {wP.pawns, wP.rooks, wP.bishops, wP.knights, wP.queens, wP.king},
                    {bP.pawns ^ initial, bP.rooks, bP.bishops, bP.knights ^ final, bP.queens, bP.king}, !white,
                    0, castlesStatus
                    };
            }
        }
        return {Pieces(), Pieces(), 0, 0};
    }

    template<bool white, int piece>
    Board pawnPromoteCapture(Squares initial, Squares final) const {
        Squares nfinal = ~final;

        if constexpr (piece == 2) { // queen
            if constexpr (white) {
                return {
                    {wP.pawns ^ initial, wP.rooks, wP.bishops, wP.knights, wP.queens ^ final, wP.king},
                    {bP.pawns & nfinal, bP.rooks & nfinal, bP.bishops & nfinal, bP.knights & nfinal, bP.queens & nfinal, bP.king}, !white,
                    0, castlesStatus
                    };
            } else {
                return {
                    {wP.pawns & nfinal, wP.rooks & nfinal, wP.bishops & nfinal, wP.knights & nfinal, wP.queens & nfinal, wP.king},
                    {bP.pawns ^ initial, bP.rooks, bP.bishops, bP.knights, bP.queens ^ final, bP.king}, !white,
                    0, castlesStatus
                    };
            }
        }
        if constexpr (piece == 3) { // rook
            if constexpr (white) {
                return {
                    {wP.pawns ^ initial, wP.rooks ^ final, wP.bishops, wP.knights, wP.queens, wP.king},
                    {bP.pawns & nfinal, bP.rooks & nfinal, bP.bishops & nfinal, bP.knights & nfinal, bP.queens & nfinal, bP.king}, !white,
                    0, castlesStatus
                    };
            } else {
                return {
                    {wP.pawns & nfinal, wP.rooks & nfinal, wP.bishops & nfinal, wP.knights & nfinal, wP.queens & nfinal, wP.king},
                    {bP.pawns ^ initial, bP.rooks ^ final, bP.bishops, bP.knights, bP.queens, bP.king}, !white,
                    0, castlesStatus
                    };
            }
        }
        if constexpr (piece == 4) { // bishop
            if constexpr (white) {
                return {
                    {wP.pawns ^ initial, wP.rooks, wP.bishops ^ final, wP.knights, wP.queens, wP.king},
                    {bP.pawns & nfinal, bP.rooks & nfinal, bP.bishops & nfinal, bP.knights & nfinal, bP.queens & nfinal, bP.king}, !white,
                    0, castlesStatus
                    };
            } else {
                return {
                    {wP.pawns & nfinal, wP.rooks & nfinal, wP.bishops & nfinal, wP.knights & nfinal, wP.queens & nfinal, wP.king},
                    {bP.pawns ^ initial, bP.rooks, bP.bishops ^ final, bP.knights, bP.queens, bP.king}, !white,
                    0, castlesStatus
                    };
            }
        }
        if constexpr (piece == 5) { // knight
            if constexpr (white) {
                return {
                    {wP.pawns ^ initial, wP.rooks, wP.bishops, wP.knights ^ final, wP.queens, wP.king},
                    {bP.pawns & nfinal, bP.rooks & nfinal, bP.bishops & nfinal, bP.knights & nfinal, bP.queens & nfinal, bP.king}, !white,
                    0, castlesStatus
                    };
            } else {
                return {
                    {wP.pawns & nfinal, wP.rooks & nfinal, wP.bishops & nfinal, wP.knights & nfinal, wP.queens & nfinal, wP.king},
                    {bP.pawns ^ initial, bP.rooks, bP.bishops, bP.knights ^ final, bP.queens, bP.king}, !white,
                    0, castlesStatus
                    };
            }
        }
        return {Pieces(), Pieces(), 0, 0};
    }

    template<bool white>
    Board pawnEnPassantCapture(Squares initial, Squares final) const {
        if constexpr (white) {
            return {
                {wP.pawns ^ (initial | final), wP.rooks, wP.bishops, wP.knights, wP.queens, wP.king},
                {bP.pawns ^ (final >> 8), bP.rooks, bP.bishops, bP.knights, bP.queens, bP.king}, !white,
                0, castlesStatus
                };
        } else {
            return {
                {wP.pawns ^ (final << 8), wP.rooks, wP.bishops, wP.knights, wP.queens, wP.king},
                {bP.pawns ^ (initial | final), bP.rooks, bP.bishops, bP.knights, bP.queens, bP.king}, !white,
                0, castlesStatus
                };
        }
    }

    template<bool white, int piece>
    Board pieceMove(Squares initial, Squares final) const {
        if constexpr (piece == 2) { // queen
            if constexpr (white) {
                return {
                    {wP.pawns, wP.rooks, wP.bishops, wP.knights, wP.queens ^ (initial | final), wP.king},
                    {bP.pawns, bP.rooks, bP.bishops, bP.knights, bP.queens, bP.king}, !white,
                    0, castlesStatus
                    };
            } else {
                return {
                    {wP.pawns, wP.rooks, wP.bishops, wP.knights, wP.queens, wP.king},
                    {bP.pawns, bP.rooks, bP.bishops, bP.knights, bP.queens ^ (initial | final), bP.king}, !white,
                    0, castlesStatus
                    };
            }
        }
        if constexpr (piece == 3) { // rook
            if constexpr (white) {
                uint8_t newCastleStatus = castlesStatus;
                if (castlesStatus & 0b11) {
                    if (_tzcnt_u64(initial)==0) newCastleStatus &= 0b1;
                    if (_tzcnt_u64(initial)==7) newCastleStatus &= 0b10;
                }
                return {
                    {wP.pawns, wP.rooks ^ (initial | final), wP.bishops, wP.knights, wP.queens, wP.king},
                    {bP.pawns, bP.rooks, bP.bishops, bP.knights, bP.queens, bP.king}, !white,
                    0, newCastleStatus
                    };
            } else {
                uint8_t newCastleStatus = castlesStatus;
                if (castlesStatus & 0b1100) {
                    if (_tzcnt_u64(initial)==56) newCastleStatus &= 0b100;
                    if (_tzcnt_u64(initial)==63) newCastleStatus &= 0b1000;
                }
                return {
                    {wP.pawns, wP.rooks, wP.bishops, wP.knights, wP.queens, wP.king},
                    {bP.pawns, bP.rooks ^ (initial | final), bP.bishops, bP.knights, bP.queens, bP.king}, !white,
                    0, newCastleStatus
                    };
            }
        }
        if constexpr (piece == 4) { // bishop
            if constexpr (white) {
                return {
                    {wP.pawns, wP.rooks, wP.bishops ^ (initial | final), wP.knights, wP.queens, wP.king},
                    {bP.pawns, bP.rooks, bP.bishops, bP.knights, bP.queens, bP.king}, !white,
                    0, castlesStatus
                    };
            } else {
                return {
                    {wP.pawns, wP.rooks, wP.bishops, wP.knights, wP.queens, wP.king},
                    {bP.pawns, bP.rooks, bP.bishops ^ (initial | final), bP.knights, bP.queens, bP.king}, !white,
                    0, castlesStatus
                    };
            }
        }
        if constexpr (piece == 5) { // knight
            if constexpr (white) {
                return {
                    {wP.pawns, wP.rooks, wP.bishops, wP.knights ^ (initial | final), wP.queens, wP.king},
                    {bP.pawns, bP.rooks, bP.bishops, bP.knights, bP.queens, bP.king}, !white,
                    0, castlesStatus
                    };
            } else {
                return {
                    {wP.pawns, wP.rooks, wP.bishops, wP.knights, wP.queens, wP.king},
                    {bP.pawns, bP.rooks, bP.bishops, bP.knights ^ (initial | final), bP.queens, bP.king}, !white,
                    0, castlesStatus
                    };
            }
        }
        return {Pieces(), Pieces(), 0, 0};
    }

    template<bool white, int piece>
    Board pieceMoveCapture(Squares initial, Squares final) const {
        Squares nfinal = ~final;
        if constexpr (piece == 2) { // queen
            if constexpr (white) {
                return {
                    {wP.pawns, wP.rooks, wP.bishops, wP.knights, wP.queens ^ (initial | final), wP.king},
                    {bP.pawns & nfinal, bP.rooks & nfinal, bP.bishops & nfinal, bP.knights & nfinal, bP.queens & nfinal, bP.king}, !white,
                    0, castlesStatus
                    };
            } else {
                return {
                    {wP.pawns & nfinal, wP.rooks & nfinal, wP.bishops & nfinal, wP.knights & nfinal, wP.queens & nfinal, wP.king},
                    {bP.pawns, bP.rooks, bP.bishops, bP.knights, bP.queens ^ (initial | final), bP.king}, !white,
                    0, castlesStatus
                    };
            }
        }
        if constexpr (piece == 3) { // rook
            if constexpr (white) {
                uint8_t newCastleStatus = castlesStatus;
                if (castlesStatus & 0b11) {
                    if (_tzcnt_u64(initial)==0) newCastleStatus ^= 0b1;
                    if (_tzcnt_u64(initial)==7) newCastleStatus ^= 0b10;
                }
                return {
                    {wP.pawns, wP.rooks ^ (initial | final), wP.bishops, wP.knights, wP.queens, wP.king},
                    {bP.pawns & nfinal, bP.rooks & nfinal, bP.bishops & nfinal, bP.knights & nfinal, bP.queens & nfinal, bP.king}, !white,
                    0, newCastleStatus
                    };
            } else {
                uint8_t newCastleStatus = castlesStatus;
                if (castlesStatus & 0b1100) {
                    if (_tzcnt_u64(initial)==56) newCastleStatus ^= 0b100;
                    if (_tzcnt_u64(initial)==63) newCastleStatus ^= 0b1000;
                }
                return {
                    {wP.pawns & nfinal, wP.rooks & nfinal, wP.bishops & nfinal, wP.knights & nfinal, wP.queens & nfinal, wP.king},
                    {bP.pawns, bP.rooks ^ (initial | final), bP.bishops, bP.knights, bP.queens, bP.king}, !white,
                    0, newCastleStatus
                    };
            }
        }
        if constexpr (piece == 4) { // bishop
            if constexpr (white) {
                return {
                    {wP.pawns, wP.rooks, wP.bishops ^ (initial | final), wP.knights, wP.queens, wP.king},
                    {bP.pawns & nfinal, bP.rooks & nfinal, bP.bishops & nfinal, bP.knights & nfinal, bP.queens & nfinal, bP.king}, !white,
                    0, castlesStatus
                    };
            } else {
                return {
                    {wP.pawns & nfinal, wP.rooks & nfinal, wP.bishops & nfinal, wP.knights & nfinal, wP.queens & nfinal, wP.king},
                    {bP.pawns, bP.rooks, bP.bishops ^ (initial | final), bP.knights, bP.queens, bP.king}, !white,
                    0, castlesStatus
                    };
            }
        }
        if constexpr (piece == 5) { // knight
            if constexpr (white) {
                return {
                    {wP.pawns, wP.rooks, wP.bishops, wP.knights ^ (initial | final), wP.queens, wP.king},
                    {bP.pawns & nfinal, bP.rooks & nfinal, bP.bishops & nfinal, bP.knights & nfinal, bP.queens & nfinal, bP.king}, !white,
                    0, castlesStatus
                    };
            } else {
                return {
                    {wP.pawns & nfinal, wP.rooks & nfinal, wP.bishops & nfinal, wP.knights & nfinal, wP.queens & nfinal, wP.king},
                    {bP.pawns, bP.rooks, bP.bishops, bP.knights ^ (initial | final), bP.queens, bP.king}, !white,
                    0, castlesStatus
                    };
            }
        }
        return {Pieces(), Pieces(), 0, 0};
    }

    template<bool white>
    Board kingMoves(Squares initial, Squares final) const {
        if constexpr (white) {
            return {
                {wP.pawns, wP.rooks, wP.bishops, wP.knights, wP.queens, wP.king ^ (initial | final)},
                {bP.pawns, bP.rooks, bP.bishops, bP.knights, bP.queens, bP.king}, !white,
                0, castlesStatus & (uint8_t)0b1100U
                };
        } else {
            return {
                {wP.pawns, wP.rooks, wP.bishops, wP.knights, wP.queens, wP.king},
                {bP.pawns, bP.rooks, bP.bishops, bP.knights, bP.queens, bP.king ^ (initial | final)}, !white,
                0, castlesStatus & (uint8_t)0b0011U
                };
        }
    }

    template<bool white>
    Board kingMovesCapture(Squares initial, Squares final) const {
        Squares nfinal = ~final;
        if constexpr (white) {
            return {
                {wP.pawns, wP.rooks, wP.bishops, wP.knights, wP.queens, wP.king ^ (initial | final)},
                {bP.pawns & nfinal, bP.rooks & nfinal, bP.bishops & nfinal, bP.knights & nfinal, bP.queens & nfinal, bP.king}, !white,
                0, castlesStatus & (uint8_t)0b1100U
                };
        } else {
            return {
                {wP.pawns & nfinal, wP.rooks & nfinal, wP.bishops & nfinal, wP.knights & nfinal, wP.queens & nfinal, wP.king},
                {bP.pawns, bP.rooks, bP.bishops, bP.knights, bP.queens, bP.king ^ (initial | final)}, !white,
                0, castlesStatus & (uint8_t)0b0011U
                };
        }
    }

    template<bool white>
    Board castlesMoveL() const {
        if constexpr (white) {
            return {
                {wP.pawns, wP.rooks ^ 0x0000000000000009ULL, wP.bishops, wP.knights, wP.queens, wP.king ^ 0x0000000000000014ULL},
                {bP.pawns, bP.rooks, bP.bishops, bP.knights, bP.queens, bP.king}, !white,
                0, castlesStatus & (uint8_t)0b1100U
                };
        } else {
            return {
                {wP.pawns, wP.rooks, wP.bishops, wP.knights, wP.queens, wP.king},
                {bP.pawns, bP.rooks ^ 0x0900000000000000ULL, bP.bishops, bP.knights, bP.queens, bP.king ^ 0x1400000000000000ULL}, !white,
                0, castlesStatus & (uint8_t)0b0011U
                };
        }
    }

    template<bool white>
    Board castlesMoveR() const {
        if constexpr (white) {
            return {
                {wP.pawns, wP.rooks ^ 0x00000000000000a0ULL, wP.bishops, wP.knights, wP.queens, wP.king ^ 0x0000000000000050ULL},
                {bP.pawns, bP.rooks, bP.bishops, bP.knights, bP.queens, bP.king}, !white,
                0, castlesStatus & (uint8_t)0b1100U
                };
        } else {
            return {
                {wP.pawns, wP.rooks, wP.bishops, wP.knights, wP.queens, wP.king},
                {bP.pawns, bP.rooks ^ 0xa000000000000000ULL, bP.bishops, bP.knights, bP.queens, bP.king ^ 0x5000000000000000ULL}, !white,
                0, castlesStatus & (uint8_t)0b0011U
                };
        }
    }
};

// Board checkMate() {
//     return {Pieces(), Pieces(), 0, 0b10000000};
// }

// Board draw() {
//     return {Pieces(), Pieces(), 0, 0b01000000};
// }

            // return {
            //     {wP.pawns & nfinal, wP.rooks & nfinal, wP.bishops & nfinal, wP.knights & nfinal, wP.queens & nfinal, wP.king}
            //     {bP.pawns ^ (initial | final), bP.rooks, bP.bishops, bP.knights, bP.queens, bP.king}, 
            //     0, castlesStatus
            //     };