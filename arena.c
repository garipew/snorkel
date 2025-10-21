#include "arena.h"
#include <stdlib.h>
#include <sys/mman.h>

void arena_grow(Arena *arena){
	void *new_block = mmap(NULL, REGION_SIZE, PROT_READ|PROT_WRITE, MAP_ANONYMOUS, -1, 0);
	if(new_block == MAP_FAILED){
		return;
	}
	Region *new_region = new_block;
	new_region->avail = (unsigned char*)new_region + sizeof(*new_region);
	new_region->limit = (unsigned char*)new_region + REGION_SIZE;
	if(!arena->start){
		arena->start = new_region;
		arena->end = arena->start;
		return;
	}
	arena->end->next = new_region;
	arena->end = new_region;
}
