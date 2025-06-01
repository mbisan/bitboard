#![allow(non_snake_case)]

mod lookup;
mod board;

fn print_squares(squares: u64) {
    for row in (0..8).rev() {
        for col in 0..8 {
            print!("{}", if (squares>>(8*row+col) & 1) == 1 {"x"} else {"o"});
        }
        println!();
    }
    println!();
}

fn get_coords(sq: u8) -> (u8, u8) {
    let row: u8 = sq / 8;
    let col: u8 = sq % 8;
    return (row, col);
}

fn slider_reachable(sq1: u8, sq2: u8) -> u8 {
    let (row1, col1) = get_coords(sq1);
    let (row2, col2) = get_coords(sq2);

    if row1==row2 {return 1;}
    if col1==col2 {return 1;}
    if (row1 as i8 - row2 as i8).abs() == (col1 as i8 - col2 as i8).abs() {return 2;}
    return 0;
}

fn main() {
    let mut board = board::board::Board::new();

    let piecemove = board::board::moveInfo {
        currState: board::board::State::new(),
        moveType: 2,
        movedPiece: 4,
        from: 8,
        to: 24
    };

    board.displayBoard();

    board.applyMove(piecemove);

    board.displayBoard();

    print_squares(board.w.p);
    print_squares(board.w.k);
}
