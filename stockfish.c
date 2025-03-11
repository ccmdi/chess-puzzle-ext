#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include "chess_puzzle.h"

bool initialize_stockfish(FILE **stockfish_in, FILE **stockfish_out) {
    int in_pipe[2], out_pipe[2];
    
    if (pipe(in_pipe) < 0 || pipe(out_pipe) < 0) {
        perror("pipe");
        return false;
    }
    
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return false;
    }
    
    if (pid == 0) {  // Child process
        close(in_pipe[1]);
        close(out_pipe[0]);
        
        dup2(in_pipe[0], STDIN_FILENO);
        dup2(out_pipe[1], STDOUT_FILENO);
        
        close(in_pipe[0]);
        close(out_pipe[1]);
        
        execl("/usr/games/stockfish", "stockfish", NULL);
        perror("execl");
        exit(1);
    }
    
    // Parent process
    close(in_pipe[0]);
    close(out_pipe[1]);
    
    *stockfish_in = fdopen(in_pipe[1], "w");
    *stockfish_out = fdopen(out_pipe[0], "r");
    
    // Initialize Stockfish
    send_to_stockfish(*stockfish_in, "uci");
    send_to_stockfish(*stockfish_in, "setoption name Threads value 1");
    send_to_stockfish(*stockfish_in, "isready");
    
    // Wait for readyok
    char buffer[MAX_BUFFER_SIZE];
    while (fgets(buffer, MAX_BUFFER_SIZE, *stockfish_out)) {
        if (strstr(buffer, "readyok")) break;
    }
    
    return true;
}

void close_stockfish(FILE *stockfish_in, FILE *stockfish_out) {
    if (stockfish_in) {
        send_to_stockfish(stockfish_in, "quit");
        fclose(stockfish_in);
    }
    
    if (stockfish_out) {
        fclose(stockfish_out);
    }
}

void send_to_stockfish(FILE *stockfish_in, const char *command) {
    fprintf(stockfish_in, "%s\n", command);
    fflush(stockfish_in);
}

char* read_from_stockfish(FILE *stockfish_out, const char *until_marker) {
    static char buffer[MAX_BUFFER_SIZE];
    buffer[0] = '\0';
    
    char line[MAX_LINE_LENGTH];
    while (fgets(line, MAX_LINE_LENGTH, stockfish_out)) {
        strcat(buffer, line);
        if (strstr(line, until_marker)) {
            break;
        }
    }
    
    return buffer;
}