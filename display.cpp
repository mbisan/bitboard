#include "base.cpp"

Board positionToBoard(std::string position) {
    Board r; r.isWhite = false;
    int i=0;
    for (auto c: position) {
        switch (c)
        {
        case 'k':
            r.blackPieces.king |= positionToBit[i]; i++;
            break;
        case 'q':
            r.blackPieces.queens |= positionToBit[i]; i++;
            break;
        case 'b':
            r.blackPieces.bishops |= positionToBit[i]; i++;
            break;
        case 'r':
            r.blackPieces.rooks |= positionToBit[i]; i++;
            break;
        case 'p':
            r.blackPieces.pawns |= positionToBit[i]; i++;
            break;
        case 'n':
            r.blackPieces.knights |= positionToBit[i]; i++;
            break;
        case 'K':
            r.whitePieces.king |= positionToBit[i]; i++;
            break;
        case 'Q':
            r.whitePieces.queens |= positionToBit[i]; i++;
            break;
        case 'B':
            r.whitePieces.bishops |= positionToBit[i]; i++;
            break;
        case 'R':
            r.whitePieces.rooks |= positionToBit[i]; i++;
            break;
        case 'P':
            r.whitePieces.pawns |= positionToBit[i]; i++;
            break;
        case 'N':
            r.whitePieces.knights |= positionToBit[i]; i++;
            break;
        case '0':
            i+=0;
            break;
        case '1':
            i+=1;
            break;
        case '2':
            i+=2;
            break;
        case '3':
            i+=3;
            break;
        case '4':
            i+=4;
            break;
        case '5':
            i+=5;
            break;
        case '6':
            i+=6;
            break;
        case '7':
            i+=7;
            break;
        case '8':
            i+=8;
            break;
        case '9':
            i+=9;
            break;
        case 'W':
            r.isWhite = true;
            break;        
        default:
            break;
        }
    }
    return r;
}

std::string cInSetBits(uint64_t number, std::string c) {
    std::string res = "";
    for (; number; number=number>>1) {
        if (number&1ULL) res += c;
        else res += " ";
    }
    return res;
}

void printBoard(const Board &board) {
    auto wking = cInSetBits(board.whitePieces.king, "K");
    std::cout << wking;
    auto wqueen = cInSetBits(board.whitePieces.queens, "Q");
    auto wrook = cInSetBits(board.whitePieces.rooks, "R");
    auto wbishop = cInSetBits(board.whitePieces.bishops, "B");
    auto wknight = cInSetBits(board.whitePieces.knights, "N");
    auto wpawn = cInSetBits(board.whitePieces.pawns, "P");

    auto bking = cInSetBits(board.blackPieces.king, "k");
    auto bqueen = cInSetBits(board.blackPieces.queens, "q");
    auto brook = cInSetBits(board.blackPieces.rooks, "r");
    auto bbishop = cInSetBits(board.blackPieces.bishops, "b");
    auto bknight = cInSetBits(board.blackPieces.knights, "n");
    auto bpawn = cInSetBits(board.blackPieces.pawns, "p");

    for (int i=0; i<8; i++) {
        for (int j=0; j<8; j++) {
            if (wking[8*i+j] != ' ') std::cout << wking[8*i+j]; continue;
            if (wqueen[8*i+j] != ' ') std::cout << wqueen[8*i+j]; continue;
            if (wrook[8*i+j] != ' ') std::cout << wrook[8*i+j]; continue;
            if (wbishop[8*i+j] != ' ') std::cout << wbishop[8*i+j]; continue;
            if (wknight[8*i+j] != ' ') std::cout << wknight[8*i+j]; continue;
            if (wpawn[8*i+j] != ' ') std::cout << wpawn[8*i+j]; continue;
            if (bking[8*i+j] != ' ') std::cout << bking[8*i+j]; continue;
            if (bqueen[8*i+j] != ' ') std::cout << bqueen[8*i+j]; continue;
            if (brook[8*i+j] != ' ') std::cout << brook[8*i+j]; continue;
            if (bbishop[8*i+j] != ' ') std::cout << bbishop[8*i+j]; continue;
            if (bknight[8*i+j] != ' ') std::cout << bknight[8*i+j]; continue;
            if (bpawn[8*i+j] != ' ') std::cout << bpawn[8*i+j]; continue;
            std::cout << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}