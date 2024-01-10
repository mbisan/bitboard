#include <iostream>
#include <map>
#include <string>

#include <math.h>

const char PIECES[13][4] { "♔", // index 0
    "♕", "♖", "♗", "♘", "♙", "♚", "♛", "♜", "♝", "♞", "♟", " " // index 12
};

void copy(uint64_t source[], uint64_t dest[], int n) {
    for (int i=0; i<n; i++) {
        dest[i] = source[i];
    }
}

int countBits(uint64_t number) {
    uint64_t copy = number;
    int count = 0;
    for (int i=0; i<64; i++){
        if (copy & 1) {
            count++;
        }
        copy = copy >> 1;
    }
    return count;
}

void print_int64(uint64_t number) {
    uint64_t copy = number;
    for (int i=0; i<64; i++){
        if (copy & 1) {
            std::cout << "1";
        } else {
            std::cout << "0";
        }
        copy = copy >> 1;
    }
    std::cout << std::endl;
}

std::map<std::string, uint64_t> fill_positions() {
    std::map<std::string, uint64_t> positions;
    uint64_t one = 1;
    for(int i=0; i<8; i++){
        for (int j=0; j<8; j++){
            char col = 'a' + j;
            positions[col + std::to_string(i+1)] = (one << (8*i+j));
        }
    }
    return positions;
}

std::map<char, int8_t> fill_piece_id() {
    std::map<char, int8_t> piece_id;

    piece_id['K'] = 0; piece_id['Q'] = 1; piece_id['R'] = 2;
    piece_id['B'] = 3; piece_id['N'] = 4; piece_id['P'] = 5;

    return piece_id;
}

std::string idtoPos(int id) {
    int i = id/8; int j = id % 8;
    char col = 'a' + j;
    return col + std::to_string(i+1);
}

void printPos(uint64_t pos) {
    uint64_t copy = pos;
    for (int i=0; i<64; i++){
        if (copy & 1) {
            std::cout << idtoPos(i);
        }
        copy = copy >> 1;
    }
    std::cout << std::endl;
}

void display_int64(uint64_t position) {
    std::cout << std::endl;
    uint64_t copy = position;
    for (int row = 7; row >=0; --row) {
        for (int col = 0; col < 8; ++col) {
            int shift = row*8+col;
            if ((position >> shift) & 1) {
                std::cout << "x";
            } else {
                std::cout << "0";
            }
        }
        std::cout << std::endl;  // Move to the next line after each row
    }
}

uint64_t pawnReachable(int position, int color) {
    uint64_t one = 1;
    uint64_t notAFile = 0xfefefefefefefefe;
    uint64_t notHFile = 0x7f7f7f7f7f7f7f7f;
    uint64_t reachable = one << position;
    if (color) {
        reachable |= ((reachable & notHFile) << 9) | ((reachable & notAFile) << 7);
    } else {
        reachable |= ((reachable & notAFile) >> 9) | ((reachable & notHFile) >> 7);
    }
    return reachable ^ (one << position);
}

uint64_t rookReachable(int position, int color) {
    uint64_t one = 1;
    uint64_t row_bits = 255;
    uint64_t col_bits = 1 + (one << 8) + (one << 16) + (one << 24) + (one << 32) + (one << 40) + (one << 48) + (one << 56);
    int col = position % 8; int row = position / 8;
    uint64_t reachable = 0;
    for (int i=0; i<8; i++) {
        reachable = reachable | row_bits << (8*row);
        reachable = reachable | col_bits << col;
    }
    return reachable ^ (one << position);
}

uint64_t bishopReachable(int position, int color) {
    int col = position % 8; int row = position / 8;
    int min;
    uint64_t reachable = 0;
    uint64_t one = 1;
    uint64_t a1h8 = (one << 0) + (one << 9) + (one << 18) + (one << 27)
                  + (one << 36) + (one << 45) + (one << 54) + (one << 63);
    uint64_t a8h1 = (one << 7) + (one << 14) + (one << 21) + (one << 28)
                  + (one << 35) + (one << 42) + (one << 49) + (one << 56);
    if (col <= row) {
        int move = row-col;
        reachable = reachable | (a1h8 << (8*move));
    } else {
        int move = col-row;
        reachable = reachable | (a1h8 >> (8*move));
    }

    if ((7-col <= row)) {
        int move = row -7 + col;
        reachable = reachable | (a8h1 << (8*move));
    } else {
        int move = row +7 - col;
        reachable = reachable | (a8h1 >> (8*move));
    }

    return reachable ^ (one << position);
}

