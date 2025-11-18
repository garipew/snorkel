#include "snorkel.h"
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
		memset(arena->current->avail, 0, arena->current->limit-arena->current->avail);
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

string* arena_create_string(Arena *arena, size_t size){
	string *str = arena_alloc(arena, sizeof(*str)+size, ALIGNOF(*str));
	str->size = size;
	str->bytes = (char*)str+round_align(sizeof(*str), ALIGNOF(char));
	return str;
}

string* arena_expand_string(Arena *arena, string* str, size_t new_size){
	string *expanded = arena_create_string(arena, new_size);
	if(str){
		memcpy(expanded->bytes, str->bytes, str->size);
		expanded->len = str->len;
	}
	return expanded;
}

string* string_concat(Arena *arena, string *a, string *b){
	if(!a && !b){
		return NULL;
	}
	if(!a && b){
		return b;
	}
	if(a && !b){
		return a;
	}
	string *c = arena_create_string(arena, a->len+b->len);
	int len = a->len;
	memcpy(c->bytes, a->bytes, len);
	memcpy(c->bytes+len, b->bytes, b->len);
	c->len = len+b->len;
	c = string_ensure_terminator(arena, c);
	return c;
}

string* string_concat_bytes(Arena* arena, string* str, char *raw, size_t size){
	if(!str){
		str = arena_expand_string(arena, str, size);
	}else if(str->len+size > str->size){
		str = arena_expand_string(arena, str, str->len+size);
	}
	// concat stops at first \0, memcpy can't be used since size is not reliable
	for(size_t copied = 0; copied < size ; copied++){
		if(!raw[copied]){
			break;
		}
		str->bytes[str->len++] = raw[copied];	
	}
	str = string_ensure_terminator(arena, str);
	return str;
}

void string_to_bytes(string *str, char *bytes, size_t start, size_t byte_count){
	for(size_t copied = 0; copied < byte_count; copied++){
		bytes[copied] = str->bytes[start+copied];
	}
}

int string_find(string *line, size_t start_index, char *bytes, size_t len){
	if(!line || start_index >= line->len || !bytes || *bytes == 0){
		return -1;
	}
	char *current = bytes;
	size_t at;
	for(at = start_index; at < line->len; at++){
		if(current == bytes+len){
			return at-len;
		}
		if(line->bytes[at] != *current){
			current = bytes;
		}
		if(line->bytes[at] == *current){
			current++;
		}
	}
	if(current == bytes+len){
		return at-len;
	}
	return -1;
}

string* string_ensure_terminator(Arena *arena, string *str){
	if(str->bytes[str->len-1] == 0){
		return str;
	}
	if(str->len < str->size){
		str->bytes[str->len] = 0;
		return str;
	}
	str = arena_expand_string(arena, str, str->size+1);
	str->bytes[str->len] = 0;
	return str;
}

string* string_substr(Arena *a, string *str, int start, int end){
	if(start < 0){
		start += str->len-1;
	}
	if(!str || start < 0 || (unsigned)start >= str->len){
		return NULL;
	}
	if(end < 0){
		end = str->len;
	}
	if(end < start){
		return NULL;
	}
	string *sub = arena_create_string(a, end-start);
	memcpy(sub->bytes, str->bytes+start, sub->size);
	sub->len = sub->size;
	sub = string_ensure_terminator(a, sub);
	return sub;
}

////////////////////////////////////////////
///	Coroutines
///////////////////////////////////////////

Arena _co_arena = {0};
scheduler _co_scheduler = {0};

void coroutine_create(void (*routine)(void)){
	coroutine *new = arena_alloc(&_co_arena, sizeof(*new), ALIGNOF(*new));
	new->signature = routine;
	if(!_co_scheduler.start){
		_co_scheduler.start = new;
		_co_scheduler.end = new;
		return;
	}
	_co_scheduler.end->next = new;
	_co_scheduler.end = new;
}

// restore stack frame and jumps to yield point
void _start_internal(coroutine *c){
	if(!c){
		return;
	}
	_co_scheduler.start = c->next;
	c->next = NULL;
	_co_scheduler.running = c;
	if(c->heap_frame){
		__asm__("push %rdi\n\t");
		memcpy((void*)c->start, c->heap_frame, c->frame_size);
		__asm__("pop %rdi\n\t"
			"leave\n\t"
			"mov %rdi, %rsp\n\t"
			"add $0x8, %rsp\n\t"
			"mov (%rsp), %rsp\n\t" // rsp = c->start
			"mov %rdi, %rbp\n\t"
			"add $0x10, %rbp\n\t"
			"mov (%rbp), %rbp\n\t" // rbp = c->end
			"sub $0x10, %rbp\n\t" // ignore the extra space reserved for the return address
			"jmp *(%rdi)\n\t"); // jmp c->yield_point
	}
	__asm__("leave\n\t"
		"jmp *0x28(%rdi)\n\t"); //jmp c->signature;
}

// load stack frame and jumps to the next coroutine
void _yield_internal(coroutine *c, coroutine *next){
	if(!c->start || !c->end){
		load_registers(c);
	}
	__asm__("push %rsi\n\t");
	if(!c->heap_frame){
		c->frame_size = c->end-c->start;
		c->heap_frame = arena_alloc(&_co_arena, c->frame_size, 1);
	}
	memcpy(c->heap_frame, (void*)c->start, c->frame_size);

	if(!next){
		next = c;
	}
	_co_scheduler.end->next = c;
	_co_scheduler.end = c;
	if(!next->return_point){
		next->return_point = (uintptr_t)_start_internal;
	}
	__asm__("pop %rdi\n\t"
		"leave\n\t"
		"jmp *0x18(%rdi)\n\t"); // jump to c->return_point
}
