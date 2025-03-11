#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "chess_puzzle.h"

int get_move_evaluations(FILE *stockfish_in, FILE *stockfish_out, const char *fen, Move *moves, int multipv_count) {
    // Set MultiPV to the requested number of moves
    char multipv_cmd[64];
    sprintf(multipv_cmd, "setoption name MultiPV value %d", multipv_count);
    send_to_stockfish(stockfish_in, multipv_cmd);
    
    char command[MAX_LINE_LENGTH];
    sprintf(command, "position fen %s", fen);
    send_to_stockfish(stockfish_in, command);
    sprintf(command, "go depth %d", ANALYSIS_DEPTH);
    send_to_stockfish(stockfish_in, command);
    
    // Read until "bestmove" and collect the final evaluations at max depth
    char line[MAX_LINE_LENGTH];
    int num_moves = 0;
    int max_depth = 0;
    
    // Arrays to store the final evaluations at max depth
    char *final_moves = malloc(multipv_count * MAX_MOVE_LENGTH);
    int *final_scores = malloc(multipv_count * sizeof(int));
    bool *has_final_moves = malloc(multipv_count * sizeof(bool));
    
    if (!final_moves || !final_scores || !has_final_moves) {
        fprintf(stderr, "Memory allocation failed\n");
        if (final_moves) free(final_moves);
        if (final_scores) free(final_scores);
        if (has_final_moves) free(has_final_moves);
        return 0;
    }
    
    // Initialize arrays
    for (int i = 0; i < multipv_count; i++) {
        memset(&final_moves[i * MAX_MOVE_LENGTH], 0, MAX_MOVE_LENGTH);
        final_scores[i] = 0;
        has_final_moves[i] = false;
    }
    
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
                if (depth == max_depth && multipv <= multipv_count) {
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
                        if (multipv >= 1 && multipv <= multipv_count) {
                            strcpy(&final_moves[(multipv-1) * MAX_MOVE_LENGTH], move);
                            final_scores[multipv-1] = score;
                            has_final_moves[multipv-1] = true;
                        }
                    }
                }
            }
        }
    }
    
    // Transfer the final moves to the result array
    for (int i = 0; i < multipv_count; i++) {
        if (has_final_moves[i]) {
            strcpy(moves[num_moves].move, &final_moves[i * MAX_MOVE_LENGTH]);
            moves[num_moves].evaluation = final_scores[i];
            num_moves++;
        }
    }
    
    free(final_moves);
    free(final_scores);
    free(has_final_moves);
    
    return num_moves;
}

bool is_puzzle_position(FILE *stockfish_in, FILE *stockfish_out, const char *fen, char *winning_move) {
    Move moves[2]; // We only need the top 2 moves
    
    int num_moves = get_move_evaluations(stockfish_in, stockfish_out, fen, moves, 2);
    
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
    return get_move_evaluations(stockfish_in, stockfish_out, fen, top_moves, n);
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