#![allow(non_snake_case)]

mod lookup;
mod board;

fn main() {
    let mut board = board::board::Board::new();

    let moves = board.generateMovesSafe();

    for pieceMove in moves {
        board.applyMove(pieceMove);
        board.displayBoard();
        board.undoMove(pieceMove);
    }

}
