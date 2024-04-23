#include "bitboard.cpp"

#include <vector>
#include <string>
#include <cstdint>
#include <iostream>

const char PIECES[13][4] { "♔", // index 0
    "♕", "♖", "♗", "♘", "♙", "♚", "♛", "♜", "♝", "♞", "♟", " " // index 12
};

void displaySquare(bool color, int piece) {
    if (color) {
        std::cout << "\033[43m" << PIECES[piece] << " \033[0m";  // White background
    } else {
        std::cout << "\033[46m" << PIECES[piece] << " \033[0m";  // Black background
    }
}


void print_uint64t(uint64_t num) {
    for (int row=7; row>=0; row--) {
        for (int col=0; col<8; col++) {
            if (num >> (8*row + col) & 0b1) std::cout << "X";
            else std::cout << "-";           
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

Board parseFEN(const std::string& fen) {
    bool ep = 0;
    bool wL = 0;
    bool wR = 0;
    bool bL = 0;
    bool bR = 0;
    bool isWhite = false;

    Pieces white = {0, 0, 0, 0, 0, 0};
    Pieces black = {0, 0, 0, 0, 0, 0};
    uint64_t enp = 0;

    // Parsing pieces
    int row = 7, col = 0;

    uint64_t position = 0;
    uint64_t string_position = 0;
    for (auto c : fen) {
        string_position++;
        if (position >= 64) break;
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
    return {white, black, enp, {ep, wL, wR, bL, bR, isWhite}};
}

void displayBoard(const Board& board) {
    Squares wocc = board.w.occupied();
    Squares bocc = board.b.occupied();
    Squares occupied = wocc | bocc;

    for (int row=7; row>=0; row--) {
        for (int col=0; col<8; col++) {
            Squares a = 0b1ULL << (8*row + col);

            bool background = (row+col)%2;
            if (bocc & a) { // found square occupied by white piece
                if (board.b.k & a) displaySquare(background, 6);
                if (board.b.q & a) displaySquare(background, 7);
                if (board.b.r & a) displaySquare(background, 8);
                if (board.b.b & a) displaySquare(background, 9);
                if (board.b.n & a) displaySquare(background, 10);
                if (board.b.p & a) displaySquare(background, 11);
            }
            else if (wocc & a) { // found square occupied by white piece
                if (board.w.k & a) displaySquare(background, 0);
                if (board.w.q & a) displaySquare(background, 1);
                if (board.w.r & a) displaySquare(background, 2);
                if (board.w.b & a) displaySquare(background, 3);
                if (board.w.n & a) displaySquare(background, 4);
                if (board.w.p & a) displaySquare(background, 5);
            }
            else {
                displaySquare(background, 12);
            }
        }
        std::cout << std::endl;
    }
    if (board.state.isWhite) std::cout << "w - ";
    else std::cout << "b - ";
    if (board.state.wL) std::cout << "Q";
    if (board.state.wR) std::cout << "K";
    if (board.state.bL) std::cout << "q";
    if (board.state.bR) std::cout << "k";
    std::cout << std::endl;
}

using FunctionPtr = std::vector<Board>(*)(Board&);

FunctionPtr functionArray[64] = {
    generateMoves<0, 0, 0, 0, 0, 0>,
    generateMoves<1, 0, 0, 0, 0, 0>,
    generateMoves<0, 1, 0, 0, 0, 0>,
    generateMoves<1, 1, 0, 0, 0, 0>,
    generateMoves<0, 0, 1, 0, 0, 0>,
    generateMoves<1, 0, 1, 0, 0, 0>,
    generateMoves<0, 1, 1, 0, 0, 0>,
    generateMoves<1, 1, 1, 0, 0, 0>,
    generateMoves<0, 0, 0, 1, 0, 0>,
    generateMoves<1, 0, 0, 1, 0, 0>,
    generateMoves<0, 1, 0, 1, 0, 0>,
    generateMoves<1, 1, 0, 1, 0, 0>,
    generateMoves<0, 0, 1, 1, 0, 0>,
    generateMoves<1, 0, 1, 1, 0, 0>,
    generateMoves<0, 1, 1, 1, 0, 0>,
    generateMoves<1, 1, 1, 1, 0, 0>,
    generateMoves<0, 0, 0, 0, 1, 0>,
    generateMoves<1, 0, 0, 0, 1, 0>,
    generateMoves<0, 1, 0, 0, 1, 0>,
    generateMoves<1, 1, 0, 0, 1, 0>,
    generateMoves<0, 0, 1, 0, 1, 0>,
    generateMoves<1, 0, 1, 0, 1, 0>,
    generateMoves<0, 1, 1, 0, 1, 0>,
    generateMoves<1, 1, 1, 0, 1, 0>,
    generateMoves<0, 0, 0, 1, 1, 0>,
    generateMoves<1, 0, 0, 1, 1, 0>,
    generateMoves<0, 1, 0, 1, 1, 0>,
    generateMoves<1, 1, 0, 1, 1, 0>,
    generateMoves<0, 0, 1, 1, 1, 0>,
    generateMoves<1, 0, 1, 1, 1, 0>,
    generateMoves<0, 1, 1, 1, 1, 0>,
    generateMoves<1, 1, 1, 1, 1, 0>,
    generateMoves<0, 0, 0, 0, 0, 1>,
    generateMoves<1, 0, 0, 0, 0, 1>,
    generateMoves<0, 1, 0, 0, 0, 1>,
    generateMoves<1, 1, 0, 0, 0, 1>,
    generateMoves<0, 0, 1, 0, 0, 1>,
    generateMoves<1, 0, 1, 0, 0, 1>,
    generateMoves<0, 1, 1, 0, 0, 1>,
    generateMoves<1, 1, 1, 0, 0, 1>,
    generateMoves<0, 0, 0, 1, 0, 1>,
    generateMoves<1, 0, 0, 1, 0, 1>,
    generateMoves<0, 1, 0, 1, 0, 1>,
    generateMoves<1, 1, 0, 1, 0, 1>,
    generateMoves<0, 0, 1, 1, 0, 1>,
    generateMoves<1, 0, 1, 1, 0, 1>,
    generateMoves<0, 1, 1, 1, 0, 1>,
    generateMoves<1, 1, 1, 1, 0, 1>,
    generateMoves<0, 0, 0, 0, 1, 1>,
    generateMoves<1, 0, 0, 0, 1, 1>,
    generateMoves<0, 1, 0, 0, 1, 1>,
    generateMoves<1, 1, 0, 0, 1, 1>,
    generateMoves<0, 0, 1, 0, 1, 1>,
    generateMoves<1, 0, 1, 0, 1, 1>,
    generateMoves<0, 1, 1, 0, 1, 1>,
    generateMoves<1, 1, 1, 0, 1, 1>,
    generateMoves<0, 0, 0, 1, 1, 1>,
    generateMoves<1, 0, 0, 1, 1, 1>,
    generateMoves<0, 1, 0, 1, 1, 1>,
    generateMoves<1, 1, 0, 1, 1, 1>,
    generateMoves<0, 0, 1, 1, 1, 1>,
    generateMoves<1, 0, 1, 1, 1, 1>,
    generateMoves<0, 1, 1, 1, 1, 1>,
    generateMoves<1, 1, 1, 1, 1, 1>,
};

uint64_t perft(int depth, Board &initial) {

    uint64_t counts = 0;

    uint8_t currState = initial.state.stateToInt();

    auto moves = functionArray[initial.state.stateToInt()](initial);
    // std::cout << depth << " - " << moves.size() << std::endl;
    if (depth==1) return moves.size();

    for (auto newpos : moves) {
        displayBoard(newpos);
        counts += perft(depth-1, newpos);
    }

    return counts;
}



int main() {
    std::string initial = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b kqKQ ";
    std::string kokolo = "rnbqkbnr/ppp1pppp/8/3p4/8/2P5/PP1PPPPP/RNBQKBNR w KQkq ";

    Board board = parseFEN(initial);
    displayBoard(board);
    std::cout << perft(2, board) << std::endl;
    std::cout << perft(3, board) << std::endl;
    std::cout << perft(4, board) << std::endl;

    return 0;
}