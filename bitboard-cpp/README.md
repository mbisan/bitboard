In VSCode, execute the "C++: run (desktop)" task to compile and run the program.

The program is a CLI, used as follows:

- p: prints chessboard
- f FEN_STRING: loads FEN position onto the board
- r: resets the board to the initial position
- m INITIAL_SQUARE FINAL_SQUARE: moves piece from INITIAL to FINAL square, does not check is the move is valid
- e NUM_STEPS: computes perft NUM_STEPS and prints the elapsed time in milliseconds
- n NUM_STEPS: computes perft NUM_STEPS