#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "chess_puzzle.h"

int get_move_evaluations(FILE *stockfish_in, FILE *stockfish_out, const char *fen, Move *moves) {
    // Set MultiPV to get top 2 moves (that's all we need for the puzzle detection)
    send_to_stockfish(stockfish_in, "setoption name MultiPV value 2");
    
    char command[MAX_LINE_LENGTH];
    sprintf(command, "position fen %s", fen);
    send_to_stockfish(stockfish_in, command);
    sprintf(command, "go depth %d", ANALYSIS_DEPTH);
    send_to_stockfish(stockfish_in, command);
    
    // Read until "bestmove" and collect the final evaluations at max depth
    char line[MAX_LINE_LENGTH];
    int num_moves = 0;
    int max_depth = 0;
    int current_multipv = 0;
    
    // Arrays to store the final evaluations at max depth
    char final_moves[2][MAX_MOVE_LENGTH] = {{0}};
    int final_scores[2] = {0};
    bool has_final_moves[2] = {false};
    
    while (fgets(line, MAX_LINE_LENGTH, stockfish_out)) {
        // Check for end of analysis
        if (strstr(line, "bestmove")) {
            break;
        }
        
        // Look for evaluation lines
        if (strstr(line, "info depth") && strstr(line, "multipv") && strstr(line, "score")) {
            int depth, multipv;
            if (sscanf(line, "info depth %d seldepth %*d multipv %d", &depth, &multipv) == 2) {
                // Track maximum depth
                if (depth > max_depth) {
                    max_depth = depth;
                }
                
                // Only process lines from the maximum depth
                if (depth == max_depth && multipv <= 2) {
                    int score = 0;
                    
                    // Parse score - handle both centipawns and mate scores
                    char *score_str = strstr(line, "score");
                    if (score_str) {
                        if (strstr(score_str, "cp")) {
                            sscanf(score_str, "score cp %d", &score);
                        } else if (strstr(score_str, "mate")) {
                            int mate_in;
                            sscanf(score_str, "score mate %d", &mate_in);
                            // Convert mate score to high centipawn value
                            score = (mate_in > 0) ? 10000 : -10000;
                        }
                    }
                    
                    // Extract the first move in the PV line
                    char *pv_str = strstr(line, " pv ");
                    if (pv_str) {
                        char move[MAX_MOVE_LENGTH] = {0};
                        sscanf(pv_str + 4, "%s", move);
                        
                        // Store this move for the multipv index (1-based in UCI)
                        if (multipv >= 1 && multipv <= 2) {
                            strcpy(final_moves[multipv-1], move);
                            final_scores[multipv-1] = score;
                            has_final_moves[multipv-1] = true;
                        }
                    }
                }
            }
        }
    }
    
    // Transfer the final moves to the result array
    for (int i = 0; i < 2; i++) {
        if (has_final_moves[i]) {
            strcpy(moves[num_moves].move, final_moves[i]);
            moves[num_moves].evaluation = final_scores[i];
            num_moves++;
        }
    }
    return num_moves;
}

bool is_puzzle_position(FILE *stockfish_in, FILE *stockfish_out, const char *fen, char *winning_move) {
    Move moves[2]; // We only need the top 2 moves
    
    int num_moves = get_move_evaluations(stockfish_in, stockfish_out, fen, moves);
    
    printf("Evaluating position: %.20s...\n", fen);
    printf("Found %d possible moves\n", num_moves);
    
    if (num_moves < 1) {
        return false; // No moves found
    }
    
    // Print the moves we found
    for (int i = 0; i < num_moves; i++) {
        printf("  Move %d (%s): evaluation %d\n", i+1, moves[i].move, moves[i].evaluation);
    }
    
    // Check if the best move is above threshold and second move (if exists) is below
    if (moves[0].evaluation >= WINNING_THRESHOLD && 
        (num_moves < 2 || moves[1].evaluation < WINNING_THRESHOLD)) {
        printf("Found 1 winning move (threshold: %d)\n", WINNING_THRESHOLD);
        strcpy(winning_move, moves[0].move);
        return true;
    }
    
    printf("Found %d winning moves (threshold: %d)\n", 
           (moves[0].evaluation >= WINNING_THRESHOLD) + 
           ((num_moves > 1 && moves[1].evaluation >= WINNING_THRESHOLD) ? 1 : 0), 
           WINNING_THRESHOLD);
    
    return false;
}

int get_top_n_moves(FILE *stockfish_in, FILE *stockfish_out, const char *fen, Move *top_moves, int n) {
    Move all_moves[MAX_MOVES];
    int num_moves = get_move_evaluations(stockfish_in, stockfish_out, fen, all_moves);
    
    // Sort moves by evaluation (descending)
    for (int i = 0; i < num_moves - 1; i++) {
        for (int j = i + 1; j < num_moves; j++) {
            if (all_moves[j].evaluation > all_moves[i].evaluation) {
                Move temp = all_moves[i];
                all_moves[i] = all_moves[j];
                all_moves[j] = temp;
            }
        }
    }
    
    // Copy top N moves
    int actual_n = (num_moves < n) ? num_moves : n;
    memcpy(top_moves, all_moves, actual_n * sizeof(Move));
    
    return actual_n;
}

void get_new_position(FILE *stockfish_in, FILE *stockfish_out, const char *fen, const char *move, char *new_fen) {
    // Set position, make move, and get FEN
    char command[MAX_LINE_LENGTH];
    sprintf(command, "position fen %s", fen);
    send_to_stockfish(stockfish_in, command);
    
    sprintf(command, "go depth 1 movetime 1");
    send_to_stockfish(stockfish_in, command);
    read_from_stockfish(stockfish_out, "bestmove");
    
    sprintf(command, "position fen %s moves %s", fen, move);
    send_to_stockfish(stockfish_in, command);
    
    send_to_stockfish(stockfish_in, "d");
    char *output = read_from_stockfish(stockfish_out, "Fen:");
    
    // Extract FEN from output
    char *fen_line = strstr(output, "Fen:");
    if (fen_line) {
        sscanf(fen_line, "Fen: %[^\n]", new_fen);
    } else {
        // Fallback - this shouldn't happen
        strcpy(new_fen, fen);
    }
}