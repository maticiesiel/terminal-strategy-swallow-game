CC = gcc
CFLAGS = -lncurses -lm

all: game

game: main.c
	$(CC) main.c -o game $(CFLAGS)

clean:
	rm -f game
