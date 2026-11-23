CC      = gcc
CFLAGS  = -Wall -std=c11
LDFLAGS = -lSDL2 -lSDL2_ttf -lm

all:
	$(CC) $(CFLAGS) main.c dice.c init.c placement.c eval.c round.c \
	ui_core.c ui_render.c ui_events.c -o stone_age $(LDFLAGS)

tests:
	$(CC) $(CFLAGS) test_game.c dice.c init.c placement.c eval.c round.c \
	-o test_runner -lm
	./test_runner

clean:
	rm -f stone_age test_runner