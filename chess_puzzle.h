#ifndef CHESS_PUZZLE_H
#define CHESS_PUZZLE_H

#include <stdio.h>
#include <stdbool.h>

// Constants
#define MAX_FEN_LENGTH 128
#define MAX_MOVE_LENGTH 10
#define MAX_BUFFER_SIZE 4096
#define MAX_LINE_LENGTH 1024
#define MAX_MOVES 256
#define WINNING_THRESHOLD 150 // centipawns
#define TOP_N_MOVES 3 // Number of top moves for black
#define ANALYSIS_DEPTH 20

// Structure for moves with evaluation
typedef struct {
    char move[MAX_MOVE_LENGTH];
    int evaluation; // in centipawns
} Move;

// Structure for puzzle sequences
typedef struct {
    char moves[MAX_LINE_LENGTH];
    int length; // number of half-moves
} PuzzleLine;

// Stockfish communication functions
bool initialize_stockfish(FILE **stockfish_in, FILE **stockfish_out);
void close_stockfish(FILE *stockfish_in, FILE *stockfish_out);
void send_to_stockfish(FILE *stockfish_in, const char *command);
char* read_from_stockfish(FILE *stockfish_out, const char *until_marker);

// Chess position analysis functions
int get_move_evaluations(FILE *stockfish_in, FILE *stockfish_out, const char *fen, Move *moves);
bool is_puzzle_position(FILE *stockfish_in, FILE *stockfish_out, const char *fen, char *winning_move);
int get_top_n_moves(FILE *stockfish_in, FILE *stockfish_out, const char *fen, Move *top_moves, int n);
void get_new_position(FILE *stockfish_in, FILE *stockfish_out, const char *fen, const char *move, char *new_fen);

// Puzzle finding algorithm
PuzzleLine find_longest_puzzle(FILE *stockfish_in, FILE *stockfish_out, const char *fen, bool white_to_move, int depth);

#endif // CHESS_PUZZLE_H