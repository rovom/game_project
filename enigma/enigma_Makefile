CC     = gcc
CFLAGS = -g -Wall -Wextra
LIBS   = -lSDL2 -lSDL2_ttf -lm

enigma: main.o game_logic.o game_draw.o
	$(CC) main.o game_logic.o game_draw.o -o enigma $(LIBS)

main.o: main.c game.h
	$(CC) -c main.c $(CFLAGS)

game_logic.o: game_logic.c game.h
	$(CC) -c game_logic.c $(CFLAGS)

game_draw.o: game_draw.c game.h
	$(CC) -c game_draw.c $(CFLAGS)

clean:
	rm -f enigma main.o game_logic.o game_draw.o
