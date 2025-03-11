#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "chess_puzzle.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <FEN>\n", argv[0]);
        return 1;
    }
    
    char *fen = argv[1];
    FILE *stockfish_in, *stockfish_out;
    
    if (!initialize_stockfish(&stockfish_in, &stockfish_out)) {
        fprintf(stderr, "Failed to initialize Stockfish\n");
        return 1;
    }
    
    // Check if white is to move in the provided FEN
    bool white_to_move = (strstr(fen, " w ") != NULL);
    
    // Find the longest puzzle line
    PositionHistory history[100] = {0};
    PuzzleLine longest_line = find_longest_puzzle(stockfish_in, stockfish_out, fen, white_to_move, 0, history, 0);
    
    if (longest_line.length > 0) {
        printf("Longest puzzle line (%d half-moves): %s\n", longest_line.length, longest_line.moves);
    } else {
        printf("No valid puzzle continuation found.\n");
    }
    
    close_stockfish(stockfish_in, stockfish_out);
    return 0;
}