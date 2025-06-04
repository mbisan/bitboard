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
    let starting_pos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    let mut board = Board::new();

    let mut board2 = Board::from_fen("rnbqkbnr/ppppp1pp/8/5p1Q/4P3/8/PPPP1PPP/RNB1KBNR b KQkq f 0 1");

    board2.displayBoard();

    println!("Perft 3: {}", perft(&mut board, 3)); // ok
    // println!("Perft 4: {}", perft(&mut board, 4)); // ok
    // println!("Perft 5: {}", perft(&mut board, 5)); // ok
    // println!("Perft 6: {}", perft(&mut board, 6)); // 119,060,538 - 119,060,324 no lo hace bien (posibles razones: discovery check/double check)
}
