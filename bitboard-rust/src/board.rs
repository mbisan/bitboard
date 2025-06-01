pub mod board {

use crate::lookup::lookup::{B_LROOK_START, W_LROOK_START, W_PAWN_PUSH, B_PAWN_PUSH};
use core::arch::x86_64::_tzcnt_u64;

unsafe fn tzcnt(x: u64) -> u64 {
    unsafe { _tzcnt_u64(x) }
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
        return State {state: (self.state & 0b000000000001100) + 1 };
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
}

#[derive(Copy, Clone)]
pub struct moveInfo {
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
    pub movedPiece: u8,
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
}

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
            print!(" - {}", unsafe { inttoletter.chars().nth((tzcnt((self.st.state >> 8) as u64) % 8) as usize).unwrap() });
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
            self.b.removePiece(piece_move);
        } else {
            self.b.moveking(piece_move);
            self.w.removePiece(piece_move);
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
            self.b.removePiece(piece_move);
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
            self.w.removePiece(piece_move);
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
                self.b.removePiece(piece_move);
            } else {
                self.b.moveRook(piece_move);
                self.w.removePiece(piece_move);
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
                self.b.removePiece(piece_move);
            } else {
                if (piece_move&B_LROOK_START)>0 {
                    self.b.moveRook(piece_move);
                    self.st = self.st.lRookMove();
                } else {
                    self.b.moveRook(piece_move);
                    self.st = self.st.rRookMove();
                }
                self.w.removePiece(piece_move);
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
            let removed_piece: u64 = ((self.st.state as u64) >> 8) >> 32;
            self.b.removePiece(removed_piece);
        } else {
            self.b.movePawn(piece_move);
            let removed_piece: u64 = ((self.st.state as u64) >> 8) >> 24;
            self.w.removePiece(removed_piece);
        }
        self.st = self.st.otherMove();
    }

    fn restore_piece(&mut self, piece_position: u64, piece: char) {
        if self.st.white() {
            if piece=='q' {
                self.w.moveQueen(piece_position);
            } else if piece=='r' {
                self.w.moveRook(piece_position);
            } else if piece=='b' {
                self.w.moveBishop(piece_position);
            } else if piece=='n' {
                self.w.moveKnight(piece_position);
            } else if piece=='p' {
                self.w.moveKnight(piece_position);
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
                self.b.moveKnight(piece_position);
            }
        }
    }

    fn undoPawnEP(&mut self, piece_move: u64) {
        if self.st.white() {
            self.w.movePawn(piece_move);
            let removed_piece: u64 = ((self.st.state as u64) >> 8) >> 32;
            self.restore_piece(removed_piece, 'p');
        } else {
            self.b.movePawn(piece_move);
            let removed_piece: u64 = ((self.st.state as u64) >> 8) >> 24;
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
                self.b.removePiece(end_square);
            } else if piece=='r' {
                self.w.movePawn(pawn_square);
                self.w.moveRook(end_square);
                self.b.removePiece(end_square);
            } else if piece=='b' {
                self.w.movePawn(pawn_square);
                self.w.moveBishop(end_square);
                self.b.removePiece(end_square);
            } else if piece=='n' {
                self.w.movePawn(pawn_square);
                self.w.moveKnight(end_square);
                self.b.removePiece(end_square);
            }
        } else {
            if piece=='q' {
                self.b.movePawn(pawn_square);
                self.b.moveQueen(end_square);
                self.w.removePiece(end_square);
            } else if piece=='r' {
                self.b.movePawn(pawn_square);
                self.b.moveRook(end_square);
                self.w.removePiece(end_square);
            } else if piece=='b' {
                self.b.movePawn(pawn_square);
                self.b.moveBishop(end_square);
                self.w.removePiece(end_square);
            } else if piece=='n' {
                self.b.movePawn(pawn_square);
                self.b.moveKnight(end_square);
                self.w.removePiece(end_square);
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
        self.st.kingMove();
    }

    fn castleR(&mut self) {
        if self.st.white() {
            self.w.moveking(0x0000000000000050u64);
            self.w.moveRook(0x00000000000000a0u64);
        } else {
            self.b.moveking(0x5000000000000000u64);
            self.b.moveRook(0xa000000000000000u64);
        }
        self.st.kingMove();
    }

    pub fn applyMove(&mut self, applied_move: moveInfo) {

        let moveType = applied_move.moveType & 0b00000111;
        if moveType==0 {
            // normal move without capture
            let moved_piece = "qrbnpk".chars().nth(applied_move.movedPiece as usize).unwrap();
            let initial_final = positionToSquare(applied_move.from) | positionToSquare(applied_move.to);
            if moved_piece=='k' {
                self.kingMove(initial_final);
            } else if moved_piece=='r' {
                self.rookMove(initial_final);
            } else {
                self.pieceMove(positionToSquare(applied_move.from) | positionToSquare(applied_move.to), moved_piece);
            }
        } else if moveType==1 {
            // normal move with capture
            // let captured_piece = (applied_move.moveType & 0b00111000) >> 3;
            let moved_piece = "qrbnpk".chars().nth(applied_move.movedPiece as usize).unwrap();
            let initial_final = positionToSquare(applied_move.from) | positionToSquare(applied_move.to);
            if moved_piece=='k' {
                self.kingMoveCapture(initial_final);
            } else if moved_piece=='r' {
                self.rookMoveCapture(initial_final);
            } else {
                self.pieceMoveCapture(positionToSquare(applied_move.from) | positionToSquare(applied_move.to), moved_piece);
            }
        } else if moveType==2 {
            // pawn push
            println!("Pushing pawn");
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

    pub fn undoMove(&mut self, applied_move: moveInfo) {
        /* Here the state is already changed, the previous state is given by the applied_move.currstate */
        self.st.state = self.st.state ^ 1; // change state to previous player so that they change pieces

        let moveType = applied_move.moveType & 0b00000111;
        if moveType==0 {
            // normal move without capture, same as capturing piece
            let moved_piece = "qrbnpk".chars().nth(applied_move.movedPiece as usize).unwrap();
            let initial_final = positionToSquare(applied_move.from) | positionToSquare(applied_move.to);
            if moved_piece=='k' {
                self.kingMove(initial_final);
            } else if moved_piece=='r' {
                self.rookMove(initial_final);
            } else {
                self.pieceMove(positionToSquare(applied_move.from) | positionToSquare(applied_move.to), moved_piece);
            }
        } else if moveType==1 {
            // normal move with capture
            let captured_piece = "qrbnpk".chars().nth(((applied_move.moveType & 0b00111000) >> 3) as usize).unwrap();
            
            // restore captured piece
            self.restore_piece(positionToSquare(applied_move.to), captured_piece);

            let moved_piece = "qrbnpk".chars().nth(applied_move.movedPiece as usize).unwrap();
            let initial_final = positionToSquare(applied_move.from) | positionToSquare(applied_move.to);
            if moved_piece=='k' {
                self.kingMove(initial_final);
            } else if moved_piece=='r' {
                self.rookMove(initial_final);
            } else {
                self.pieceMove(positionToSquare(applied_move.from) | positionToSquare(applied_move.to), moved_piece);
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
            self.undoPawnEP(positionToSquare(applied_move.from) | positionToSquare(applied_move.to));
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
}


}