CC = gcc
CFLAGS = -Wall -Wextra -g

SOURCES = main.c stockfish.c chess.c puzzle.c
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE = cpe

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)