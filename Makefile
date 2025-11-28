CC=gcc
CFLAGS=-Wall -Wextra -Wno-cast-function-type -std=c99 -g -O3
CLIBS=-I/usr/local/include/snorkel -L/usr/local/lib -lsnorkel

.PHONY: build install uninstall all clean test

build: libsnorkel.so

all: install co_example

test: $(addsuffix .test, $(basename $(wildcard tests/*.c)))

install: libsnorkel.so
	mkdir -p /usr/local/include/snorkel 
	mkdir -p /usr/local/lib 
	cp snorkel.h /usr/local/include/snorkel
	cp libsnorkel.so /usr/local/lib/
	ldconfig

libsnorkel.so: snorkel.c snorkel.h
	$(CC) $(CFLAGS) -fPIC snorkel.c -shared -o libsnorkel.so

co_example: examples/co_example.c
	$(CC) $(CFLAGS) examples/co_example.c -o co_example $(CLIBS)
	
clean:
	rm -rf libsnorkel.so co_example

uninstall:
	rm -r /usr/local/include/snorkel
	rm /usr/local/lib/libsnorkel.so

tests/%.test: tests/%.c
	@$(CC) $(CFLAGS) $< -o $@ $(CLIBS)
	@./$@ 2>&1 | diff -q $(addsuffix .ok, $(basename $@)) - || \
		(echo "Test $@ failed" && exit 1)
	@rm -rf tests/*.test
	@echo "$(notdir $@) OK"
