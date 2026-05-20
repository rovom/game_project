CC = gcc
CFLAGS = -g -Wall -Wextra -Wpedantic
LIBS = -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer

prog: main.o source.o joueur.o character_select.o highscore.o minimap.o save_system.o main_menu.o puzzle_game/game.o
	$(CC) main.o source.o joueur.o character_select.o highscore.o minimap.o save_system.o main_menu.o puzzle_game/game.o -o prog $(LIBS)

main.o: main.c header.h highscore.h minimap/minimap.h save_system.h main_menu.h
	$(CC) -c main.c $(CFLAGS)

source.o: source.c header.h
	$(CC) -c source.c $(CFLAGS)

joueur.o: joueur.c header.h
	$(CC) -c joueur.c $(CFLAGS)

character_select.o: character_select.c header.h
	$(CC) -c character_select.c $(CFLAGS)

highscore.o: highscore.c highscore.h
	$(CC) -c highscore.c $(CFLAGS)

minimap.o: minimap/minimap.c minimap/minimap.h
	$(CC) -c minimap/minimap.c $(CFLAGS)

save_system.o: save_system.c save_system.h header.h
	$(CC) -c save_system.c $(CFLAGS)

main_menu.o: main_menu.c main_menu.h header.h
	$(CC) -c main_menu.c $(CFLAGS)

puzzle_game/game.o: puzzle_game/game.c puzzle_game/game.h puzzle_game/assets.h
	$(CC) -c puzzle_game/game.c -o puzzle_game/game.o $(CFLAGS)

clean:
	rm -f prog main.o source.o joueur.o character_select.o highscore.o minimap.o save_system.o main_menu.o puzzle_game/game.o
	$(MAKE) -C puzzle_game clean
