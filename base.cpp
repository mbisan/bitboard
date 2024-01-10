#include <iostream>
#include <immintrin.h>
#include <array>
#include <vector>
#include <string>

#include "lookup.cpp"

#define pawnInitial(isWhite) (isWhite ? 1 : 6)
#define pawnAdvace(isWhite) (isWhite ? 8 : -8)

// _blsr_u64(X) https://www.felixcloutier.com/x86/blsr sets the lowest set bit to 0 i.e. _blsr_u64(0b100100) = 0b100000
// _tzcnt_u64(X) https://www.felixcloutier.com/x86/blsr counts number of trailing zeros i.e. _tzcnt_u64(0b100100) = 2

// for(; X; X=_blsr_u64(X)) std::cout << _tzcnt_u64(X) << ", "; 0b1100100 > outputs 2, 5, 6

#define BitLoop(x) for(uint64_t temp=x; temp; temp=_blsr_u64(temp))
#define bitFromSquare(x) _tzcnt_u64(x)

struct isInCheckResult {
    int checkCount;
    Squares checkmask;
    Squares pinmaskHV;
    Squares pinmaskD;
};

struct Pieces {
    Squares pawns = 0;
    Squares rooks = 0;
    Squares bishops = 0;
    Squares knights = 0;
    Squares queens = 0;
    Squares king = 0;
    uint8_t enPassant = 0b00000000;
    bool castleR = true;
    bool castleL = true;
    // bit pawnsID[64], rooksID[64], bishopsID[64], knightsID[64], queensID[64] {0};
};

struct Board {
    Pieces whitePieces, blackPieces;
    bool isWhite;
};