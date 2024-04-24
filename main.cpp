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

uint64_t perft(int depth, Board &initial, int printDepth) {

    uint64_t counts = 0;

    uint8_t currState = initial.state.stateToInt();

    auto moves = functionArray[initial.state.stateToInt()](initial);
    if (depth == printDepth-1) std::cout << depth << " - " << moves.size() << std::endl;
    if (depth==1) return moves.size();

    for (auto newpos : moves) {
        if (depth == printDepth) displayBoard(newpos);
        counts += perft(depth-1, newpos, printDepth);
    }

    return counts;
}

int charToCol(char c) {
    switch (c)
    {
    case 'a':
        return 0;
    case 'b':
        return 1;
    case 'c':
        return 2;
    case 'd':
        return 3;
    case 'e':
        return 4;
    case 'f':
        return 5;
    case 'g':
        return 6;
    case 'h':
        return 7;
    default:
        return 100;
    }
}

int charToRow(char c) {
    switch (c)
    {
    case '1':
        return 0;
    case '2':
        return 1;
    case '3':
        return 2;
    case '4':
        return 3;
    case '5':
        return 4;
    case '6':
        return 5;
    case '7':
        return 6;
    case '8':
        return 7;
    default:
        return 100;
    }
}

Squares parsePosition(std::string pos) {
    int row0 = charToRow(pos[1]);
    int col0 = charToCol(pos[0]);

    if (row0*8+col0 > 63) return 0;

    return 1ULL << (row0*8+col0);
}

void cli(void) {
    std::string initial = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w kqKQ ";
    // "rnbqkbnr/ppp1pppp/8/1B1p4/4P3/8/PPPP1PPP/RNBQK1NR b kqKQ "
    Board board = parseFEN(initial);
    std::string input;
    while (true) {
        getline(std::cin, input);

        switch (input[0])
        {
        case 'f': // fen string
            board = parseFEN(input.substr(2, input.size()-2));
            break;
        case 'r':
            board = parseFEN(initial);
            break;
        case 'm': // in this case, the move should be either initial square, // final square i.e. a4b4 no castles yet
            {
                Squares initialbit = parsePosition(input.substr(2, 2));
                Squares finalbit = parsePosition(input.substr(4, 2));

                Squares occupied = board.b.occupied() | board.w.occupied();

                // find piece
                if (board.state.isWhite) {

                    if ((board.w.occupied() & initialbit) && (~occupied & finalbit)) { // initial position is valid and final position is empty (not checked)!!
                        if (board.w.k & initialbit) {
                            board = board.pieceMove<KING>(initialbit|finalbit);
                            break;
                        } else if (board.w.q & initialbit) {
                            board = board.pieceMove<QUEEN>(initialbit|finalbit);
                            break;
                        } else if (board.w.b & initialbit) {
                            board = board.pieceMove<BISHOP>(initialbit|finalbit);
                            break;
                        } else if (board.w.r & initialbit) {
                            board = board.pieceMove<ROOK>(initialbit|finalbit);
                            break;
                        } else if (board.w.n & initialbit) {
                            board = board.pieceMove<KNIGHT>(initialbit|finalbit);
                            break;
                        } else if (board.w.p & initialbit) {
                            if ((initialbit >> 16) & finalbit) {
                                board = board.pawnPush(initialbit, initialbit|finalbit);
                                break;
                            }
                            board = board.pieceMove<PAWN>(initialbit|finalbit);
                            break;
                        }
                    } else if ((board.w.occupied() & initialbit) && (board.b.occupied() & finalbit)) { // initial position is valid and final position is occupied by enemy!!
                        if (board.w.k & initialbit) {
                            board = board.pieceMoveCapture<KING>(initialbit|finalbit);
                            break;
                        } else if (board.w.q & initialbit) {
                            board = board.pieceMoveCapture<QUEEN>(initialbit|finalbit);
                            break;
                        } else if (board.w.b & initialbit) {
                            board = board.pieceMoveCapture<BISHOP>(initialbit|finalbit);
                            break;
                        } else if (board.w.r & initialbit) {
                            board = board.pieceMoveCapture<ROOK>(initialbit|finalbit);
                            break;
                        } else if (board.w.n & initialbit) {
                            board = board.pieceMoveCapture<KNIGHT>(initialbit|finalbit);
                            break;
                        } else if (board.w.p & initialbit) {
                            board = board.pieceMoveCapture<PAWN>(initialbit|finalbit);
                            break;
                        }
                    } 
                } else { // black's turn

                    if ((board.b.occupied() & initialbit) && (~occupied & finalbit)) { // initial position is valid and final position is empty (not checked)!!
                        if (board.b.k & initialbit) {
                            board = board.pieceMove<KING>(initialbit|finalbit);
                            break;
                        } else if (board.b.q & initialbit) {
                            board = board.pieceMove<QUEEN>(initialbit|finalbit);
                            break;
                        } else if (board.b.b & initialbit) {
                            board = board.pieceMove<BISHOP>(initialbit|finalbit);
                            break;
                        } else if (board.b.r & initialbit) {
                            board = board.pieceMove<ROOK>(initialbit|finalbit);
                            break;
                        } else if (board.b.n & initialbit) {
                            board = board.pieceMove<KNIGHT>(initialbit|finalbit);
                            break;
                        } else if (board.b.p & initialbit) {
                            if ((initialbit << 16) & finalbit) {
                                board = board.pawnPush(initialbit, initialbit|finalbit);
                                break;
                            }
                            board = board.pieceMove<PAWN>(initialbit|finalbit);
                            break;
                        }
                    } else if ((board.b.occupied() & initialbit) && (board.w.occupied() & finalbit)) { // initial position is valid and final position is occupied by enemy!!
                        if (board.b.k & initialbit) {
                            board = board.pieceMoveCapture<KING>(initialbit|finalbit);
                            break;
                        } else if (board.b.q & initialbit) {
                            board = board.pieceMoveCapture<QUEEN>(initialbit|finalbit);
                            break;
                        } else if (board.b.b & initialbit) {
                            board = board.pieceMoveCapture<BISHOP>(initialbit|finalbit);
                            break;
                        } else if (board.b.r & initialbit) {
                            board = board.pieceMoveCapture<ROOK>(initialbit|finalbit);
                            break;
                        } else if (board.b.n & initialbit) {
                            board = board.pieceMoveCapture<KNIGHT>(initialbit|finalbit);
                            break;
                        } else if (board.b.p & initialbit) {
                            board = board.pieceMoveCapture<PAWN>(initialbit|finalbit);
                            break;
                        }
                    } 
                }

                break;
            }
        case 'p': // print
            displayBoard(board);
            break;
        case 'e': // eval, will be e (number)
            {   
                int perftn = atoi(input.substr(2, input.size()-2).c_str());
                std::cout << perft(perftn, board, perftn) << std::endl;
                break;
            }
        default:
            std::cout << "Ignore " << input << std::endl;
            break;
        }
    }
}



int main() {

    cli();

    return 0;
}