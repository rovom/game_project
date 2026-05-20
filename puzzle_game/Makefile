# ============================================================
#  Makefile  –  Interstellar Puzzle (SDL2)
# ============================================================

CC      = gcc
TARGET  = puzzle
SRCS    = main.c game.c
OBJS    = $(SRCS:.c=.o)

# Base flags
CFLAGS  = -Wall -Wextra -O2 -std=c11 \
          $(shell sdl2-config --cflags)

LDFLAGS = $(shell sdl2-config --libs) \
          -lSDL2_image \
          -lSDL2_ttf \
          -lm


all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c game.h assets.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

run: all
	./$(TARGET)

# Phony targets
.PHONY: all clean run
