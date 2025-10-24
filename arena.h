#ifndef ARENA_H
#define ARENA_H

#define _GNU_SOURCE
#define REGION_SIZE 8192 // PAGE_SIZE * 2

#ifdef __GNUC__
#define ALIGNOF __alignof__
#else
#error "Compiler support not available yet"
#endif

#include <stdint.h>
#include <stddef.h>

typedef uint8_t u8;

typedef struct Region Region;
struct Region{
	Region *next;
	u8 *avail;
	u8 *limit;
};

typedef struct {
	Region *start, *end;
	Region *current;
	size_t region_size;
} Arena;

void* arena_grow(Arena*, size_t);
void* arena_alloc(Arena*, size_t, size_t);
void arena_free(Arena*);
void arena_reset(Arena*);
#endif // ARENA_H
