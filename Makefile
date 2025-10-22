CC=gcc
CFLAGS=-Wall -Wextra -pedantic -std=c99

arena.o: arena.c arena.h
	$(CC) $(CFLAGS) -c arena.c

clean:
	rm -rf *.o
