#include "arena.h"
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#define round_align(start, align) \
	(((start)+(align)-1) & ~((align)-1))

#define have_space(region, size, align) \
	(round_align((uintptr_t)region->avail, align) + size < (uintptr_t)region->limit)


void arena_grow(Arena *arena){
	void *new_block = mmap(NULL, REGION_SIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	if(new_block == MAP_FAILED){
		return;
	}
	Region *new_region = new_block;
	new_region->avail = (unsigned char*)new_region + sizeof(*new_region);
	new_region->limit = (unsigned char*)new_region + REGION_SIZE;
	arena->current = new_region;
	if(!arena->start){
		arena->start = new_region;
		arena->end = arena->start;
		return;
	}
	arena->end->next = new_region;
	arena->end = new_region;
}

int find_space(Arena *arena, size_t size, size_t align){
	for(; arena->current; arena->current = arena->current->next){
		if(have_space(arena->current, size, align)){
			// TODO(garipew): Unsure where to clean the memory, right now it cleans
			// only when it is about to be used. But here sometimes it might clean
			// already cleaned memory. The other option is to do it in arena_reset,
			// but then it could get very heavy. I could also use a flag "dirty" and
			// just clean memory that has it set, but then we are starting to get very
			// verbose, I don't like the idea very much.
			// TLDR: Find a better place for cleanup
			memset(arena->current->avail, 0, arena->current->limit-arena->current->avail);
			return 1;
		}
	}
	return 0;
}

void* arena_alloc(Arena *arena, size_t size, size_t align){
	if(size == 0){
		return NULL;
	}
	if(align == 0){
		align = 1;
	}
	if(!find_space(arena, size, align)){
		arena_grow(arena);
	}
	void *new_ptr = (void*)round_align((uintptr_t)arena->current->avail, align);
	arena->current->avail = (void*)(size+(uintptr_t)new_ptr);
	return new_ptr;
}

void arena_reset(Arena *arena){
	for(arena->current = arena->start; arena->current; arena->current = arena->current->next){
		arena->current->avail = (unsigned char*)arena->current + sizeof(*arena->current);
	}
	arena->current = arena->start;
}

void arena_free(Arena *arena){
	arena->current = arena->start;
	for(; arena->current;){
		arena->end = arena->current;
		arena->current = arena->current->next;
		munmap(arena->end, REGION_SIZE);
	}
	arena->start = NULL;
	arena->end = NULL;
	arena->current = NULL;
}
