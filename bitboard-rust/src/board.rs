pub mod board {

use crate::lookup::lookup::{B_LROOK_START, W_LROOK_START, W_PAWN_PUSH, B_PAWN_PUSH};

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

        B15B14B13B12B11B10B9 B8 -- En passant square on rows 3 and 5 of the board
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
            return (self.state & 0b0000000000011000) > 1;
        }
        return (self.state & 0b0000000000000110) > 1;
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
pub struct Pieces {
    pub k: u64,
    pub q: u64,
    pub r: u64,
    pub b: u64,
    pub n: u64,
    pub p: u64
}

impl Pieces {
    pub fn new() -> Pieces {
        return Pieces {
            k: 0,
            q: 0,
            r: 0,
            b: 0,
            n: 0,
            p: 0
        }
    }
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
        self.k = self.q ^ piece_move;
    }

    pub fn moveRook(&mut self, piece_move: u64) {
        // piece_move contains initial and final positions as 1s
        self.k = self.r ^ piece_move;
    }

    pub fn moveBishop(&mut self, piece_move: u64) {
        // piece_move contains initial and final positions as 1s
        self.k = self.b ^ piece_move;
    }

    pub fn moveKnight(&mut self, piece_move: u64) {
        // piece_move contains initial and final positions as 1s
        self.k = self.n ^ piece_move;
    }

    pub fn movePawn(&mut self, piece_move: u64) {
        // piece_move contains initial and final positions as 1s
        self.k = self.p ^ piece_move;
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

    pub fn kingMove(&mut self, piece_move: u64) {
        if self.st.white() {
            self.w.moveking(piece_move);
        } else {
            self.b.moveking(piece_move);
        }
        self.st = self.st.kingMove();
    }

    pub fn kingMoveCapture(&mut self, piece_move: u64) {
        if self.st.white() {
            self.w.moveking(piece_move);
            self.b.removePiece(piece_move);
        } else {
            self.b.moveking(piece_move);
            self.w.removePiece(piece_move);
        }
        self.st = self.st.kingMove();
    }

    pub fn pieceMove(&mut self, piece_move: u64, piece: char) {
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

    pub fn pieceMoveCapture(&mut self, piece_move: u64, piece: char) {
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

    pub fn rookMove(&mut self, piece_move: u64) {
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

    pub fn rookMoveCapture(&mut self, piece_move: u64) {
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

    pub fn pawnPush(&mut self, piece_move: u64) {
        if self.st.white() {
            self.w.movePawn(piece_move);
        } else {
            self.b.movePawn(piece_move);
        }
        self.st = self.st.pawnPush(piece_move);
    }

    pub fn pawnEP(&mut self, piece_move: u64) {
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

    pub fn pawnPromote(&mut self, pawn_square: u64, end_square: u64, piece: char) {
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

    pub fn pawnPromoteCapture(&mut self, pawn_square: u64, end_square: u64, piece: char) {
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

    pub fn castleL(&mut self) {
        if self.st.white() {
            self.w.moveking(0x0000000000000014u64);
            self.w.moveRook(0x0000000000000009u64);
        } else {
            self.b.moveking(0x1400000000000000u64);
            self.b.moveRook(0x0900000000000000u64);
        }
        self.st.kingMove();
    }

    pub fn castleR(&mut self) {
        if self.st.white() {
            self.w.moveking(0x0000000000000050u64);
            self.w.moveRook(0x00000000000000a0u64);
        } else {
            self.b.moveking(0x5000000000000000u64);
            self.b.moveRook(0xa000000000000000u64);
        }
        self.st.kingMove();
    }
}


}