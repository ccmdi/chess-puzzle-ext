CC = gcc
CFLAGS = -Wall -Wextra -g

SOURCES = main.c stockfish.c chess.c puzzle.c
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE = cpe
HEADERS = chess_puzzle.h

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@

# Make all object files depend on all header files
$(OBJECTS): $(HEADERS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)