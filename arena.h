#ifndef ARENA_H
#define ARENA_H

#include <stdint.h>

#define REGION_SIZE 4096

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
#endif // ARENA_H
