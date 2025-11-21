CC=gcc
CFLAGS=-Wall -Wextra -pedantic -std=c99 -g
CLIBS=-I/usr/local/include/snorkel -L/usr/local/lib -lsnorkel

build: libsnorkel.so

install: libsnorkel.so
	mkdir -p /usr/local/include/snorkel 
	mkdir -p /usr/local/lib 
	cp snorkel.h /usr/local/include/snorkel
	cp libsnorkel.so /usr/local/lib/
	ldconfig

all: install co_example

libsnorkel.so: snorkel.c snorkel.h
	$(CC) $(CFLAGS) -fPIC snorkel.c -shared -o libsnorkel.so

co_example: examples/co_example.c
	$(CC) $(CFLAGS) examples/co_example.c -o co_example $(CLIBS)
	
clean:
	rm -rf libsnorkel.so co_example

uninstall:
	rm -r /usr/local/include/snorkel
	rm /usr/local/lib/libsnorkel.so
