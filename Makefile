CC=gcc
CFLAGS=-Wall -pedantic

arena.o: arena.c arena.h
	$(CC) $(CFLAGS) -c arena.c

clean:
	rm -rf *.o
