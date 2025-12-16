CC=gcc
CFLAGS=-Wall -Wextra -Wno-cast-function-type -Wno-override-init -std=c99 -g -O3
CLIBS=-I/usr/local/include/snorkel -L/usr/local/lib -lsnorkel

.PHONY: build install uninstall all clean test

build: libsnorkel.so

all: install co_example

test: $(addsuffix .test, $(basename $(wildcard tests/*.c))) tests/libsnorkel.so
	@rm -rf tests/libsnorkel.so

install: libsnorkel.so
	mkdir -p /usr/local/include/snorkel
	mkdir -p /usr/local/lib
	cp snorkel.h /usr/local/include/snorkel
	cp libsnorkel.so /usr/local/lib/
	ldconfig

libsnorkel.so: CFLAGS += -fPIC -shared
libsnorkel.so: snorkel.c snorkel.h
	$(CC) $(CFLAGS) snorkel.c -o libsnorkel.so

tests/libsnorkel.so: CFLAGS += -fPIC -shared -DSNORKEL_TEST
tests/libsnorkel.so: snorkel.c snorkel.h
	@$(CC) $(CFLAGS) snorkel.c -o tests/libsnorkel.so
	
clean:
	rm -rf libsnorkel.so co_example

uninstall:
	rm -r /usr/local/include/snorkel
	rm /usr/local/lib/libsnorkel.so

tests/%.test: CFLAGS += -DSNORKEL_TEST
tests/%.test: tests/%.c tests/libsnorkel.so
	@$(CC) $(CFLAGS) $< -o $@ -I/usr/local/include/snorkel -Ltests -lsnorkel \
		-Wl,-rpath,'$$ORIGIN'
	@./$@ 2>&1 | diff -q $(addsuffix .ok, $(basename $@)) - || \
		(echo "Test $@ failed" && exit 1)
	@rm -rf tests/*.test
	@echo "$(notdir $@) OK"
