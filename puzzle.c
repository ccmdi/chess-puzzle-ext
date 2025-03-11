#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "chess_puzzle.h"

PuzzleLine find_longest_puzzle(FILE *stockfish_in, FILE *stockfish_out, const char *fen, bool white_to_move, int depth) {
    PuzzleLine result = {0};
    
    // Base case: if it's White's turn, check if this is a puzzle position
    if (white_to_move) {
        char winning_move[MAX_MOVE_LENGTH];
        
        if (is_puzzle_position(stockfish_in, stockfish_out, fen, winning_move)) {
            // This is a valid puzzle position - make the move and continue
            char new_fen[MAX_FEN_LENGTH];
            get_new_position(stockfish_in, stockfish_out, fen, winning_move, new_fen);
            
            // Recursively find the longest continuation
            PuzzleLine continuation = find_longest_puzzle(stockfish_in, stockfish_out, new_fen, false, depth + 1);
            
            // Construct the result
            strcpy(result.moves, winning_move);
            if (continuation.length > 0) {
                strcat(result.moves, " ");
                strcat(result.moves, continuation.moves);
            }
            
            result.length = 1 + continuation.length;
        }
        
        return result;
    }
    
    // Black's turn: try top N moves and find the one extending the puzzle the longest
    // Increase this to explore more of Black's options
    #define BLACK_MOVES_TO_TRY 5
    Move top_moves[BLACK_MOVES_TO_TRY];
    
    // Get the top moves for Black (we want more than TOP_N_MOVES here to find longer puzzles)
    int n_top_moves = get_move_evaluations(stockfish_in, stockfish_out, fen, top_moves, BLACK_MOVES_TO_TRY);
    
    printf("Black's options at position: %.20s...\n", fen);
    for (int i = 0; i < n_top_moves; i++) {
        printf("  Option %d: %s (eval: %d)\n", i+1, top_moves[i].move, top_moves[i].evaluation);
    }
    
    PuzzleLine best_line = {0};
    
    for (int i = 0; i < n_top_moves; i++) {
        printf("Trying Black's move: %s\n", top_moves[i].move);
        
        char new_fen[MAX_FEN_LENGTH];
        get_new_position(stockfish_in, stockfish_out, fen, top_moves[i].move, new_fen);
        
        PuzzleLine line = find_longest_puzzle(stockfish_in, stockfish_out, new_fen, true, depth + 1);
        
        printf("  → Move %s leads to puzzle length: %d\n", top_moves[i].move, line.length);
        
        if (line.length > best_line.length) {
            printf("  → New best move for Black: %s (extends puzzle to %d moves)\n", 
                   top_moves[i].move, line.length + 1);
            
            strcpy(best_line.moves, top_moves[i].move);
            if (line.length > 0) {
                strcat(best_line.moves, " ");
                strcat(best_line.moves, line.moves);
            }
            
            best_line.length = 1 + line.length;
        }
    }
    
    return best_line;
}