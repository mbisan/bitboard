#include <iostream>
#include <immintrin.h>
#include <array>
#include <vector>
#include <string>

#include "lookup.cpp"

#define pawnInitial(isWhite) (isWhite ? 1 : 6)
#define pawnAdvace(isWhite) (isWhite ? 8 : -8)

#define enPassantFile(isWhite) (isWhite ? 4 : 2)

#define pawnStartFile(isWhite) (isWhite ? 0x000000000000ff00ULL : 0x00ff000000000000ULL)
#define startFile(isWhite) (isWhite ? 0x00000000000000ffULL : 0xff00000000000000ULL)
#define pawnEP(isWhite) (isWhite ? 0x000000ff00000000ULL : 0x00000000ff000000ULL)

#define castleLcheck(isWhite) (isWhite ? 0x000000000000000cULL : 0x0c00000000000000ULL)
#define castleLocc(isWhite) (isWhite ? 0x000000000000000eULL : 0x0e00000000000000ULL)
#define castleRcheck(isWhite) (isWhite ? 0x0000000000000060ULL : 0x6000000000000000ULL)

// _blsr_u64(X) https://www.felixcloutier.com/x86/blsr sets the lowest set bit to 0 i.e. _blsr_u64(0b100100) = 0b100000
// _tzcnt_u64(X) https://www.felixcloutier.com/x86/blsr counts number of trailing zeros i.e. _tzcnt_u64(0b100100) = 2

// for(; X; X=_blsr_u64(X)) std::cout << _tzcnt_u64(X) << ", "; 0b1100100 > outputs 2, 5, 6

#define BitLoop(x) for(uint64_t temp=x; temp; temp=_blsr_u64(temp))
#define BitLoop2(x) for(uint64_t temp2=x; temp2; temp2=_blsr_u64(temp2))
#define bitFromSquare(x) _tzcnt_u64(x)

struct isInCheckResult {
    int checkCount;
    Squares checkMask;
    Squares kingCheckMask;
    Squares pinmaskHV;
    Squares pinmaskD;
    Squares enpassantpin;
};

struct Pieces {
    Squares pawns = 0;
    Squares rooks = 0;
    Squares bishops = 0;
    Squares knights = 0;
    Squares queens = 0;
    Squares king = 0;
    uint8_t enPassant = 0b00000000;
    uint8_t castles = 0b11; // (castles & 1) is true if can left castle, (castles & 2) if can right castle
    // bit pawnsID[64], rooksID[64], bishopsID[64], knightsID[64], queensID[64] {0};
};

struct Board {
    Pieces whitePieces;
    Pieces blackPieces;
    bool isWhite;
    uint8_t gameResult = 0;
};