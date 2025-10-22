#include "arena.h"
#include <stdlib.h>
#include <sys/mman.h>

#define round_align(start, align) \
	(((start)+(align)-1) & ~((align)-1))

void arena_grow(Arena *arena){
	void *new_block = mmap(NULL, REGION_SIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
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

void* arena_alloc(Arena *arena, size_t size, size_t align){
	if(size == 0){
		return NULL;
	}
	if(align == 0){
		align = 1;
	}
	if(	!arena->start ||
		round_align((uintptr_t)arena->end->avail, align) + size >= (uintptr_t)arena->end->limit
	){
		arena_grow(arena);
	}
	void *new_ptr = (void*)round_align((uintptr_t)arena->end->avail, align);
	arena->end->avail = (void*)(size+(uintptr_t)new_ptr);
	return new_ptr;
}

void arena_free(Arena *arena){
	if(!arena->start)
		return;
	for(Region *at=arena->start, *dangling; at;){
		dangling = at;
		at = dangling->next;
		munmap(dangling, REGION_SIZE);
	}
	arena->start = NULL;
	arena->end = NULL;
}
