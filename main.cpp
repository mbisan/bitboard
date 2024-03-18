#include "bitboard.cpp"

#include <vector>
#include <string>
#include <cstdint>

Board parseFEN(const std::string& fen) {
    bool ep = 0;
    bool wL = 0;
    bool wR = 0;
    bool bL = 0;
    bool bR = 0;
    bool isWhite = false;

    Pieces white, black;
    uint64_t ep = 0;

    // Parsing pieces
    int row = 7, col = 0;

    uint64_t position = 0;
    uint64_t string_position = 0;
    for (auto c : fen) {
        string_position++;
        if (position > 64) break;
        if (c == '/') {
            row--;
            col = 0;
            continue;
        }
        if (isdigit(c)) {
            col += c - '0';
            position += c - '0';
        } else {
            uint64_t square = 1ULL << (row * 8 + col);
            switch (c) {
                case 'K':
                    white.k |= square;
                    break;
                case 'Q':
                    white.q |= square;
                    break;
                case 'R':
                    white.r |= square;
                    break;
                case 'B':
                    white.b |= square;
                    break;
                case 'N':
                    white.n |= square;
                    break;
                case 'P':
                    white.p |= square;
                    break;
                case 'k':
                    black.k |= square;
                    break;
                case 'q':
                    black.q |= square;
                    break;
                case 'r':
                    black.r |= square;
                    break;
                case 'b':
                    black.b |= square;
                    break;
                case 'n':
                    black.n |= square;
                    break;
                case 'p':
                    black.p |= square;
                    break;
                default:
                    break;
            }
            col++;
            position++;
        }
    }

    string_position++;
    if (fen[string_position] == 'w') {
        isWhite = true;
    }
    string_position+=2;

    while (fen[string_position] != ' ') {
        switch (fen[string_position])
        {
        case 'K':
            wL = true;
            break;
        case 'Q':
            wR = true;
            break;
        case 'k':
            bL = true;
            break;
        case 'q':
            bR = true;
            break;        
        default:
            break;
        }
        string_position++;
    }
    // ep TODO
    return {white, black, ep, {ep, wL, wR, bL, bR, isWhite}};
}

void displayBoard(const Board& board) {
    
}

void perft(int depth) {

}



int main() {

}