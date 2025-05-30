pub mod board {

pub struct State {
    pub state: u8,
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
    */
}

impl State {
    pub fn new() -> State {return State {state: 0b00011111};}

    pub fn white(&self) -> bool {return (self.state & 1) == 1;}
    pub fn enPassant(&self) -> bool {return (self.state>>5 & 1) == 1;}
    pub fn castle(&self) -> bool {
        if self.white() {
            return (self.state & 0b00011000) > 1;
        }
        return (self.state & 0b00000110) > 1;
    }
    pub fn kingMove(&self) -> State {
        if self.white() {
            return State {state: self.state & 0b00000110};
        }
        return State {state: (self.state & 0b0001100) + 1 };
    }
    pub fn otherMove(&self) -> State {
        if self.white() {
            return State {state: self.state & 0b00011110};
        }
        return State {state: (self.state & 0b00011110) + 1 };
    }
    pub fn lRookMove(&self) -> State {
        if self.white() {
            return State {state: self.state & 0b00010110};
        }
        return State {state: (self.state & 0b00011100) + 1 };
    }
    pub fn rRookMove(&self) -> State {
        if self.white() {
            return State {state: self.state & 0b00001110};
        }
        return State {state: (self.state & 0b00011010) + 1 };
    }
    pub fn pawnPush(&self) -> State {
        if self.white() {
            return State {state: (self.state | 0b00100000) & 0b00111110};
        }
        return State {state: ((self.state | 0b00100000) & 0b00111110) + 1 };
    }
}

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
}

pub struct Board {
    pub w: Pieces, pub b: Pieces, pub st: State
}

impl Board {
    pub fn new() -> Board {
        return Board { w: Pieces::white(), b: Pieces::black(), st: State::new() }
    }
}


}