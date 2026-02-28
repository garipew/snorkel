#ifndef SNORKEL_ARENA_H
#define SNORKEL_ARENA_H

#define REGION_SIZE 32768 // PAGE_SIZE * 8

#ifdef __STDC_VERSION__
#if __STDC_VERSION__ >= 201112L
#include <stdalign.h>
#if __STDC_VERSION__ >= 202311L
#define ALIGNOF(T) alignof(T)
#else
#define ALIGNOF(T) _Alignof(T)
#endif // __STDC_VERSION__ >= 202311L
#elif __STDC_VERSION__ >= 199901L
#ifdef __GNUC__
#define ALIGNOF(T) __alignof__(T)
#endif // __GNUC__
#endif // __STDC_VERSION__ >= 201112L
#endif // __STDC_VERSION__
       
#ifndef ALIGNOF
#error "Missing ALIGNOF: compiler/standard not supported"
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

struct flag {
	Region *region;
	u8 *addr;
	struct flag *prev;
};

typedef struct {
	Region *start, *end;
	Region *current;
	size_t region_size;
	u8 fixed_size;
	struct flag *checkpoint;
} Arena;

void* arena_grow(Arena*, size_t);
void* arena_alloc(Arena*, size_t, size_t);
void arena_free(Arena*);
void arena_reset(Arena*);
void arena_flag(Arena*);
void arena_restore(Arena*);
#endif // SNORKEL_ARENA_H

#ifdef SNORKEL_IMPLEMENTATION
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

void arena_zero(u8* dst, size_t len){
	u8* ptr = dst;
	while(ptr < dst + len){
		*ptr = 0;
		ptr++;
	}
}

void* arena_alloc(Arena *arena, size_t size, size_t align){
	if(size == 0){
		return NULL;
	}
	if(align == 0){
		align = 1;
	}
	if(arena->start && size > arena->region_size){
		// TODO(garipew): Unsure if this should be a thing. Right now, the first allocation
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
	// TODO(garipew): Right now, alloc also cleans the memory. This is nice to do, already an
	// improvement compared to doing so in reset. But also there's some redundancy here...
	// Maybe I could find a way to clean exactly what is going to be used and nothing more?
	if(find_space(arena, size, align)){
		arena_zero(arena->current->avail, arena->current->limit-arena->current->avail);
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

void arena_flag(Arena *a){
	if(a->current == NULL){
		return;
	}
	Region *current = a->current;
	u8 *addr = current->avail;
	struct flag *f = arena_alloc(a, sizeof(*f), ALIGNOF(*f));
	f->region = current;
	f->addr = addr;
	f->prev = a->checkpoint;
	a->checkpoint = f;
}

void arena_restore(Arena *a){
	if(a->checkpoint == NULL){
		return;
	}
	a->current = a->checkpoint->region;
	a->current->avail = a->checkpoint->addr;
	a->checkpoint = a->checkpoint->prev;
}

#endif // SNORKEL_IMPLEMENTATION
