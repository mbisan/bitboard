#![allow(non_snake_case)]

mod board;
mod lookup;
use board::board::Board;

fn perft(board: &mut Board, depth: u32) -> u64 {
    if depth == 0 {
        return 1;
    }

    let moves = board.generateMovesSafe();
    let mut nodes: u64 = 0;

    for mv in moves {
        board.applyMove(mv);
        nodes += perft(board, depth - 1);
        board.undoMove(mv);
    }

    nodes
}

fn print_squares(squares: u64) {
    for row in (0..8).rev() {
        for col in 0..8 {
            print!("{}", if (squares>>(8*row+col) & 1) == 1 {"x"} else {"o"});
        }
        println!();
    }
    println!();
}

fn main() {
    let mut board = Board::new();

    println!("Perft 3: {}", perft(&mut board, 3)); // prints 1545
    println!("Perft 4: {}", perft(&mut board, 4)); // ok
    println!("Perft 5: {}", perft(&mut board, 5)); // ok
    println!("Perft 6: {}", perft(&mut board, 6)); // 119,060,538 - 119,060,324 no lo hace bien (posibles razones: discovery check/double check)
}
