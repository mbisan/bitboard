#![allow(non_snake_case)]

mod board;
mod lookup;
use board::board::Board;

use std::{io::{self, Write}, str::FromStr};

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

fn mainLoop() -> i32 {
    let mut board = Board::new();

    loop {
        print!(">");
        io::stdout().flush().unwrap();

        let mut input = String::new();

        match io::stdin().read_line(&mut input) {
            Ok(_) => {
                let trimmed = input.trim();
                if trimmed.len()==0 { continue; }
                let parts: Vec<&str> = input.split_whitespace().collect();
                let command = parts[0];

                match command {
                    "q" => {
                        println!("Exit...");
                        return 0;
                    }
                    "fen" => {
                        println!("Loading FEN");
                        let fen_string: String = input.split_whitespace().skip(1).collect::<Vec<&str>>().join(" ");
                        board = Board::from_fen(&fen_string);
                        board.displayBoard();
                    }
                    "display" => {
                        board.displayBoard();
                    }
                    "moves" => {
                        let moves: Vec<board::board::MoveInfo> = board.generateMovesSafe();
                        let mut printmovesloop = String::new();
                        for curr_move in moves {
                            board.applyMove(curr_move);
                            board.displayBoard();
                            board.undoMove(curr_move);
                            print!(">>");
                            io::stdout().flush().unwrap();
                            match io::stdin().read_line(&mut printmovesloop) {
                                Ok(_) => {
                                    match printmovesloop.trim() {
                                        "q" => { break; }
                                        _ => { continue;}
                                    }
                                }
                                _ => { break; }
                            }
                        }
                    }
                    _ => { continue;}
                }
            }
            Err(error) => {
                eprintln!("Error reading input: {}", error);
                return 1;
            }
        }

    }
}

fn main() {
    mainLoop();
}
