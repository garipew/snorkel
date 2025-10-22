#ifndef ARENA_H
#define ARENA_H

#define _GNU_SOURCE
#define REGION_SIZE 8192 // PAGE_SIZE * 2

#include <stdint.h>
#include <stddef.h>

typedef struct Region Region;
struct Region{
	Region *next;
	unsigned char *avail;
	unsigned char *limit;
};

typedef struct {
	Region *start, *end;
	Region *current;
} Arena;

void arena_grow(Arena*);
void* arena_alloc(Arena*, size_t, size_t);
void arena_free(Arena*);
void arena_reset(Arena*);
#endif // ARENA_H
