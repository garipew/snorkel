#ifndef ARENA_H
#define ARENA_H

#include <stdint.h>
#include <stddef.h>

#define REGION_SIZE 8192 // PAGE_SIZE * 2

typedef struct Region Region;
struct Region{
	Region *next;
	unsigned char *avail;
	unsigned char *limit;
};

typedef struct {
	Region *start, *end;
} Arena;

void arena_grow(Arena*);
void* arena_alloc(Arena*, size_t, size_t);
void arena_free(Arena*);
#endif // ARENA_H
