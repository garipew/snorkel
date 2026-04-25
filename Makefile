CC=gcc
CFLAGS=-Wall -Wextra -Wno-cast-function-type -Wno-override-init -std=c99 -g -O3

.PHONY: test

test: $(addsuffix .test, $(basename $(wildcard tests/*.c)))

tests/%.test: CFLAGS += -DSNORKEL_TEST
tests/%.test: tests/%.c
	@$(CC) $(CFLAGS) $< -o $@ \
		-Wl,-rpath,'$$ORIGIN'
	@./$@ 2>&1 | diff -q $(addsuffix .ok, $(basename $@)) - || \
		(echo "Test $@ failed" && exit 1)
	@rm -rf tests/*.test
	@echo "$(notdir $@) OK"