uint64_t queenReachable(int position, int color) {
    return rookReachable(position, color) | bishopReachable(position, color);
}

uint64_t kingReachable(int position, int color) {
    uint64_t one = 1;
    uint64_t row_bits = 255;
    uint64_t notAFile = 0xfefefefefefefefe;
    uint64_t notHFile = 0x7f7f7f7f7f7f7f7f;
    uint64_t not1Row = 0xffffffffffffff00;
    uint64_t not8Row = 0x00ffffffffffffff;

    uint64_t reachable = 1ULL << position;
    reachable |= ((reachable & not1Row) >> 8) | ((reachable & not8Row) << 8) | 
                 ((reachable & notHFile) << 1) | ((reachable & notAFile) >> 1) |
                 ((reachable & not1Row & notAFile) >> 9) | ((reachable & not1Row & notHFile) >> 7) |  // south east, west
                 ((reachable & not8Row & notHFile) << 9) | ((reachable & not8Row & notAFile) << 7);  // north east, west

    return reachable;
}

void generateKnightMoves(uint64_t *movements) {
    uint64_t one = 1;
    for (int row=0; row<8; row++) {
        for (int col=0; col<8; col++) {
            int id = 8*row+col; 
            movements[id] = 0;
            
            int row_indices[8] {row - 2, row - 2, row - 1, row - 1, row + 1, row + 1, row + 2, row + 2};
            int col_indices[8] {col - 1, col + 1, col - 2, col + 2, col - 2, col + 2, col - 1, col + 1};

            for (int k=0; k<8; k++) {
                if ((row_indices[k]>=0) && (row_indices[k]<8) && (col_indices[k]>=0) && (col_indices[k]<8)) {
                    movements[id] |= one << (8*row_indices[k] + col_indices[k]);
                }
            }
        }
    }
}

void generateRookMoves(uint64_t *movements) {
    uint64_t one = 1;
    for (int i=0; i<64; i++) {
        movements[i] = rookReachable(i, 1);
    }
}

void generateBishopMoves(uint64_t *movements) {
    uint64_t one = 1;
    for (int i=0; i<64; i++) {
        movements[i] = bishopReachable(i, 1);
    }
}

void generateQueenMoves(uint64_t *movements) {
    uint64_t one = 1;
    for (int i=0; i<64; i++) {
        movements[i] = queenReachable(i, 1);
    }
}

void generateKingMoves(uint64_t *movements) {
    uint64_t one = 1;
    for (int i=0; i<64; i++) {
        movements[i] = kingReachable(i, 1);
    }
}

void generateWhitePawnMoves(uint64_t *movements) {
    uint64_t one = 1;
    for (int i=0; i<64; i++) {
        movements[i] = pawnReachable(i, 1);
    }
}

void generateBlackPawnMoves(uint64_t *movements) {
    uint64_t one = 1;
    for (int i=0; i<64; i++) {
        movements[i] = pawnReachable(i, 0);
    }
}

void generatePositionBits(uint64_t *positions) {
    for (int i=0;i<64;i++) {
        positions[i] = 1ULL << i;
    }
}

class Chessboard {
public:
    // each bit indicates whether the position is populated with a piece
    // white_pieces[0] indicates the position of the white king
    uint64_t white_pieces[6] = {0}; 
    uint64_t black_pieces[6] = {0};

    uint64_t *pieces[2] {white_pieces, black_pieces};

    std::map<std::string, uint64_t> positions = fill_positions();
    std::map<char, int8_t> piece_id = fill_piece_id();

    uint64_t one = 1;

    uint64_t knightMoves[64];
    uint64_t bishopMoves[64];
    uint64_t queenMoves[64];
    uint64_t rookMoves[64];
    uint64_t kingMoves[64];
    uint64_t whitePawnMoves[64];
    uint64_t blackPawnMoves[64];

    uint64_t positionBits[64];

    uint64_t *pawnMoves[2] {whitePawnMoves, blackPawnMoves};
    // uint64_t *whiteMoves[6] {kingMoves, queenMoves, rookMoves, bishopMoves, knightMoves, whitePawnMoves};
    // uint64_t *blackMoves[6] {kingMoves, queenMoves, rookMoves, bishopMoves, knightMoves, blackPawnMoves};
    // uint64_t **moves[2] {whiteMoves, blackMoves};

