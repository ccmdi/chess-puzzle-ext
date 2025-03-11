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
        
        // Try different common paths for Stockfish
        const char* paths[] = {
            // "/usr/local/bin/stockfish",
            // "/usr/bin/stockfish",
            "/usr/games/stockfish"
            // "stockfish"  // Rely on PATH
        };
        //g
        
        for (int i = 0; i < sizeof(paths)/sizeof(paths[0]); i++) {
            execl(paths[i], "stockfish", NULL);
            // If we get here, execl failed
        }
        
        // If all attempts failed
        fprintf(stderr, "Could not execute Stockfish. Make sure it's installed and in your PATH.\n");
        exit(1);
    }
    
    // Parent process
    close(in_pipe[0]);
    close(out_pipe[1]);
    
    *stockfish_in = fdopen(in_pipe[1], "w");
    *stockfish_out = fdopen(out_pipe[0], "r");
    
    if (!*stockfish_in || !*stockfish_out) {
        perror("fdopen");
        return false;
    }
    
    // Initialize Stockfish
    send_to_stockfish(*stockfish_in, "uci");
    send_to_stockfish(*stockfish_in, "setoption name Threads value 1");
    send_to_stockfish(*stockfish_in, "setoption name MultiPV value 2");
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
    size_t total_len = 0;
    
    while (fgets(line, MAX_LINE_LENGTH, stockfish_out)) {
        size_t line_len = strlen(line);
        
        // Check if adding this line would overflow the buffer
        if (total_len + line_len >= MAX_BUFFER_SIZE - 1) {
            fprintf(stderr, "Warning: Buffer overflow prevented in read_from_stockfish\n");
            break;
        }
        
        strcat(buffer, line);
        total_len += line_len;
        
        if (strstr(line, until_marker)) {
            break;
        }
    }
    
    return buffer;
}