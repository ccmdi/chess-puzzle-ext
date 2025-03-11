#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "chess_puzzle.h"

int get_move_evaluations(FILE *stockfish_in, FILE *stockfish_out, const char *fen, Move *moves) {
    char command[MAX_LINE_LENGTH];
    sprintf(command, "position fen %s", fen);
    send_to_stockfish(stockfish_in, command);
    sprintf(command, "go depth %d", ANALYSIS_DEPTH);
    send_to_stockfish(stockfish_in, command);
    
    char *output = read_from_stockfish(stockfish_out, "bestmove");
    
    // Parse the output to get move evaluations
    int num_moves = 0;
    char *line = strtok(output, "\n");
    
    while (line != NULL) {
        // Parse "info" lines with scores
        if (strstr(line, "info") && strstr(line, "cp") && strstr(line, "pv")) {
            char move[MAX_MOVE_LENGTH];
            int score;
            
            char *score_str = strstr(line, "cp");
            if (score_str) {
                sscanf(score_str, "cp %d", &score);
                
                char *pv_str = strstr(line, "pv");
                if (pv_str) {
                    sscanf(pv_str, "pv %s", move);
                    
                    // Store the move and its evaluation
                    strcpy(moves[num_moves].move, move);
                    moves[num_moves].evaluation = score;
                    num_moves++;
                }
            }
        }
        
        line = strtok(NULL, "\n");
    }
    
    return num_moves;
}

bool is_puzzle_position(FILE *stockfish_in, FILE *stockfish_out, const char *fen, char *winning_move) {
    printf("Evaluating position: %.20s...\n", fen);
    
    Move moves[MAX_MOVES];
    int num_moves = get_move_evaluations(stockfish_in, stockfish_out, fen, moves);
    
    printf("Found %d possible moves\n", num_moves);
    
    // Count moves above the winning threshold
    int winning_moves = 0;
    int best_move_index = -1;
    int best_eval = -10000;
    
    for (int i = 0; i < num_moves; i++) {
        printf("  Move %s: evaluation %d\n", moves[i].move, moves[i].evaluation);
        
        if (moves[i].evaluation >= WINNING_THRESHOLD) {
            winning_moves++;
            if (moves[i].evaluation > best_eval) {
                best_eval = moves[i].evaluation;
                best_move_index = i;
            }
        }
    }
    
    printf("Found %d winning moves (threshold: %d)\n", winning_moves, WINNING_THRESHOLD);
    
    // If exactly one winning move, return it
    if (winning_moves == 1 && best_move_index >= 0) {
        strcpy(winning_move, moves[best_move_index].move);
        return true;
    }
    
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