    uint8_t lastMove[4] {255};

    int allowsEnPassant = 255;
    bool canoocastle[2] = {true};
    bool canooocastle[2] = {true};
    
void put_piece(const char* pos, const char piece, bool color) {
    if (color) {
        white_pieces[piece_id[piece]] = white_pieces[piece_id[piece]] | positions[pos];
    } else {
        black_pieces[piece_id[piece]] = black_pieces[piece_id[piece]] | positions[pos];
    }
}

void startingPosition() {
    // put white pieces
    put_piece("a1", 'R', 1); put_piece("a2", 'P', 1);
    put_piece("b1", 'N', 1); put_piece("b2", 'P', 1);
    put_piece("c1", 'B', 1); put_piece("c2", 'P', 1);
    put_piece("d1", 'Q', 1); put_piece("d2", 'P', 1);
    put_piece("e1", 'K', 1); put_piece("e2", 'P', 1);
    put_piece("f1", 'B', 1); put_piece("f2", 'P', 1);
    put_piece("g1", 'N', 1); put_piece("g2", 'P', 1);
    put_piece("h1", 'R', 1); put_piece("h2", 'P', 1);

    // put black pieces
    put_piece("a8", 'R', 0); put_piece("a7", 'P', 0);
    put_piece("b8", 'N', 0); put_piece("b7", 'P', 0);
    put_piece("c8", 'B', 0); put_piece("c7", 'P', 0);
    put_piece("d8", 'Q', 0); put_piece("d7", 'P', 0);
    put_piece("e8", 'K', 0); put_piece("e7", 'P', 0);
    put_piece("f8", 'B', 0); put_piece("f7", 'P', 0);
    put_piece("g8", 'N', 0); put_piece("g7", 'P', 0);
    put_piece("h8", 'R', 0); put_piece("h7", 'P', 0);
}

Chessboard() {
    generateBishopMoves(bishopMoves);
    generateBlackPawnMoves(blackPawnMoves);
    generateWhitePawnMoves(whitePawnMoves);
    generateKingMoves(kingMoves);
    generateQueenMoves(queenMoves);
    generateKnightMoves(knightMoves);
    generateRookMoves(rookMoves);
    generatePositionBits(positionBits);
}

uint64_t occupiedSquares() {
    uint64_t occupied = 0;
    for (int i=0; i<6; i++) {
        occupied |= pieces[0][i];
        occupied |= pieces[1][i];
    }
    return occupied;
}

uint64_t whitePiecesSquares() {
    uint64_t occupied = 0;
    for (int i=0; i<6; i++) {
        occupied |= pieces[0][i];
    }
    return occupied;
}

uint64_t blackPiecesSquares() {
    uint64_t occupied = 0;
    for (int i=0; i<6; i++) {
        occupied |= pieces[1][i];
    }
    return occupied;
}

uint64_t piecesSquares(bool color) {
    if (color) {
        return whitePiecesSquares();
    }
    return blackPiecesSquares();
}

uint64_t bishopAttacking(int position, bool color) {
    uint64_t reachable = 0;

    uint8_t r = position/8; uint8_t c = position%8;

    uint64_t self;
    uint64_t other;
    if (color) {
        self = whitePiecesSquares();
        other = blackPiecesSquares();
    } else {
        other = whitePiecesSquares();
        self = blackPiecesSquares();
    }

    int row = r; int col = c;
    uint64_t newpos;
    while (row<7 && col<7) {
        row++; col++;
        newpos = 1ULL << (row*8 + col);
        if ((newpos & self) > 0) { // found self piece in diagonal
            break;
        }
        reachable |= newpos;
        if ((newpos & other) > 0) { // found other piece in diagonal
            break;
        }
    }

    row = r; col = c;
    newpos;
    while (row<7 && col>0) {
        row++; col--;
        newpos = 1ULL << (row*8 + col);
        if ((newpos & self) > 0) { // found self piece in diagonal
            break;
        }
        reachable |= newpos;
        if ((newpos & other) > 0) { // found other piece in diagonal
            break;
        }
    }

    row = r; col = c;
    newpos;
    while (row>0 && col<7) {
        row--; col++;
        newpos = 1ULL << (row*8 + col);
        if ((newpos & self) > 0) { // found self piece in diagonal
            break;
        }
        reachable |= newpos;
        if ((newpos & other) > 0) { // found other piece in diagonal
            break;
        }
    }

    row = r; col = c;
    newpos;
    while (row>0 && col>0) {
        row--; col--;
        newpos = 1ULL << (row*8 + col);
        if ((newpos & self) > 0) { // found self piece in diagonal
            break;
        }
        reachable |= newpos;
        if ((newpos & other) > 0) { // found other piece in diagonal
            break;
        }
    }

    return reachable;
}

uint64_t rookAttacking(int position, bool color) {
    uint64_t reachable = 0;

    uint8_t r = position/8; uint8_t c = position%8;

    uint64_t self;
    uint64_t other;
    if (color) {
        self = whitePiecesSquares();
        other = blackPiecesSquares();
    } else {
        other = whitePiecesSquares();
        self = blackPiecesSquares();
    }

    int row = r; int col = c;
    uint64_t newpos;
    while (row<7) {
        row++;
        newpos = 1ULL << (row*8 + col);
        if ((newpos & self) > 0) { // found self piece in diagonal
            break;
        }
        reachable |= newpos;
        if ((newpos & other) > 0) { // found other piece in diagonal
            break;
        }
    }

    row = r; col = c;
    newpos;
    while (col<7) {
        col++;
        newpos = 1ULL << (row*8 + col);
        if ((newpos & self) > 0) { // found self piece in diagonal
            break;
        }
        reachable |= newpos;
        if ((newpos & other) > 0) { // found other piece in diagonal
            break;
        }
    }

    row = r; col = c;
    newpos;
    while (row>0) {
        row--;
        newpos = 1ULL << (row*8 + col);
        if ((newpos & self) > 0) { // found self piece in diagonal
            break;
        }
        reachable |= newpos;
        if ((newpos & other) > 0) { // found other piece in diagonal
            break;
        }
    }

    row = r; col = c;
    newpos;
    while (col>0) {
        col--;
        newpos = 1ULL << (row*8 + col);
        if ((newpos & self) > 0) { // found self piece in diagonal
            break;
        }
        reachable |= newpos;
        if ((newpos & other) > 0) { // found other piece in diagonal
            break;
        }
    }

    return reachable;
}

uint64_t queenAttacking(int position, bool color) {
    return bishopAttacking(position, color) | rookAttacking(position, color);
}

uint64_t knightAttacking(int position, bool color) {
    uint64_t reachable = 0;

    uint64_t self;
    if (color) {
        self = whitePiecesSquares();
    } else {
        self = blackPiecesSquares();
    }

    return knightMoves[position] & (~self);
}

uint64_t kingAttacking(int position, bool color) {
    uint64_t reachable = 0;

    uint64_t self;
    if (color) {
        self = whitePiecesSquares();
    } else {
        self = blackPiecesSquares();
    }

    return kingMoves[position] & (~self);
}

uint64_t pawnAttacking(int position, bool color) {
    uint64_t reachable = 0;

    int col = 1;
    if (color) col=0;

    uint64_t self;
    if (color) {
        self = whitePiecesSquares();
    } else {
        self = blackPiecesSquares();
    }

    return pawnMoves[col][position] & (~self);
}

void display() {
    for (int row = 7; row >=0; --row) {
        std::cout << row + 1 << " ";
        for (int col = 0; col < 8; ++col) {
            int shift = row*8+col;
            int squareColor = (row + col) % 2;  // 0 for white, 1 for black

            for(int i=0; i<7; i++) {
                if (i>=6) {
                    displaySquare(squareColor, 12);
                    break;
                } else if ((white_pieces[i] >> shift) & one) { // i.e. found piece
                    displaySquare(squareColor, i);
                    break;
                } else if ((black_pieces[i] >> shift) & one) {
                    displaySquare(squareColor, i + 6);
                    break;
                }
            }

        }
        std::cout << std::endl;  // Move to the next line after each row
    }
    std::cout << "   a b c d e f g h" << std::endl;
}

void displaySquare(bool color, int piece) {
    if (color) {
        std::cout << "\033[43m" << PIECES[piece] << " \033[0m";  // White background
    } else {
        std::cout << "\033[46m" << PIECES[piece] << " \033[0m";  // Black background
    }
}

bool checkKingInCheck(bool color) {
    int self=1; int other=0; 
    if (color) {
        self=0; other=1; 
    }

    uint64_t kingposition = pieces[self][0];
    for (int i=1; i<6; i++) { // iterate through enemy's pieces (except king)
        uint64_t copy = pieces[other][i];
        if (copy==0) continue;
        for (int j=0; j<64; j++) {
            if (copy & 1) { // found the piece
                uint64_t attacked;
                switch (i)
                {
                case 1:
                    attacked = queenAttacking(j, self);
                    break;
                case 2:
                    attacked = rookAttacking(j, self);
                    break;
                case 3:
                    attacked = bishopAttacking(j, self);
                    break;
                case 4:
                    attacked = knightAttacking(j, self);
                    break;
                case 5:
                    attacked = pawnAttacking(j, self);
                    break;
                default:
                    break;
                }
                if (attacked & kingposition) return true;
            }
            copy = copy >> 1;
        }
    }
    return false;
}

bool movePawn(int initial, int final, bool color, int promote) {
    // pawn can only move to enemy occupied pieces or in a straight line
    // and if the move does not leave the king in check
    int self = 1; int other = 0;
    if (color) {
        self = 0; other = 1;
    }

    int row = initial / 8; int col = initial % 8;
    std::cout << "Moving pawn" << row << col << std::endl;

    // piece not found in the initial square
    if ((pieces[self][5] & positionBits[initial]) == 0) return false;

    uint64_t selftemp[6]; copy(pieces[self], selftemp, 6); // copy previous state
    uint64_t othertemp[6]; copy(pieces[other], othertemp, 6); // copy previous state

    uint64_t reachable = pawnAttacking(initial, color) ;

    uint64_t otherpieces;
    if (color) otherpieces = blackPiecesSquares();
    else otherpieces = whitePiecesSquares();

    reachable &= otherpieces; // pawn can only move to attacked squares if those are occupied by enemy pieces

    // handle en passant
    if ((allowsEnPassant - 1) == col || ((allowsEnPassant + 1) == col)) {
        if (color && row==4) {
            reachable |= positionBits[(row+1)*8 + allowsEnPassant]; 
        } else if (!color && row==3) {
            reachable |= positionBits[(row-1)*8 + allowsEnPassant]; 
        }
    }

    // handle move forward and initial movement
    uint64_t occupied = occupiedSquares();

    if (color && (row < 7)) reachable |= positionBits[(row+1)*8+col] & (~occupied);
    if (color && (row == 1)) reachable |= positionBits[(row+2)*8+col] & (~occupied);
    
    if (!color && (row >=0)) reachable |= positionBits[(row-1)*8+col] & (~occupied);
    if (!color && (row == 6)) reachable |= positionBits[(row-2)*8+col] & (~occupied);

    // if finalposition not reachable
    if ((reachable & positionBits[final]) == 0) return false; 

    // handle promotion
    if ((color && (final>=56)) || (!color && (final < 8))) {
        pieces[self][promote] |= positionBits[final];
        pieces[self][5] ^= positionBits[initial];
    } else {
        pieces[self][5] ^= positionBits[final] | positionBits[initial];
    }

    // final square occupied by other piece
    for (int i=0; i<6; i++) {
        pieces[other][i] &= ~positionBits[final];
    }

    if (checkKingInCheck(color)) { // piece can only move if king is left in unattacked squares
        // restore state
        copy(selftemp, pieces[self], 6); copy(othertemp, pieces[other], 6);
        return false;
    }

    int diff = initial - final;
    if (diff > 10 || diff < -10) allowsEnPassant = col;
    else allowsEnPassant = 255;

    return true;
}

bool movePiece(int piece, int initial, int final, bool color, int promote) {
    if (piece>=6 || piece<0) return false;
    if (piece==5) {
        bool result = movePawn(initial, final, color, promote);
        return result;
    }

    int self = 1; int other = 0;
    if (color) {
        self = 0; other = 1;
    }

    // piece not found in the initial square
    if ((pieces[self][piece] & positionBits[initial]) == 0) return false;

    uint64_t selftemp[6]; copy(pieces[self], selftemp, 6); // copy previous state
    uint64_t othertemp[6]; copy(pieces[other], othertemp, 6); // copy previous state

    uint64_t reachable;
    switch (piece)
    {
    case 0:
        reachable = kingAttacking(initial, color);
        break;
    case 1:
        reachable = queenAttacking(initial, color);
        break;
    case 2:
        reachable = rookAttacking(initial, color);
        break;
    case 3:
        reachable = bishopAttacking(initial, color);
        break;
    case 4:
        reachable = knightAttacking(initial, color);
        break;
    case 5:
        reachable = pawnAttacking(initial, color);
        break;
    default:
        break;
    }

    // if finalposition not reachable
    if ((reachable & positionBits[final]) == 0) return false; 

    pieces[self][piece] ^= positionBits[final] | positionBits[initial];

    // final square occupied by other piece
    for (int i=0; i<6; i++) {
        pieces[other][i] &= ~positionBits[final];
    }

    if (checkKingInCheck(color)) { // piece can only move if king is left in unattacked squares
        // restore state
        copy(selftemp, pieces[self], 6); copy(othertemp, pieces[other], 6);
        return false;
    }

    allowsEnPassant = 255;
    if (piece == 0) {
        canoocastle[self] = false; canooocastle[self] = false;
    } else if (piece == 2 && (initial == 7 || initial == 63)) {
        canoocastle[self] = false;
    } else if (piece == 2 && (initial == 0 || initial == 56)) {
        canooocastle[self] = false;
    }

    return true;
}

// bool checkCheckmate() {

// }

bool movePieceFromString(std::string move, bool color) {
    int self = 1; int other = 0;
    if (color) {
        self = 0; other = 1;
    }
    bool result;

    if (move.length() == 2) { // pawn move (for example a3)
        char row = move[1]; char col = move[0];
        int rowid = row - (int)'0'; int colid = col - (int)'a';

        if (rowid > 8 || rowid <=0 || colid > 8 || colid <=0) return false;

        if (color) result = movePiece(5, 8*(rowid - 1)+colid, 8*rowid+colid, color, 1);
        else result = movePiece(5, 8*(rowid + 1)+colid, 8*rowid+colid, color, 0);

        return result;
    } else if (move.length() == 3) { // piece move (Kd2) i.e. king moves to e2, must be unambiguous
        if (!move.compare("O-O")) {
            if (!canoocastle[self]) return false;

            result = true;
            uint64_t selftemp[6]; copy(pieces[self], selftemp, 6); // copy previous state
            uint64_t othertemp[6]; copy(pieces[other], othertemp, 6); // copy previous state

            if (color) {
                result &= movePiece(0, 4, 5, color, 0);
                result &= movePiece(0, 5, 6, color, 0);
                result &= movePiece(2, 7, 5, color, 0);
            } else {
                result &= movePiece(0, 7*8+4, 7*8+5, color, 0);
                result &= movePiece(0, 7*8+5, 7*8+6, color, 0);
                result &= movePiece(2, 7*8+7, 7*8+5, color, 0);
            }
            if (!result) {
                copy(selftemp, pieces[self], 6); copy(othertemp, pieces[other], 6);
            }
            return result;
        }

        char row = move[2]; char col = move[1];
        int rowid = row - (int)'0'; int colid = col - (int)'a';

        char piece = move[0];
        if (!std::isupper(piece)) { // pawn capture
            int initialcol = piece - (int)'a';
            if (color) result = movePiece(5, 8*(rowid - 1)+initialcol, 8*rowid+colid, color, 1);
            else result = movePiece(5, 8*(rowid + 1)+initialcol, 8*rowid+colid, color, 0);

            return result;
        }

        uint64_t reachable;
        // find piece starting position
        int initialposition;
        uint64_t possiblepieces = pieces[self][piece_id[piece]];
        for (int i=0; i<64; i++) {
            if (possiblepieces & 1) {
                switch (piece)
                {
                case 'K':
                    reachable = kingAttacking(i, color);
                    break;
                case 'Q':
                    reachable = queenAttacking(i, color);
                    break;
                case 'R':
                    reachable = rookAttacking(i, color);
                    break;
                case 'B':
                    reachable = bishopAttacking(i, color);
                    break;
                case 'N':
                    reachable = knightAttacking(i, color);
                    break;
                default:
                    break;
                }
                if ((reachable & positionBits[8*row+col]) != 0) {
                    initialposition = i; break;
                }
            }
        }

        if (color) result = movePiece(piece_id[piece], initialposition, 8*rowid+colid, color, 1);
        else result = movePiece(piece_id[piece], initialposition, 8*rowid+colid, color, 0);

        return result;
    } else if (move.length() == 4) { // disanbiguous Q1d8 i.e. queen on row 1 or Qad8 queen on col a
        char row = move[3]; char col = move[2];
        char piece = move[0]; char pieceidentifier = move[1];
        int rowid = row - (int)'0'; int colid = col - (int)'a';

        bool identifierisrow = false;
        int identifierid = (pieceidentifier - (int)'0');
        if (identifierid >=0 && identifierid<8) identifierisrow = true;
        else identifierid = (pieceidentifier - (int)'a');

        uint64_t row_bits = 255;
        uint64_t col_bits = 1 + (one << 8) + (one << 16) + (one << 24) + (one << 32) + (one << 40) + (one << 48) + (one << 56);

        uint64_t reachable;
        // find piece starting position
        int initialposition;
        uint64_t possiblepieces = pieces[self][piece_id[piece]];
        if (identifierisrow) {
            possiblepieces |= row_bits << (8*identifierid);
        } else {
            possiblepieces |= col_bits << identifierid;
        }

        for (int i=0; i<64; i++) {
            for (int i=0; i<64; i++) {
                if (possiblepieces & 1) {
                    switch (piece)
                    {
                    case 'K':
                        reachable = kingAttacking(i, color);
                        break;
                    case 'Q':
                        reachable = queenAttacking(i, color);
                        break;
                    case 'R':
                        reachable = rookAttacking(i, color);
                        break;
                    case 'B':
                        reachable = bishopAttacking(i, color);
                        break;
                    case 'N':
                        reachable = knightAttacking(i, color);
                        break;
                    default:
                        break;
                    }
                    if ((reachable & positionBits[8*row+col]) != 0) {
                        initialposition = i; break;
                    }
                }
            }
        }

        if (color) result = movePiece(piece_id[piece], initialposition, 8*rowid+colid, color, 1);
        else result = movePiece(piece_id[piece], initialposition, 8*rowid+colid, color, 0);

        return result;
    } else if (move.length() == 5) {
        if (!move.compare("O-O-O")) {
            if (!canooocastle[self]) return false;

            result = true;
            uint64_t selftemp[6]; copy(pieces[self], selftemp, 6); // copy previous state
            uint64_t othertemp[6]; copy(pieces[other], othertemp, 6); // copy previous state

            if (color) {
                result &= movePiece(0, 4, 3, color, 1);
                result &= movePiece(0, 3, 2, color, 1);
                result &= movePiece(2, 0, 3, color, 1);
            } else {
                result &= movePiece(0, 7*8+4, 7*8+3, color, 0);
                result &= movePiece(0, 7*8+3, 7*8+2, color, 0);
                result &= movePiece(2, 7*8+0, 7*8+3, color, 0);
            }
            if (!result) {
                copy(selftemp, pieces[self], 6); copy(othertemp, pieces[other], 6);
            }
            return result;
        }

        char row = move[4]; char col = move[3];
        char piece = move[0]; char piecerow = move[1]; char piececol = move[2];

        int rowid = row - (int)'0'; int colid = col - (int)'a';
        int irowid = piecerow - (int)'0'; int icolid = piececol - (int)'a';

        if (color) result = movePiece(piece_id[piece], 8*irowid+icolid, 8*rowid+colid, color, 1);
        else result = movePiece(piece_id[piece], 8*irowid+icolid, 8*rowid+colid, color, 0);

        return result;
    } else {
        return false;
    }

}

void startGame() {
    startingPosition();
    std::string move;
    bool color = true;
    display();

    while (true) {
        if (color) std::cout << "White moves: ";
        else std::cout << "Black moves: ";
        
        std::cin >> move;
        if (movePieceFromString(move, color)) {
            color = !color;
        } else {
            std::cout << "Not valid" << std::endl;
        }
        display();
    }   
}

};

int main() {
    Chessboard chessboard;
    chessboard.display();

    if (chessboard.movePiece(5, 1*8+0, 2*8+0, 1, 0)) std::cout << "Moved king" << std::endl;
    chessboard.display();
    std::cout << std::endl;
    if (chessboard.checkKingInCheck(0)) std::cout << "King in check" << std::endl;
    else std::cout << "King not in check" << std::endl;

    chessboard.startGame();
}