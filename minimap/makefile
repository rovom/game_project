#
prog: main.o minimap.o
	gcc main.o minimap.o -o prog -lSDL2 -g


main.o: main.c
	gcc -c main.c -g


minimap.o: minimap.c minimap.h
	gcc -c minimap.c -g


clean:
	rm -f *.o prog
