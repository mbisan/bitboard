#include "bitboard.cpp"

#include <vector>
#include <string>
#include <cstdint>

Board parseFEN(const std::string& fen) {
    bool ep;
    bool wL;
    bool wR;
    bool bL;
    bool bR;
    bool isWhite;

    Pieces white, black;
    uint64_t ep = 0;

    // Parsing pieces
    int row = 7, col = 0;

    uint64_t position = 
    for (auto &ch: fen) {
        if (isdigit(ch)) {

        }
    }
    for (char& c : tokens[0]) {
        if (c == '/') {
            row--;
            col = 0;
            continue;
        }
        if (isdigit(c)) {
            col += c - '0';
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
        }
    }

    // Parsing side to move
    // tokens[1] should contain either 'w' or 'b'
    // Ignoring this for now, as it's not directly used in the board representation provided

    // Parsing castling availability
    // tokens[2] contains castling availability
    // Ignoring this for now, as it's not directly used in the board representation provided

    // Parsing en passant target square
    if (tokens[3] != "-") {
        int file = tokens[3][0] - 'a';
        int rank = 8 - (tokens[3][1] - '0');
        ep = 1ULL << (rank * 8 + file);
    }

    // Parsing halfmove clock and fullmove number
    // Ignoring these for now, as they're not directly used in the board representation provided

    return {white, black, ep, state};
}


void perft(int depth) {

}



int main() {

}