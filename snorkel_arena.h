#ifndef SNORKEL_ARENA_H
#define SNORKEL_ARENA_H

#define REGION_SIZE (1ul<<28) // 256MB

#include <stdint.h>
#include <stddef.h>

typedef uint8_t u8;

typedef struct Region Region;
struct Region{
	Region *next;
	u8 *avail;
	u8 *limit;
	size_t commited;
	size_t size;
};

struct flag {
	Region *region;
	u8 *addr;
	struct flag *prev;
};

typedef struct {
	Region *start, *end;
	Region *current;
	struct flag *checkpoint;
	u8 align;
} Arena;

void* arena_grow(Arena*, size_t);
void* arena_alloc(Arena*, size_t);
void arena_free(Arena*);
void arena_reset(Arena*);
void arena_flag(Arena*);
void arena_restore(Arena*);
void arena_set_align(Arena*, u8);
#endif // SNORKEL_ARENA_H

#ifdef SNORKEL_IMPLEMENTATION
#include <sys/mman.h>
#include <unistd.h>

#define round_align(start, align) \
	(((start)+(align)-1) & ~((align)-1))

#define get_next_aligned(arena) \
	(void*)round_align((uintptr_t)arena->current->avail, arena->align)

void* arena_grow(Arena *arena, size_t at_least){
	size_t page_size = sysconf(_SC_PAGE_SIZE);

	size_t region_size =  REGION_SIZE;
	if(region_size < at_least) {
		region_size = round_align(at_least, page_size);
	}

	Region *new_region = mmap(NULL, region_size, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	if(new_region == MAP_FAILED){
		return NULL;
	}
	if(mprotect(new_region, round_align(sizeof(*new_region), page_size), PROT_READ|PROT_WRITE)) {
		munmap(new_region, REGION_SIZE);
		return NULL;
	}
	new_region->avail = (u8*)new_region + sizeof(*new_region);
	new_region->commited = round_align(sizeof(*new_region), page_size);
	new_region->limit = (u8*)new_region + new_region->commited;
	new_region->size = region_size;
	arena->current = new_region;
	arena->end = new_region;
	if(!arena->start){
		arena->start = new_region;
		return arena->current;
	}
	arena->end->next = new_region;
	return arena->current;
}

void* arena_commit(Arena *arena, size_t size) {
	if(!arena->start || !arena->end || !arena->current) {
		return NULL;
	}
	if(arena->current->commited + size > REGION_SIZE) {
		return NULL;
	}
	size_t page_size = sysconf(_SC_PAGE_SIZE);
	size_t new_commit = round_align(size, page_size);
	if(!mprotect(arena->current->limit, new_commit, PROT_READ|PROT_WRITE)) {
		arena->current->limit += new_commit;
		arena->current->commited += new_commit;
		return get_next_aligned(arena);
	}
	return NULL;
}

void arena_zero(u8 *dst, size_t len){
	u8 *ptr = dst;
	while(ptr < dst + len){
		*ptr = 0;
		ptr++;
	}
}

int find_space(Arena *arena, size_t size){
	Region *current = arena->current;
	for(; arena->current; arena->current = arena->current->next){
		size_t padding = (uintptr_t)get_next_aligned(arena) - (uintptr_t)arena->current->avail;
		uintptr_t next = (uintptr_t)arena->current->avail + padding + size;
		if(next < (uintptr_t)arena->current->limit) {
			arena_zero(get_next_aligned(arena), size);
			return 1;
		}
		if(arena->current->size - arena->current->commited < padding + size) {
			continue;
		}
		if(!arena_commit(arena, next - (uintptr_t)arena->current->limit)) {
			break;
		}
		return 1;
	}
	arena->current = current;
	return 0;
}

void* arena_alloc(Arena *arena, size_t size){
	if(size == 0){
		return NULL;
	}
	if(arena->align == 0){
		arena->align = 16;
	}

	if(find_space(arena, size) || (arena_grow(arena, size) && find_space(arena, size))) {
		void *new_ptr = get_next_aligned(arena);
		arena->current->avail = (void*)(size+(uintptr_t)new_ptr);
		return new_ptr;
	}
	return NULL;
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
		munmap(arena->end, arena->end->size);
	}
	arena_zero((void*)arena, sizeof(*arena));
}

void arena_flag(Arena *arena){
	if(arena->current == NULL){
		return;
	}
	Region *current = arena->current;
	u8 *addr = current->avail;
	struct flag *f = arena_alloc(arena, sizeof(*f));
	f->region = current;
	f->addr = addr;
	f->prev = arena->checkpoint;
	arena->checkpoint = f;
}

void arena_restore(Arena *arena){
	if(arena->checkpoint == NULL){
		return;
	}
	arena->current = arena->checkpoint->region;
	arena->current->avail = arena->checkpoint->addr;
	arena->checkpoint = arena->checkpoint->prev;
}

void arena_set_align(Arena *arena, u8 align) {
	arena->align = align;
}

#endif // SNORKEL_IMPLEMENTATION
