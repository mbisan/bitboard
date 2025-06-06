pub mod board {

use crate::lookup::lookup::{
    slide, BISHOP_MOVES, B_LROOK_START, B_L_CASTLE_SEEN, B_PAWN_LAST, B_PAWN_PUSH, B_PAWN_START, B_R_CASTLE_SEEN, CHECK_BETWEEN, KING_MOVES, KNIGHT_MOVES, NOT_A_FILE, NOT_H_FILE, PIN_BETWEEN, ROOK_MOVES, W_LROOK_START, W_L_CASTLE_SEEN, W_PAWN_LAST, W_PAWN_PUSH, W_PAWN_START, W_R_CASTLE_SEEN
};
use core::arch::x86_64::{_tzcnt_u64, _blsr_u64, _blsi_u64};

fn print_squares(squares: u64) {
    for row in (0..8).rev() {
        for col in 0..8 {
            print!("{}", if (squares>>(8*row+col) & 1) == 1 {"x"} else {"o"});
        }
        println!();
    }
    println!();
}

fn tzcnt(x: u64) -> usize {
    unsafe { _tzcnt_u64(x) as usize }
}

fn blsr(x: u64) -> u64 {
    unsafe { _blsr_u64(x) }
}

fn blsi(x: u64) -> u64 {
    unsafe { _blsi_u64(x) }
}

pub fn printmove(move_info: MoveInfo) {
    let from_row = move_info.from/8;
    let from_col = move_info.from%8;
    let to_row = move_info.to/8;
    let to_col = move_info.to%8;

    print!("{}", "abcdefgh".chars().nth(from_col as usize).unwrap());
    print!("{}", "12345678".chars().nth(from_row as usize).unwrap());
    print!("{}", "abcdefgh".chars().nth(to_col as usize).unwrap());
    print!("{}: ", "12345678".chars().nth(to_row as usize).unwrap());
}

const PIECES: [&str; 13] = [
    "♔", // 0
    "♕", "♖", "♗", "♘", "♙", // 1–5
    "♚", "♛", "♜", "♝", "♞", "♟", // 6–11
    " "  // 12
];

fn displaySquare(color: bool, piece: usize) {
    if color {
        print!("\x1b[43m{} \x1b[0m", PIECES[piece]); // Yellow background
    } else {
        print!("\x1b[46m{} \x1b[0m", PIECES[piece]); // Cyan background
    }
}

pub fn positionToSquare(position: u8) -> u64 {
    return 1u64 << position;
}

#[derive(Copy, Clone)]
pub struct State {
    pub state: u16,
    /*
        B7 B6 B5 B4 B3 B2 B1 B0
         |  |  |  |  |  |  |  | iswhite, 1 if white, 0 else
         |  |  |  |  |  |  ---- bL, 1 if Left castle available, 0 else
         |  |  |  |  |  ------- bR, 1 if Right castle available, 0 else
         |  |  |  |  ---------- wL
         |  |  |  ------------- wR
         |  |  ---------------- ep, en passant available
         |  | 
         | 

        B15B14B13B12B11B10B9 B8 -- En passant square
    */
}

impl State {
    pub fn new() -> State {return State {state: 0b0000000000011111};}

    pub fn white(&self) -> bool {
        // return white if while
        return (self.state & 1) == 1;
    }
    pub fn enPassant(&self) -> bool {
        // return true if last move was a pawn push
        return (self.state>>5 & 1) == 1;
    }
    pub fn castle(&self) -> bool {
        // return true if current player can castle (either L or R)
        if self.white() {
            return (self.state & 0b0000000000011000) > 0;
        }
        return (self.state & 0b0000000000000110) > 0;
    }
    pub fn castleWL(&self) -> bool {
        return (self.state & 0b0000000000001000)>0
    }
    pub fn castleWR(&self) -> bool {
        return (self.state & 0b0000000000010000)>0
    }
    pub fn castleBL(&self) -> bool {
        return (self.state & 0b0000000000000010)>0
    }
    pub fn castleBR(&self) -> bool {
        return (self.state & 0b0000000000000100)>0
    }
    pub fn kingMove(&self) -> State {
        if self.white() {
            return State {state: self.state & 0b0000000000000110};
        }
        return State {state: (self.state & 0b000000000011000) + 1 };
    }
    pub fn otherMove(&self) -> State {
        if self.white() {
            return State {state: self.state & 0b0000000000011110};
        }
        return State {state: (self.state & 0b0000000000011110) + 1 };
    }
    pub fn lRookMove(&self) -> State {
        if self.white() {
            return State {state: self.state & 0b0000000000010110};
        }
        return State {state: (self.state & 0b0000000000011100) + 1 };
    }
    pub fn rRookMove(&self) -> State {
        if self.white() {
            return State {state: self.state & 0b0000000000001110};
        }
        return State {state: (self.state & 0b0000000000011010) + 1 };
    }
    pub fn pawnPush(&self, piece_move: u64) -> State {
        if self.white() {
            let ep_position: u16 = ((piece_move & W_PAWN_PUSH) >> 16) as u16;
            return State {state: ((self.state | 0b00100000) & 0b0000000000111110) | ep_position };
        }
        let ep_position: u16 = ((piece_move & B_PAWN_PUSH) >> 24) as u16;
        return State {state: (((self.state | 0b00100000) & 0b0000000000111110) + 1) | ep_position };
    }
    pub fn epSquare(&self) -> u64 {
        let enPassantSquare: u64 = (self.state>>8) as u64;
        if self.white() {
            return enPassantSquare<<32;
        } else {
            return enPassantSquare<<24;
        }
    }
}

pub struct StatusReport {
    pub checkCount: u64,
    kingIndex: usize,
    checkMask: u64,
    kingBan: u64,
    pinHV: u64,
    pinD: u64,
    enemySeen: u64,
    selfOcc: u64,
    enemyOcc: u64,
    epPin: u64,
}

#[derive(Copy, Clone)]
pub struct MoveInfo {
    pub currState: State, // 16 bits
    /*
    CastleL, CastleR, w/b -> 4 movimientos
    en passant capture
    promotion -> 4 tipos de pieza

    three first bits:
    0 -> normal move, i.e. no capture, no castling, no EP, no promotion, no push
    1 -> normal capture
    2 -> pawn push
    3 -> castleL
    4 -> castleR (the player is encoded in currState)
    5 -> en passant capture -> en passant info is in currState (last 8 bits)
    6 -> pawn promotion
    7 -> pawn promote capture

    next 3 bits:
    type of piece captured
    0 -> queen
    1 -> rook
    2 -> bishop
    3 -> knight
    4 -> pawn

    next 2 bits:
    type of piece when promotion/promotecapture
    0 -> queen
    1 -> rook
    2 -> bishop
    3 -> knight
    */
    pub moveType: u8,
    pub movedPiece: char,
    /*
    type of piece moved
    0 -> queen
    1 -> rook
    2 -> bishop
    3 -> knight
    4 -> pawn
    5 -> king
    */
    pub from: u8, // 0-63 position
    pub to: u8 // 0-63 position
}

#[derive(Copy, Clone)]
pub struct Pieces {
    pub k: u64,
    pub q: u64,
    pub r: u64,
    pub b: u64,
    pub n: u64,
    pub p: u64
}

impl Pieces {
    pub fn white() -> Pieces {
        return Pieces {
            k: 0x0000000000000008,
            q: 0x0000000000000010,
            r: 0x0000000000000081,
            b: 0x0000000000000024,
            n: 0x0000000000000042,
            p: 0x000000000000ff00
        }
    }
    pub fn black() -> Pieces {
        return Pieces {
            k: 0x0800000000000000,
            q: 0x1000000000000000,
            r: 0x8100000000000000,
            b: 0x2400000000000000,
            n: 0x4200000000000000,
            p: 0x00ff000000000000
        }
    }

    pub fn occupied(&self) -> u64 {
        return self.k|self.q|self.r|self.b|self.n|self.p;
    }

    pub fn moveking(&mut self, piece_move: u64) {
        // piece_move contains initial and final positions as 1s
        self.k = self.k ^ piece_move;
    }

    pub fn moveQueen(&mut self, piece_move: u64) {
        // piece_move contains initial and final positions as 1s
        self.q = self.q ^ piece_move;
    }

    pub fn moveRook(&mut self, piece_move: u64) {
        // piece_move contains initial and final positions as 1s
        self.r = self.r ^ piece_move;
    }

    pub fn moveBishop(&mut self, piece_move: u64) {
        // piece_move contains initial and final positions as 1s
        self.b = self.b ^ piece_move;
    }

    pub fn moveKnight(&mut self, piece_move: u64) {
        // piece_move contains initial and final positions as 1s
        self.n = self.n ^ piece_move;
    }

    pub fn movePawn(&mut self, piece_move: u64) {
        // piece_move contains initial and final positions as 1s
        self.p = self.p ^ piece_move;
    }

    pub fn removePiece(&mut self, position: u64) {
        // position contains 1 except on the removed piece
        self.q = self.q & position;
        self.r = self.r & position;
        self.b = self.b & position;
        self.n = self.n & position;
        self.p = self.p & position;
    }

    pub fn pieceType(&self, position: u64) -> u8 {
        /*
            Bitmask for captured piece
            0 -> queen
            1 -> rook
            2 -> bishop
            3 -> knight
            4 -> pawn
        */
        if (self.p&position)!=0 { return 0b00100000; }
        if (self.r&position)!=0 { return 0b00001000; }
        if (self.b&position)!=0 { return 0b00010000; }
        if (self.q&position)!=0 { return 0b00000000; }
        if (self.n&position)!=0 { return 0b00011000; }
        return 0b00111000;
    }
}

#[derive(Copy, Clone)]
pub struct Board {
    pub w: Pieces, pub b: Pieces, pub st: State
}

impl Board {
    pub fn new() -> Board {
        return Board { w: Pieces::white(), b: Pieces::black(), st: State::new() }
    }

    pub fn displayBoard(&self) {
        let wocc = self.w.occupied();
        let bocc = self.b.occupied();

        for row in (0..8).rev() {
            for col in 0..8 {
                let a = positionToSquare(8*row + col);
                let background: bool = ((row+col)%2)==1;
                
                if (bocc & a)>0 { // found square occupied by white piece
                    if (self.b.k & a)>0 { displaySquare(background, 6);}
                    if (self.b.q & a)>0 { displaySquare(background, 7);}
                    if (self.b.r & a)>0 { displaySquare(background, 8);}
                    if (self.b.b & a)>0 { displaySquare(background, 9);}
                    if (self.b.n & a)>0 { displaySquare(background, 10);}
                    if (self.b.p & a)>0 { displaySquare(background, 11);}
                } else if (wocc & a)>0 { // found square occupied by white piece
                    if (self.w.k & a)>0 { displaySquare(background, 0); }
                    if (self.w.q & a)>0 { displaySquare(background, 1); }
                    if (self.w.r & a)>0 { displaySquare(background, 2); }
                    if (self.w.b & a)>0 { displaySquare(background, 3); }
                    if (self.w.n & a)>0 { displaySquare(background, 4); }
                    if (self.w.p & a)>0 { displaySquare(background, 5); }
                } else {
                    displaySquare(background, 12);
                }
            }
            println!("");
        }
        if self.st.white() {
            print!("w - ");
        } else {
            print!("b - ");
        }
        if self.st.castleWL() { print!("Q"); }
        if self.st.castleWR() { print!("K"); }
        if self.st.castleBL() { print!("q"); }
        if self.st.castleBR() { print!("k"); }

        if self.st.enPassant() {
            let inttoletter = "abcdefgh";
            print!(" - {}", inttoletter.chars().nth((tzcnt((self.st.state >> 8) as u64) % 8) as usize).unwrap());
        }
        println!("");
    }

    fn kingMove(&mut self, piece_move: u64) {
        if self.st.white() {
            self.w.moveking(piece_move);
        } else {
            self.b.moveking(piece_move);
        }
        self.st = self.st.kingMove();
    }

    fn kingMoveCapture(&mut self, piece_move: u64) {
        if self.st.white() {
            self.w.moveking(piece_move);
            self.b.removePiece(!piece_move);
        } else {
            self.b.moveking(piece_move);
            self.w.removePiece(!piece_move);
        }
        self.st = self.st.kingMove();
    }

    fn pieceMove(&mut self, piece_move: u64, piece: char) {
        // moves queen, bishop, knight
        if self.st.white() {
            if piece=='q' {
                self.w.moveQueen(piece_move);
            } else if piece=='b' {
                self.w.moveBishop(piece_move);
            } else if piece=='n' {
                self.w.moveKnight(piece_move);
            } else if piece=='p' {
                self.w.movePawn(piece_move);
            }
        } else {
            if piece=='q' {
                self.b.moveQueen(piece_move);
            } else if piece=='b' {
                self.b.moveBishop(piece_move);
            } else if piece=='n' {
                self.b.moveKnight(piece_move);
            } else if piece=='p' {
                self.b.movePawn(piece_move);
            }
        }
        self.st = self.st.otherMove();
    }

    fn pieceMoveCapture(&mut self, piece_move: u64, piece: char) {
        // moves queen, bishop, knight
        if self.st.white() {
            if piece=='q' {
                self.w.moveQueen(piece_move);
            } else if piece=='b' {
                self.w.moveBishop(piece_move);
            } else if piece=='n' {
                self.w.moveKnight(piece_move);
            } else if piece=='p' {
                self.w.movePawn(piece_move);
            }
            self.b.removePiece(!piece_move);
        } else {
            if piece=='q' {
                self.b.moveQueen(piece_move);
            } else if piece=='b' {
                self.b.moveBishop(piece_move);
            } else if piece=='n' {
                self.b.moveKnight(piece_move);
            } else if piece=='p' {
                self.b.movePawn(piece_move);
            }
            self.w.removePiece(!piece_move);
        }
        self.st = self.st.otherMove();
    }

    fn rookMove(&mut self, piece_move: u64) {
        if self.st.castle()==false {
            if self.st.white() {
                self.w.moveRook(piece_move);
            } else {
                self.b.moveRook(piece_move);
            }            
            self.st = self.st.otherMove();
        } else {
            if self.st.white() {
                if (piece_move&W_LROOK_START)>0 {
                    self.w.moveRook(piece_move);
                    self.st = self.st.lRookMove();
                } else {
                    self.w.moveRook(piece_move);
                    self.st = self.st.rRookMove();
                }
            } else {
                if (piece_move&B_LROOK_START)>0 {
                    self.b.moveRook(piece_move);
                    self.st = self.st.lRookMove();
                } else {
                    self.b.moveRook(piece_move);
                    self.st = self.st.rRookMove();
                }
            }    
        }
    }

    fn rookMoveCapture(&mut self, piece_move: u64) {
        if self.st.castle()==false {
            if self.st.white() {
                self.w.moveRook(piece_move);
                self.b.removePiece(!piece_move);
            } else {
                self.b.moveRook(piece_move);
                self.w.removePiece(!piece_move);
            }            
            self.st = self.st.otherMove();
        } else {
            if self.st.white() {
                if (piece_move&W_LROOK_START)>0 {
                    self.w.moveRook(piece_move);
                    self.st = self.st.lRookMove();
                } else {
                    self.w.moveRook(piece_move);
                    self.st = self.st.rRookMove();
                }
                self.b.removePiece(!piece_move);
            } else {
                if (piece_move&B_LROOK_START)>0 {
                    self.b.moveRook(piece_move);
                    self.st = self.st.lRookMove();
                } else {
                    self.b.moveRook(piece_move);
                    self.st = self.st.rRookMove();
                }
                self.w.removePiece(!piece_move);
            }    
        }
    }

    fn pawnPush(&mut self, piece_move: u64) {
        if self.st.white() {
            self.w.movePawn(piece_move);
        } else {
            self.b.movePawn(piece_move);
        }
        self.st = self.st.pawnPush(piece_move);
    }

    fn pawnEP(&mut self, piece_move: u64) {
        if self.st.white() {
            self.w.movePawn(piece_move);
            let removed_piece: u64 = ((self.st.state as u64) >> 8) << 32;
            self.b.removePiece(!removed_piece);
        } else {
            self.b.movePawn(piece_move);
            let removed_piece: u64 = ((self.st.state as u64) >> 8) << 24;
            self.w.removePiece(!removed_piece);
        }
        self.st = self.st.otherMove();
    }

    fn restore_piece(&mut self, piece_position: u64, piece: char) {
        if !self.st.white() {
            if piece=='q' {
                self.w.moveQueen(piece_position);
            } else if piece=='r' {
                self.w.moveRook(piece_position);
            } else if piece=='b' {
                self.w.moveBishop(piece_position);
            } else if piece=='n' {
                self.w.moveKnight(piece_position);
            } else if piece=='p' {
                self.w.movePawn(piece_position);
            }
        } else {
            if piece=='q' {
                self.b.moveQueen(piece_position);
            } else if piece=='r' {
                self.b.moveRook(piece_position);
            } else if piece=='b' {
                self.b.moveBishop(piece_position);
            } else if piece=='n' {
                self.b.moveKnight(piece_position);
            } else if piece=='p' {
                self.b.movePawn(piece_position);
            }
        }
    }

    fn undoPawnEP(&mut self, piece_move: u64, previous_state: State) {
        if self.st.white() {
            self.w.movePawn(piece_move);
            let removed_piece: u64 = ((previous_state.state as u64) >> 8) << 32;
            self.restore_piece(removed_piece, 'p');
        } else {
            self.b.movePawn(piece_move);
            let removed_piece: u64 = ((previous_state.state as u64) >> 8) << 24;
            self.restore_piece(removed_piece, 'p');
        }
    }

    fn pawnPromote(&mut self, pawn_square: u64, end_square: u64, piece: char) {
        if self.st.white() {
            if piece=='q' {
                self.w.movePawn(pawn_square);
                self.w.moveQueen(end_square);
            } else if piece=='r' {
                self.w.movePawn(pawn_square);
                self.w.moveRook(end_square);
            } else if piece=='b' {
                self.w.movePawn(pawn_square);
                self.w.moveBishop(end_square);
            } else if piece=='n' {
                self.w.movePawn(pawn_square);
                self.w.moveKnight(end_square);
            }
        } else {
            if piece=='q' {
                self.b.movePawn(pawn_square);
                self.b.moveQueen(end_square);
            } else if piece=='r' {
                self.b.movePawn(pawn_square);
                self.b.moveRook(end_square);
            } else if piece=='b' {
                self.b.movePawn(pawn_square);
                self.b.moveBishop(end_square);
            } else if piece=='n' {
                self.b.movePawn(pawn_square);
                self.b.moveKnight(end_square);
                
            }
        }
        self.st = self.st.otherMove();
    }

    fn pawnPromoteCapture(&mut self, pawn_square: u64, end_square: u64, piece: char) {
        if self.st.white() {
            if piece=='q' {
                self.w.movePawn(pawn_square);
                self.w.moveQueen(end_square);
                self.b.removePiece(!end_square);
            } else if piece=='r' {
                self.w.movePawn(pawn_square);
                self.w.moveRook(end_square);
                self.b.removePiece(!end_square);
            } else if piece=='b' {
                self.w.movePawn(pawn_square);
                self.w.moveBishop(end_square);
                self.b.removePiece(!end_square);
            } else if piece=='n' {
                self.w.movePawn(pawn_square);
                self.w.moveKnight(end_square);
                self.b.removePiece(!end_square);
            }
        } else {
            if piece=='q' {
                self.b.movePawn(pawn_square);
                self.b.moveQueen(end_square);
                self.w.removePiece(!end_square);
            } else if piece=='r' {
                self.b.movePawn(pawn_square);
                self.b.moveRook(end_square);
                self.w.removePiece(!end_square);
            } else if piece=='b' {
                self.b.movePawn(pawn_square);
                self.b.moveBishop(end_square);
                self.w.removePiece(!end_square);
            } else if piece=='n' {
                self.b.movePawn(pawn_square);
                self.b.moveKnight(end_square);
                self.w.removePiece(!end_square);
            }
        }
        self.st = self.st.otherMove();
    }

    fn castleL(&mut self) {
        if self.st.white() {
            self.w.moveking(0x0000000000000014u64);
            self.w.moveRook(0x0000000000000009u64);
        } else {
            self.b.moveking(0x1400000000000000u64);
            self.b.moveRook(0x0900000000000000u64);
        }
        self.st = self.st.kingMove();
    }

    fn castleR(&mut self) {
        if self.st.white() {
            self.w.moveking(0x0000000000000050u64);
            self.w.moveRook(0x00000000000000a0u64);
        } else {
            self.b.moveking(0x5000000000000000u64);
            self.b.moveRook(0xa000000000000000u64);
        }
        self.st = self.st.kingMove();
    }

    pub fn applyMove(&mut self, applied_move: MoveInfo) {

        let moveType = applied_move.moveType & 0b00000111;
        if moveType==0 {
            // normal move without capture
            let initial_final = positionToSquare(applied_move.from) | positionToSquare(applied_move.to);
            if applied_move.movedPiece=='k' {
                self.kingMove(initial_final);
            } else if applied_move.movedPiece=='r' {
                self.rookMove(initial_final);
            } else {
                self.pieceMove(positionToSquare(applied_move.from) | positionToSquare(applied_move.to), applied_move.movedPiece);
            }
        } else if moveType==1 {
            // normal move with capture
            // let captured_piece = (applied_move.moveType & 0b00111000) >> 3;
            let initial_final = positionToSquare(applied_move.from) | positionToSquare(applied_move.to);
            if applied_move.movedPiece=='k' {
                self.kingMoveCapture(initial_final);
            } else if applied_move.movedPiece=='r' {
                self.rookMoveCapture(initial_final);
            } else {
                self.pieceMoveCapture(positionToSquare(applied_move.from) | positionToSquare(applied_move.to), applied_move.movedPiece);
            }
        } else if moveType==2 {
            // pawn push
            self.pawnPush(positionToSquare(applied_move.from) | positionToSquare(applied_move.to));
        } else if moveType==3 {
            // castleL
            self.castleL();
        } else if moveType==4 {
            // castleR
            self.castleR();
        } else if moveType==5 {
            // en passant
            self.pawnEP(positionToSquare(applied_move.from) | positionToSquare(applied_move.to));
        } else if moveType==6 {
            // pawn promotion
            let promoted_piece = "qrbn".chars().nth((applied_move.moveType >> 6) as usize).unwrap();
            self.pawnPromote(positionToSquare(applied_move.from), positionToSquare(applied_move.to), promoted_piece);
        } else if moveType==7 {
            // pawn promote capture
            let promoted_piece = "qrbn".chars().nth((applied_move.moveType >> 6) as usize).unwrap();
            self.pawnPromoteCapture(positionToSquare(applied_move.from), positionToSquare(applied_move.to), promoted_piece);
        }

    }

    pub fn undoMove(&mut self, applied_move: MoveInfo) {
        /* Here the state is already changed, the previous state is given by the applied_move.currstate */
        self.st.state = self.st.state ^ 1; // change state to previous player so that they change pieces

        let moveType = applied_move.moveType & 0b00000111;
        if moveType==0 {
            // normal move without capture, same as capturing piece
            let initial_final = positionToSquare(applied_move.from) | positionToSquare(applied_move.to);
            if applied_move.movedPiece=='k' {
                self.kingMove(initial_final);
            } else if applied_move.movedPiece=='r' {
                self.rookMove(initial_final);
            } else {
                self.pieceMove(positionToSquare(applied_move.from) | positionToSquare(applied_move.to), applied_move.movedPiece);
            }
        } else if moveType==1 {
            // normal move with capture
            let captured_piece = "qrbnpk".chars().nth(((applied_move.moveType & 0b00111000) >> 3) as usize).unwrap();
            
            // restore captured piece
            self.restore_piece(positionToSquare(applied_move.to), captured_piece);

            let initial_final = positionToSquare(applied_move.from) | positionToSquare(applied_move.to);
            if applied_move.movedPiece=='k' {
                self.kingMove(initial_final);
            } else if applied_move.movedPiece=='r' {
                self.rookMove(initial_final);
            } else {
                self.pieceMove(positionToSquare(applied_move.from) | positionToSquare(applied_move.to), applied_move.movedPiece);
            }
        } else if moveType==2 {
            // pawn push
            self.pawnPush(positionToSquare(applied_move.from) | positionToSquare(applied_move.to));
        } else if moveType==3 {
            // castleL
            self.castleL();
        } else if moveType==4 {
            // castleR
            self.castleR();
        } else if moveType==5 {
            // en passant
            self.undoPawnEP(positionToSquare(applied_move.from) | positionToSquare(applied_move.to), applied_move.currState);
        } else if moveType==6 {
            // pawn promotion
            let promoted_piece = "qrbn".chars().nth((applied_move.moveType >> 6) as usize).unwrap();
            self.pawnPromote(positionToSquare(applied_move.from), positionToSquare(applied_move.to), promoted_piece);
        } else if moveType==7 {
            // pawn promote capture

            let captured_piece = "qrbnpk".chars().nth(((applied_move.moveType & 0b00111000) >> 3) as usize).unwrap();
            // restore captured piece
            self.restore_piece(positionToSquare(applied_move.to), captured_piece);
            
            let promoted_piece = "qrbn".chars().nth((applied_move.moveType >> 6) as usize).unwrap();
            self.pawnPromote(positionToSquare(applied_move.from), positionToSquare(applied_move.to), promoted_piece);
        }

        self.st = applied_move.currState;
    }

    pub fn check(&self) -> StatusReport {

        let curr: &Pieces;
        let enemy: &Pieces;
        if self.st.white() {
            curr = &self.w;
            enemy = &self.b;
        } else {
            curr = &self.b;
            enemy = &self.w;
        }

        let selfOcc: u64 = curr.occupied();
        let enemyOcc: u64 = enemy.occupied();
        let occ: u64 = selfOcc|enemyOcc;

        let occNotKing: u64 = occ & !curr.k;

        let kingIndex: usize = tzcnt(curr.k);
        let mut checkCount: u64 = 0;

        let mut checkMask: u64 = 0;
        let mut kingBan: u64 = 0;
        let mut pinHV: u64 = 0;
        let mut pinD: u64 = 0;
        let mut epPin: u64 = 0;

        let mut enemySeen: u64 = 0;

        let mut temp: u64;
        
        temp = enemy.r|enemy.q;
        while temp!=0 {
            let pieceIndex: usize = tzcnt(temp);
            let atk: u64 = ROOK_MOVES[pieceIndex];
            let rookSeen: u64 = slide(atk, occ, pieceIndex);
            enemySeen |= rookSeen;

            let pin: u64 = PIN_BETWEEN[kingIndex][pieceIndex];

            if (rookSeen&curr.k)!=0 {
                checkMask |= pin;
                kingBan |= CHECK_BETWEEN[kingIndex][pieceIndex];
                checkCount+=1;
            } else if (atk & curr.k)!=0 {
                let inbetween: u64 = (pin ^ blsi(temp)) & occNotKing;
                let onePieceRemoved = blsr(inbetween);
                if onePieceRemoved==0 {
                    pinHV |= pin;
                } else if blsr(onePieceRemoved)==0 {
                    if self.st.enPassant() && (pieceIndex/8 == kingIndex/8) {
                        epPin = inbetween & curr.p;
                    }
                }
            }
            temp=blsr(temp);
        }

        temp = enemy.b|enemy.q;
        while temp!=0 {
            let pieceIndex: usize = tzcnt(temp);
            let atk: u64 = BISHOP_MOVES[pieceIndex];
            let bishopSeen: u64 = slide(atk, occ, pieceIndex);
            enemySeen |= bishopSeen;

            let pin: u64 = PIN_BETWEEN[kingIndex][pieceIndex];

            if (bishopSeen&curr.k)!=0 {
                checkMask |= pin;
                kingBan |= CHECK_BETWEEN[kingIndex][pieceIndex];
                checkCount+=1;
            } else if (atk & curr.k)!=0 {
                let inbetween: u64 = (pin ^ blsi(temp)) & occNotKing;
                let onePieceRemoved: u64 = blsr(inbetween);
                if onePieceRemoved==0 {
                    pinD |= pin;
                }
            }
            temp=blsr(temp);
        }

        temp = enemy.n;
        while temp!=0 {
            let pieceIndex: usize = tzcnt(temp);
            let atk: u64 = KNIGHT_MOVES[pieceIndex];
            enemySeen |= atk;

            if (atk&curr.k)!=0 {
                checkCount+=1;
                checkMask |= blsi(temp);
            }

            temp=blsr(temp);
        }

        let atkL: u64;
        let atkR: u64;
        if self.st.white() {
            atkL = (enemy.p & NOT_A_FILE) >> 9;
            atkR = (enemy.p & NOT_H_FILE) >> 7;
            if (atkL & curr.k)!=0 {
                checkMask |= (curr.k<<9) & enemy.p;
                checkCount+=1;
            }
            if (atkR & curr.k)!=0 {
                checkMask |= (curr.k<<7) & enemy.p;
                checkCount+=1;
            }
        } else {
            atkL = (enemy.p & NOT_A_FILE) << 7;
            atkR = (enemy.p & NOT_H_FILE) << 9;
            if (atkL & curr.k)!=0 {
                checkMask |= (curr.k>>7) & enemy.p;
                checkCount+=1;
            }
            if (atkR & curr.k)!=0 {
                checkMask |= (curr.k>>9) & enemy.p;
                checkCount+=1;
            }
        }
        enemySeen |= atkL | atkR;
        enemySeen |= KING_MOVES[tzcnt(enemy.k)];

        return StatusReport {
            checkCount: checkCount,
            kingIndex: kingIndex,
            checkMask: checkMask,
            kingBan: kingBan,
            pinHV: pinHV,
            pinD: pinD,
            enemySeen: enemySeen,
            selfOcc: selfOcc,
            enemyOcc: enemyOcc,
            epPin: epPin
        }
    }

    pub fn generateMovesSafe(&self) -> Vec<MoveInfo> {
        return unsafe {
            self.generateMoves()
        }
    }

    pub unsafe fn generateMoves(&self) -> Vec<MoveInfo> {
        let mut out: Vec<MoveInfo> = Vec::new();

        let curr: &Pieces;
        let enemy: &Pieces;
        let isWhite: bool = self.st.white();
        if isWhite {
            curr = &self.w;
            enemy = &self.b;
        } else {
            curr = &self.b;
            enemy = &self.w;
        }

        let res: StatusReport = self.check();
        let notEnemy = !res.enemyOcc;
        let notSelf = !res.selfOcc;
        let occ = res.selfOcc | res.enemyOcc;
        let notpin = !(res.pinHV | res.pinD);

        // king moves
        {    
            let kingReachable = KING_MOVES[res.kingIndex] & !res.kingBan & !res.enemySeen & notSelf;
            let mut temp: u64;

            temp = kingReachable & notEnemy;
            while temp!=0 {
                out.push(MoveInfo { currState: self.st, moveType: 0, movedPiece: 'k', from: res.kingIndex as u8, to: tzcnt(temp) as u8 });
                temp = blsr(temp);
            }
            temp = kingReachable & res.enemyOcc;
            while temp!=0 {
                out.push(MoveInfo { currState: self.st, moveType: 1 | enemy.pieceType(blsi(temp)), movedPiece: 'k', from: res.kingIndex as u8, to: tzcnt(temp) as u8 });
                temp = blsr(temp);
            }

            if res.checkCount > 1 { // only king can move
                return out;
            }
        }

        let notselfCheckmask: u64 = if res.checkCount == 0 { notSelf & !res.checkMask } else { notSelf & res.checkMask };

        // rook moves
        {
            // pinned
            let mut pinnedRooks: u64 = curr.r & res.pinHV;
            while pinnedRooks!=0 {
                let pieceIndex: usize = tzcnt(pinnedRooks);
                let atk: u64 = slide(ROOK_MOVES[pieceIndex], occ, pieceIndex) & notselfCheckmask & res.pinHV;

                let mut temp: u64 = atk & notEnemy;
                while temp!=0 {
                    out.push(MoveInfo { currState: self.st, moveType: 0, movedPiece: 'r', from: pieceIndex as u8, to: tzcnt(temp) as u8 });
                    temp = blsr(temp)
                }
                temp = atk & res.enemyOcc;
                while temp!=0 {
                    out.push(MoveInfo { currState: self.st, moveType: 1 | enemy.pieceType(blsi(temp)), movedPiece: 'r', from: pieceIndex as u8, to: tzcnt(temp) as u8 });
                    temp = blsr(temp)
                }
                pinnedRooks = blsr(pinnedRooks);
            }
            // not pinned
            let mut unpinnedRooks: u64 = curr.r & notpin;
            while unpinnedRooks!=0 {
                let pieceIndex: usize = tzcnt(unpinnedRooks);
                let atk: u64 = slide(ROOK_MOVES[pieceIndex], occ, pieceIndex) & notselfCheckmask;

                let mut temp: u64 = atk & notEnemy;
                while temp!=0 {
                    out.push(MoveInfo { currState: self.st, moveType: 0, movedPiece: 'r', from: pieceIndex as u8, to: tzcnt(temp) as u8 });
                    temp = blsr(temp)
                }
                temp = atk & res.enemyOcc;
                while temp!=0 {
                    out.push(MoveInfo { currState: self.st, moveType: 1 | enemy.pieceType(blsi(temp)), movedPiece: 'r', from: pieceIndex as u8, to: tzcnt(temp) as u8 });
                    temp = blsr(temp)
                }
                unpinnedRooks = blsr(unpinnedRooks);
            }
        }

        // bishop moves
        {
            // pinned
            let mut pinnedBishops: u64 = curr.b & res.pinD;
            while pinnedBishops!=0 {
                let pieceIndex: usize = tzcnt(pinnedBishops);
                let atk: u64 = slide(BISHOP_MOVES[pieceIndex], occ, pieceIndex) & notselfCheckmask & res.pinD;

                let mut temp: u64 = atk & notEnemy;
                while temp!=0 {
                    out.push(MoveInfo { currState: self.st, moveType: 0, movedPiece: 'b', from: pieceIndex as u8, to: tzcnt(temp) as u8 });
                    temp = blsr(temp)
                }
                temp = atk & res.enemyOcc;
                while temp!=0 {
                    out.push(MoveInfo { currState: self.st, moveType: 1 | enemy.pieceType(blsi(temp)), movedPiece: 'b', from: pieceIndex as u8, to: tzcnt(temp) as u8 });
                    temp = blsr(temp)
                }
                pinnedBishops = blsr(pinnedBishops);
            }
            // not pinned
            let mut unpinnedBishops: u64 = curr.b & notpin;
            while unpinnedBishops!=0 {
                let pieceIndex: usize = tzcnt(unpinnedBishops);
                let atk: u64 = slide(BISHOP_MOVES[pieceIndex], occ, pieceIndex) & notselfCheckmask;

                let mut temp: u64 = atk & notEnemy;
                while temp!=0 {
                    out.push(MoveInfo { currState: self.st, moveType: 0, movedPiece: 'b', from: pieceIndex as u8, to: tzcnt(temp) as u8 });
                    temp = blsr(temp)
                }
                temp = atk & res.enemyOcc;
                while temp!=0 {
                    out.push(MoveInfo { currState: self.st, moveType: 1 | enemy.pieceType(blsi(temp)), movedPiece: 'b', from: pieceIndex as u8, to: tzcnt(temp) as u8 });
                    temp = blsr(temp)
                }
                unpinnedBishops = blsr(unpinnedBishops);
            }
        }

        // queen moves
        {
            // pinned HV
            let mut queensHV: u64 = curr.q & res.pinHV;
            while queensHV!=0 {
                let pieceIndex: usize = tzcnt(queensHV);
                let atk: u64 = slide(ROOK_MOVES[pieceIndex], occ, pieceIndex) & notselfCheckmask & res.pinHV;

                let mut temp: u64 = atk & notEnemy;
                while temp!=0 {
                    out.push(MoveInfo { currState: self.st, moveType: 0, movedPiece: 'q', from: pieceIndex as u8, to: tzcnt(temp) as u8 });
                    temp = blsr(temp)
                }
                temp = atk & res.enemyOcc;
                while temp!=0 {
                    out.push(MoveInfo { currState: self.st, moveType: 1 | enemy.pieceType(blsi(temp)), movedPiece: 'q', from: pieceIndex as u8, to: tzcnt(temp) as u8 });
                    temp = blsr(temp)
                }
                queensHV = blsr(queensHV);
            }
            // pinned D
            let mut queensD: u64 = curr.q & res.pinD;
            while queensD!=0 {
                let pieceIndex: usize = tzcnt(queensD);
                let atk: u64 = slide(BISHOP_MOVES[pieceIndex], occ, pieceIndex) & notselfCheckmask & res.pinD;

                let mut temp: u64 = atk & notEnemy;
                while temp!=0 {
                    out.push(MoveInfo { currState: self.st, moveType: 0, movedPiece: 'q', from: pieceIndex as u8, to: tzcnt(temp) as u8 });
                    temp = blsr(temp)
                }
                temp = atk & res.enemyOcc;
                while temp!=0 {
                    out.push(MoveInfo { currState: self.st, moveType: 1 | enemy.pieceType(blsi(temp)), movedPiece: 'q', from: pieceIndex as u8, to: tzcnt(temp) as u8 });
                    temp = blsr(temp)
                }
                queensD = blsr(queensD);
            }
            // not pinned
            let mut unpinnedQueens: u64 = curr.q & notpin;
            while unpinnedQueens!=0 {
                let pieceIndex: usize = tzcnt(unpinnedQueens);
                let atk: u64 = (slide(ROOK_MOVES[pieceIndex], occ, pieceIndex) | slide(BISHOP_MOVES[pieceIndex], occ, pieceIndex)) & notselfCheckmask;

                let mut temp: u64 = atk & notEnemy;
                while temp!=0 {
                    out.push(MoveInfo { currState: self.st, moveType: 0, movedPiece: 'q', from: pieceIndex as u8, to: tzcnt(temp) as u8 });
                    temp = blsr(temp)
                }
                temp = atk & res.enemyOcc;
                while temp!=0 {
                    out.push(MoveInfo { currState: self.st, moveType: 1 | enemy.pieceType(blsi(temp)), movedPiece: 'q', from: pieceIndex as u8, to: tzcnt(temp) as u8 });
                    temp = blsr(temp)
                }
                unpinnedQueens = blsr(unpinnedQueens);
            }
        }

        // knight moves
        {
            // not pinned
            let mut unpinnedKnight: u64 = curr.n & notpin;
            while unpinnedKnight!=0 {
                let pieceIndex: usize = tzcnt(unpinnedKnight);
                let atk: u64 = KNIGHT_MOVES[pieceIndex] & notselfCheckmask;

                let mut temp: u64 = atk & notEnemy;
                while temp!=0 {
                    out.push(MoveInfo { currState: self.st, moveType: 0, movedPiece: 'n', from: pieceIndex as u8, to: tzcnt(temp) as u8 });
                    temp = blsr(temp)
                }
                temp = atk & res.enemyOcc;
                while temp!=0 {
                    out.push(MoveInfo { currState: self.st, moveType: 1 | enemy.pieceType(blsi(temp)), movedPiece: 'n', from: pieceIndex as u8, to: tzcnt(temp) as u8 });
                    temp = blsr(temp)
                }
                unpinnedKnight = blsr(unpinnedKnight);
            }
        }

        // pawns
        {
            let pawnAdvance: u64;
            let pawnPush: u64;
            let pawnCaptureL: u64;
            let pawnCaptureR: u64;
            let pawnAdvancePromote: u64;
            let pawnCaptureLpromote: u64;
            let pawnCaptureRpromote: u64;

            if isWhite {
                pawnAdvance = curr.p & !W_PAWN_LAST & !(occ >> 8);
                pawnPush = pawnAdvance & W_PAWN_START & !(occ >> 16);
                pawnCaptureL = curr.p & !W_PAWN_LAST & ((res.enemyOcc & NOT_H_FILE) >> 7);
                pawnCaptureR = curr.p & !W_PAWN_LAST & ((res.enemyOcc & NOT_A_FILE) >> 9);
                pawnAdvancePromote = curr.p & W_PAWN_LAST & !(occ >> 8);
                pawnCaptureLpromote = curr.p & W_PAWN_LAST & ((res.enemyOcc & NOT_H_FILE) >> 7);
                pawnCaptureRpromote = curr.p & W_PAWN_LAST & ((res.enemyOcc & NOT_A_FILE) >> 9);
            } else {
                pawnAdvance = curr.p & !B_PAWN_LAST & !(occ << 8);
                pawnPush = pawnAdvance & B_PAWN_START & !(occ << 16);
                pawnCaptureL = curr.p & !B_PAWN_LAST & ((res.enemyOcc & NOT_A_FILE) << 7);
                pawnCaptureR = curr.p & !B_PAWN_LAST & ((res.enemyOcc & NOT_H_FILE) << 9);
                pawnAdvancePromote = curr.p & B_PAWN_LAST & !(occ << 8);
                pawnCaptureLpromote = curr.p & B_PAWN_LAST & ((res.enemyOcc & NOT_A_FILE) << 7);
                pawnCaptureRpromote = curr.p & B_PAWN_LAST & ((res.enemyOcc & NOT_H_FILE) << 9);
            }

            let mut temp;

            if isWhite {
                // unpinned
                temp = pawnAdvance & notpin;
                while temp!=0 {
                    let finalposition: u64 = blsi(temp)<<8;
                    if (finalposition & notselfCheckmask)!=0 {
                        out.push(MoveInfo { currState: self.st, moveType: 0, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 });
                    }
                    temp = blsr(temp);
                }
                temp = pawnPush & notpin;
                while temp!=0 {
                    let finalposition: u64 = blsi(temp)<<16;
                    if (finalposition & notselfCheckmask)!=0 {
                        out.push(MoveInfo { currState: self.st, moveType: 2, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 });
                    }
                    temp = blsr(temp);
                }
                temp = pawnCaptureR & notpin;
                while temp!=0 {
                    let finalposition: u64 = blsi(temp)<<9;
                    if (finalposition & notselfCheckmask)!=0 {
                        out.push(MoveInfo { currState: self.st, moveType: 1 | enemy.pieceType(finalposition), movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 });
                    }
                    temp = blsr(temp);
                }
                temp = pawnCaptureL & notpin;
                while temp!=0 {
                    let finalposition: u64 = blsi(temp)<<7;
                    if (finalposition & notselfCheckmask)!=0 {
                        out.push(MoveInfo { currState: self.st, moveType: 1 | enemy.pieceType(finalposition), movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 });
                    }
                    temp = blsr(temp);
                }
                // pinned
                temp = pawnAdvance & res.pinHV;
                while temp!=0 {
                    let finalposition: u64 = blsi(temp)<<8;
                    if (finalposition & res.pinHV & notselfCheckmask)!=0 {
                        out.push(MoveInfo { currState: self.st, moveType: 0, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 });
                    }
                    temp = blsr(temp);
                }
                temp = pawnPush & res.pinHV;
                while temp!=0 {
                    let finalposition: u64 = blsi(temp)<<16;
                    if (finalposition & res.pinHV & notselfCheckmask)!=0 {
                        out.push(MoveInfo { currState: self.st, moveType: 2, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 });
                    }
                    temp = blsr(temp);
                }
                temp = pawnCaptureR & res.pinD;
                while temp!=0 {
                    let finalposition: u64 = blsi(temp)<<9;
                    if (finalposition & res.pinD & notselfCheckmask)!=0 {
                        out.push(MoveInfo { currState: self.st, moveType: 1 | enemy.pieceType(finalposition), movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 });
                    }
                    temp = blsr(temp);
                }
                temp = pawnCaptureL & res.pinD;
                while temp!=0 {
                    let finalposition: u64 = blsi(temp)<<7;
                    if (finalposition & res.pinD & notselfCheckmask)!=0 {
                        out.push(MoveInfo { currState: self.st, moveType: 1 | enemy.pieceType(finalposition), movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 });
                    }
                    temp = blsr(temp);
                }

                // not pinned promotion
                temp = pawnAdvancePromote & notpin;
                while temp!=0 {
                    let finalposition: u64 = blsi(temp)<<8;
                    if (finalposition & notselfCheckmask)!=0 {
                        out.push(MoveInfo { currState: self.st, moveType: 6 | 0b00000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // queen
                        out.push(MoveInfo { currState: self.st, moveType: 6 | 0b01000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // rook
                        out.push(MoveInfo { currState: self.st, moveType: 6 | 0b10000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // bishop
                        out.push(MoveInfo { currState: self.st, moveType: 6 | 0b11000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // knight
                    }
                    temp = blsr(temp);
                }
                temp = pawnCaptureRpromote & notpin;
                while temp!=0 {
                    let finalposition: u64 = blsi(temp)<<9;
                    if (finalposition & notselfCheckmask)!=0 {
                        out.push(MoveInfo { currState: self.st, moveType: 7 | enemy.pieceType(finalposition) | 0b00000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // queen
                        out.push(MoveInfo { currState: self.st, moveType: 7 | enemy.pieceType(finalposition) | 0b01000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // rook
                        out.push(MoveInfo { currState: self.st, moveType: 7 | enemy.pieceType(finalposition) | 0b10000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // bishop
                        out.push(MoveInfo { currState: self.st, moveType: 7 | enemy.pieceType(finalposition) | 0b11000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // knight
                    }
                    temp = blsr(temp);
                }
                temp = pawnCaptureLpromote & notpin;
                while temp!=0 {
                    let finalposition: u64 = blsi(temp)<<7;
                    if (finalposition & notselfCheckmask)!=0 {
                        out.push(MoveInfo { currState: self.st, moveType: 7 | enemy.pieceType(finalposition) | 0b00000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // queen
                        out.push(MoveInfo { currState: self.st, moveType: 7 | enemy.pieceType(finalposition) | 0b01000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // rook
                        out.push(MoveInfo { currState: self.st, moveType: 7 | enemy.pieceType(finalposition) | 0b10000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // bishop
                        out.push(MoveInfo { currState: self.st, moveType: 7 | enemy.pieceType(finalposition) | 0b11000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // knight
                    }
                    temp = blsr(temp);
                }

                // pinned promotion
                temp = pawnAdvancePromote & res.pinHV;
                while temp!=0 {
                    let finalposition: u64 = blsi(temp)<<8;
                    if (finalposition & res.pinHV & notselfCheckmask)!=0 {
                        out.push(MoveInfo { currState: self.st, moveType: 6 | 0b00000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // queen
                        out.push(MoveInfo { currState: self.st, moveType: 6 | 0b01000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // rook
                        out.push(MoveInfo { currState: self.st, moveType: 6 | 0b10000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // bishop
                        out.push(MoveInfo { currState: self.st, moveType: 6 | 0b11000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // knight
                    }
                    temp = blsr(temp);
                }
                temp = pawnCaptureRpromote & res.pinD;
                while temp!=0 {
                    let finalposition: u64 = blsi(temp)<<9;
                    if (finalposition & res.pinD & notselfCheckmask)!=0 {
                        out.push(MoveInfo { currState: self.st, moveType: 7 | enemy.pieceType(finalposition) | 0b00000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // queen
                        out.push(MoveInfo { currState: self.st, moveType: 7 | enemy.pieceType(finalposition) | 0b01000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // rook
                        out.push(MoveInfo { currState: self.st, moveType: 7 | enemy.pieceType(finalposition) | 0b10000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // bishop
                        out.push(MoveInfo { currState: self.st, moveType: 7 | enemy.pieceType(finalposition) | 0b11000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // knight
                    }
                    temp = blsr(temp);
                }
                temp = pawnCaptureLpromote & res.pinD;
                while temp!=0 {
                    let finalposition: u64 = blsi(temp)<<7;
                    if (finalposition & res.pinD & notselfCheckmask)!=0 {
                        out.push(MoveInfo { currState: self.st, moveType: 7 | enemy.pieceType(finalposition) | 0b00000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // queen
                        out.push(MoveInfo { currState: self.st, moveType: 7 | enemy.pieceType(finalposition) | 0b01000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // rook
                        out.push(MoveInfo { currState: self.st, moveType: 7 | enemy.pieceType(finalposition) | 0b10000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // bishop
                        out.push(MoveInfo { currState: self.st, moveType: 7 | enemy.pieceType(finalposition) | 0b11000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // knight
                    }
                    temp = blsr(temp);
                }

                // castles
                if self.st.castleWL() {
                    if (res.checkCount == 0) && ((res.enemySeen | occ) & W_L_CASTLE_SEEN)==0 {
                        out.push(MoveInfo { currState: self.st, moveType: 3, movedPiece: 'k', from: 0, to: 0 });
                    }
                }
                if self.st.castleWR() {
                    if (res.checkCount == 0) && ((res.enemySeen | occ) & W_R_CASTLE_SEEN)==0 {
                        out.push(MoveInfo { currState: self.st, moveType: 4, movedPiece: 'k', from: 0, to: 0 });
                    }
                }

            } else { // pawns for black
                // unpinned
                temp = pawnAdvance & notpin;
                while temp!=0 {
                    let finalposition: u64 = blsi(temp)>>8;
                    if (finalposition & notselfCheckmask)!=0 {
                        out.push(MoveInfo { currState: self.st, moveType: 0, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 });
                    }
                    temp = blsr(temp);
                }
                temp = pawnPush & notpin;
                while temp!=0 {
                    let finalposition: u64 = blsi(temp)>>16;
                    if (finalposition & notselfCheckmask)!=0 {
                        out.push(MoveInfo { currState: self.st, moveType: 2, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 });
                    }
                    temp = blsr(temp);
                }
                temp = pawnCaptureR & notpin;
                while temp!=0 {
                    let finalposition: u64 = blsi(temp)>>9;
                    if (finalposition & notselfCheckmask)!=0 {
                        out.push(MoveInfo { currState: self.st, moveType: 1 | enemy.pieceType(finalposition), movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 });
                    }
                    temp = blsr(temp);
                }
                temp = pawnCaptureL & notpin;
                while temp!=0 {
                    let finalposition: u64 = blsi(temp)>>7;
                    if (finalposition & notselfCheckmask)!=0 {
                        out.push(MoveInfo { currState: self.st, moveType: 1 | enemy.pieceType(finalposition), movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 });
                    }
                    temp = blsr(temp);
                }
                // pinned
                temp = pawnAdvance & res.pinHV;
                while temp!=0 {
                    let finalposition: u64 = blsi(temp)>>8;
                    if (finalposition & res.pinHV & notselfCheckmask)!=0 {
                        out.push(MoveInfo { currState: self.st, moveType: 0, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 });
                    }
                    temp = blsr(temp);
                }
                temp = pawnPush & res.pinHV;
                while temp!=0 {
                    let finalposition: u64 = blsi(temp)>>16;
                    if (finalposition & res.pinHV & notselfCheckmask)!=0 {
                        out.push(MoveInfo { currState: self.st, moveType: 2, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 });
                    }
                    temp = blsr(temp);
                }
                temp = pawnCaptureR & res.pinD;
                while temp!=0 {
                    let finalposition: u64 = blsi(temp)>>9;
                    if (finalposition & res.pinD & notselfCheckmask)!=0 {
                        out.push(MoveInfo { currState: self.st, moveType: 1 | enemy.pieceType(finalposition), movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 });
                    }
                    temp = blsr(temp);
                }
                temp = pawnCaptureL & res.pinD;
                while temp!=0 {
                    let finalposition: u64 = blsi(temp)>>7;
                    if (finalposition & res.pinD & notselfCheckmask)!=0 {
                        out.push(MoveInfo { currState: self.st, moveType: 1 | enemy.pieceType(finalposition), movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 });
                    }
                    temp = blsr(temp);
                }

                // not pinned promotion
                temp = pawnAdvancePromote & notpin;
                while temp!=0 {
                    let finalposition: u64 = blsi(temp)>>8;
                    if (finalposition & notselfCheckmask)!=0 {
                        out.push(MoveInfo { currState: self.st, moveType: 6 | 0b00000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // queen
                        out.push(MoveInfo { currState: self.st, moveType: 6 | 0b01000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // rook
                        out.push(MoveInfo { currState: self.st, moveType: 6 | 0b10000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // bishop
                        out.push(MoveInfo { currState: self.st, moveType: 6 | 0b11000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // knight
                    }
                    temp = blsr(temp);
                }
                temp = pawnCaptureRpromote & notpin;
                while temp!=0 {
                    let finalposition: u64 = blsi(temp)>>9;
                    if (finalposition & notselfCheckmask)!=0 {
                        out.push(MoveInfo { currState: self.st, moveType: 7 | enemy.pieceType(finalposition) | 0b00000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // queen
                        out.push(MoveInfo { currState: self.st, moveType: 7 | enemy.pieceType(finalposition) | 0b01000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // rook
                        out.push(MoveInfo { currState: self.st, moveType: 7 | enemy.pieceType(finalposition) | 0b10000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // bishop
                        out.push(MoveInfo { currState: self.st, moveType: 7 | enemy.pieceType(finalposition) | 0b11000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // knight
                    }
                    temp = blsr(temp);
                }
                temp = pawnCaptureLpromote & notpin;
                while temp!=0 {
                    let finalposition: u64 = blsi(temp)>>7;
                    if (finalposition & notselfCheckmask)!=0 {
                        out.push(MoveInfo { currState: self.st, moveType: 7 | enemy.pieceType(finalposition) | 0b00000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // queen
                        out.push(MoveInfo { currState: self.st, moveType: 7 | enemy.pieceType(finalposition) | 0b01000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // rook
                        out.push(MoveInfo { currState: self.st, moveType: 7 | enemy.pieceType(finalposition) | 0b10000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // bishop
                        out.push(MoveInfo { currState: self.st, moveType: 7 | enemy.pieceType(finalposition) | 0b11000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // knight
                    }
                    temp = blsr(temp);
                }

                // pinned promotion
                temp = pawnAdvancePromote & res.pinHV;
                while temp!=0 {
                    let finalposition: u64 = blsi(temp)>>8;
                    if (finalposition & res.pinHV & notselfCheckmask)!=0 {
                        out.push(MoveInfo { currState: self.st, moveType: 6 | 0b00000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // queen
                        out.push(MoveInfo { currState: self.st, moveType: 6 | 0b01000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // rook
                        out.push(MoveInfo { currState: self.st, moveType: 6 | 0b10000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // bishop
                        out.push(MoveInfo { currState: self.st, moveType: 6 | 0b11000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // knight
                    }
                    temp = blsr(temp);
                }
                temp = pawnCaptureRpromote & res.pinD;
                while temp!=0 {
                    let finalposition: u64 = blsi(temp)>>9;
                    if (finalposition & res.pinD & notselfCheckmask)!=0 {
                        out.push(MoveInfo { currState: self.st, moveType: 7 | enemy.pieceType(finalposition) | 0b00000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // queen
                        out.push(MoveInfo { currState: self.st, moveType: 7 | enemy.pieceType(finalposition) | 0b01000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // rook
                        out.push(MoveInfo { currState: self.st, moveType: 7 | enemy.pieceType(finalposition) | 0b10000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // bishop
                        out.push(MoveInfo { currState: self.st, moveType: 7 | enemy.pieceType(finalposition) | 0b11000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // knight
                    }
                    temp = blsr(temp);
                }
                temp = pawnCaptureLpromote & res.pinD;
                while temp!=0 {
                    let finalposition: u64 = blsi(temp)>>7;
                    if (finalposition & res.pinD & notselfCheckmask)!=0 {
                        out.push(MoveInfo { currState: self.st, moveType: 7 | enemy.pieceType(finalposition) | 0b00000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // queen
                        out.push(MoveInfo { currState: self.st, moveType: 7 | enemy.pieceType(finalposition) | 0b01000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // rook
                        out.push(MoveInfo { currState: self.st, moveType: 7 | enemy.pieceType(finalposition) | 0b10000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // bishop
                        out.push(MoveInfo { currState: self.st, moveType: 7 | enemy.pieceType(finalposition) | 0b11000000, movedPiece: 'p', from: tzcnt(temp) as u8, to: tzcnt(finalposition) as u8 }); // knight
                    }
                    temp = blsr(temp);
                }

                // castles
                if self.st.castleBL() {
                    if (res.checkCount == 0) && ((res.enemySeen | occ) & B_L_CASTLE_SEEN)==0 {
                        out.push(MoveInfo { currState: self.st, moveType: 3, movedPiece: 'k', from: 0, to: 0 });
                    }
                }
                if self.st.castleBR() {
                    if (res.checkCount == 0) && ((res.enemySeen | occ) & B_R_CASTLE_SEEN)==0 {
                        out.push(MoveInfo { currState: self.st, moveType: 4, movedPiece: 'k', from: 0, to: 0 });
                    }
                }
            }
        }

        if self.st.enPassant() {
            let epSquare: u64 = self.st.epSquare();

            if (epSquare & !(res.pinD))!=0 {
                let enemyPawnBehind = if isWhite { epSquare << 8 } else { epSquare >> 8 };

                if (epSquare & NOT_A_FILE)!=0 { // capture to the right
                    let pawnToTheLeft = (epSquare>>1) & curr.p & !res.epPin & !res.pinHV;

                    if (pawnToTheLeft & !res.pinD)!=0 && (enemyPawnBehind & notselfCheckmask)!=0 {
                        out.push(MoveInfo { currState: self.st, moveType: 5, movedPiece: 'p', from: tzcnt(pawnToTheLeft) as u8, to: tzcnt(enemyPawnBehind) as u8 });
                    } else if (pawnToTheLeft & !res.pinD)!=0 && (enemyPawnBehind & notselfCheckmask & res.pinD)!=0 {
                        out.push(MoveInfo { currState: self.st, moveType: 5, movedPiece: 'p', from: tzcnt(pawnToTheLeft) as u8, to: tzcnt(enemyPawnBehind) as u8 });
                    } else if (pawnToTheLeft & !res.pinD)!=0 && (epSquare & notselfCheckmask)!=0 {
                        out.push(MoveInfo { currState: self.st, moveType: 5, movedPiece: 'p', from: tzcnt(pawnToTheLeft) as u8, to: tzcnt(enemyPawnBehind) as u8 });
                    }
                }
                if (epSquare & NOT_H_FILE)!=0 { // capture to the right
                    let pawnToTheLeft = (epSquare<<1) & curr.p & !res.epPin & !res.pinHV;

                    if (pawnToTheLeft & !res.pinD)!=0 && (enemyPawnBehind & notselfCheckmask)!=0 {
                        out.push(MoveInfo { currState: self.st, moveType: 5, movedPiece: 'p', from: tzcnt(pawnToTheLeft) as u8, to: tzcnt(enemyPawnBehind) as u8 });
                    } else if (pawnToTheLeft & !res.pinD)!=0 && (enemyPawnBehind & notselfCheckmask & res.pinD)!=0 {
                        out.push(MoveInfo { currState: self.st, moveType: 5, movedPiece: 'p', from: tzcnt(pawnToTheLeft) as u8, to: tzcnt(enemyPawnBehind) as u8 });
                    } else if (pawnToTheLeft & !res.pinD)!=0 && (epSquare & notselfCheckmask)!=0 {
                        out.push(MoveInfo { currState: self.st, moveType: 5, movedPiece: 'p', from: tzcnt(pawnToTheLeft) as u8, to: tzcnt(enemyPawnBehind) as u8 });
                    }
                }
            }
        }

        return out;
    }

    pub fn from_fen(fen: &str) -> Self {
        let mut w = Pieces { k: 0, q: 0, r: 0, b: 0, n: 0, p: 0 };
        let mut b = Pieces { k: 0, q: 0, r: 0, b: 0, n: 0, p: 0 };
        let mut st = State { state: 0 };

        let parts: Vec<&str> = fen.split_whitespace().collect();
        assert!(parts.len() >= 4, "Invalid FEN string");

        let piece_placement = parts[0];
        let active_color = parts[1];
        let castling_rights = parts[2];
        let en_passant = parts[3];

        let mut row: i32 = 7;
        let mut col: i32 = 0;
        for c in piece_placement.chars() {
            match c {
                '/' => { row -= 1; col = 0},
                '1'..='8' => {
                    let empty = c.to_digit(10).unwrap() as i32;
                    col += empty;
                }
                _ => {
                    let bit = 1u64 << (row * 8 + col);
                    match c {
                        'P' => w.p |= bit,
                        'N' => w.n |= bit,
                        'B' => w.b |= bit,
                        'R' => w.r |= bit,
                        'Q' => w.q |= bit,
                        'K' => w.k |= bit,
                        'p' => b.p |= bit,
                        'n' => b.n |= bit,
                        'b' => b.b |= bit,
                        'r' => b.r |= bit,
                        'q' => b.q |= bit,
                        'k' => b.k |= bit,
                        _ => panic!("Invalid piece in FEN"),
                    }
                    col += 1;
                }
            }
        }

        // Side to move
        if active_color == "w" {
            st.state |= 1
        }

        // Castling rights
        if castling_rights.contains('K') { st.state |= 0b00010000 }
        if castling_rights.contains('Q') { st.state |= 0b00001000 }
        if castling_rights.contains('k') { st.state |= 0b00000100 }
        if castling_rights.contains('q') { st.state |= 0b00000010 }

        // En passant file
        if en_passant != "-" {
            let file_char = en_passant.chars().next().unwrap();
            let column_num = (file_char as u8 - b'a') as u8;
            st.state |= (1<<column_num)<<8;
            st.state |= 1<<5;
        }

        Board { w, b, st }
    }

}


}