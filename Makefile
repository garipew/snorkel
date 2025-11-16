CC=gcc
CFLAGS=-Wall -Wextra -pedantic -std=c99 -fPIC -g

build: libsnorkel.so

install: libsnorkel.so
	mkdir -p /usr/local/include/snorkel 
	mkdir -p /usr/local/lib 
	cp snorkel.h /usr/local/include/snorkel
	cp libsnorkel.so /usr/local/lib/
	ldconfig

libsnorkel.so: snorkel.c snorkel.h
	$(CC) $(CFLAGS) snorkel.c -shared -o libsnorkel.so

clean:
	rm libsnorkel.so

uninstall:
	rm -r /usr/local/include/snorkel
	rm /usr/local/lib/libsnorkel.so
