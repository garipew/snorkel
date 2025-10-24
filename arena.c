#include "arena.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>

#define round_align(start, align) \
	(((start)+(align)-1) & ~((align)-1))

#define have_space(region, size, align) \
	(round_align((uintptr_t)region->avail, align) + size < (uintptr_t)region->limit)


void* arena_grow(Arena *arena, size_t at_least){
	if(arena->region_size < REGION_SIZE){
		arena->region_size = REGION_SIZE;
	}
	if(at_least > REGION_SIZE && !arena->start){
		arena->region_size = round_align(sizeof(Region)+at_least, 16);
	}
	void *new_block = mmap(NULL, arena->region_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	if(new_block == MAP_FAILED){
		return NULL;
	}
	Region *new_region = new_block;
	new_region->avail = (u8*)new_region + sizeof(*new_region);
	new_region->limit = (u8*)new_region + arena->region_size;
	arena->current = new_region;
	if(!arena->start){
		arena->start = new_region;
		arena->end = arena->start;
		return arena->current;
	}
	arena->end->next = new_region;
	arena->end = new_region;
	return arena->current;
}

int find_space(Arena *arena, size_t size, size_t align){
	for(; arena->current; arena->current = arena->current->next){
		if(have_space(arena->current, size, align)){
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
	if(arena->start && size > arena->region_size){
		// TODO(...): Unsure if this should be a thing. Right now, the first allocation
		// defines the scope of an arena, ensuring the congruency of the sizes of regions.
		// But I don't know if this should be responsibility of the allocator...  
		// In fact this doesn't solve the possibility of fragmentation. There is no solution
		// but the informed usage. I have heard that arenas aren't the 'one size fits all'
		// allocator such as malloc, but at the same time, I can not have a constant
		// max size for regions.
		// This will at least help me understand how I would like to use arenas...
		// TLDR: What to do if I have 4KB regions and need 400MB?
		fprintf(stderr, "This arena does not allow such allocations.\n");
		fprintf(stderr, "The local max is %luB.\n", arena->region_size);
		return NULL;
	}
	if(find_space(arena, size, align)){
		memset((u8*)round_align((uintptr_t)arena->current->avail, align), 0, size);
	}else if(!arena_grow(arena, size)){
		return NULL;
	}
	void *new_ptr = (void*)round_align((uintptr_t)arena->current->avail, align);
	arena->current->avail = (void*)(size+(uintptr_t)new_ptr);
	return new_ptr;
}

void arena_reset(Arena *arena){
	for(arena->current = arena->start; arena->current; arena->current = arena->current->next){
		arena->current->avail = (u8*)arena->current + sizeof(*arena->current);
	}
	arena->current = arena->start;
}

void arena_free(Arena *arena){
	arena->current = arena->start;
	for(; arena->current;){
		arena->end = arena->current;
		arena->current = arena->current->next;
		munmap(arena->end, arena->region_size);
	}
	arena->start = NULL;
	arena->end = NULL;
	arena->current = NULL;
	arena->region_size = 0;
}
