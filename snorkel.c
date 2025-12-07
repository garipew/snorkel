#include "snorkel.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>

////////////////////////////////////////////
///	Arenas
///////////////////////////////////////////

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

////////////////////////////////////////////
///	Strings
///////////////////////////////////////////

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
Arena _co_frame = {0};
struct _co_scheduler _co_sched_std = {0};
struct _co_scheduler *_co_sched = &_co_sched_std;

coroutine* (coroutine_create)(void* (*routine)(void*), void *arg, struct optsched optsched)
{
	_co_sched = optsched.sched;
	coroutine *new = arena_alloc(&_co_arena, sizeof(*new), ALIGNOF(*new));
	new->yield_point = (void*)routine;
	new->heap_frame = arena_alloc(&_co_frame, FRAME_SIZE, 16);
	new->rsp = new->heap_frame + FRAME_SIZE;
	new->rbp = new->rsp;
	new->arg = arg;
	if(!_co_sched->start){
		_co_sched->start = new;
		_co_sched->end = new;
		return new;
	}
	_co_sched->end->next = new;
	_co_sched->end = new;
	return new;
}

// NOTE(garipew): Naked are also never inlined.
__attribute__((naked, optimize("O0")))
void _co_restore_context()
{
	__asm__ volatile("pop %r8\n\t" // save ret addr
			/* Restore non-volatile from frame */
			"pop %r15\n\t"
			"pop %r14\n\t"
			"pop %r13\n\t"
			"pop %r12\n\t"
			"pop %rbx\n\t"
			"pop %rbx\n\t"
			"jmp *%r8\n\t"); // back to caller
}

__attribute__((naked, optimize("O0")))
void _co_load_context()
{
	__asm__ volatile("pop %r8\n\t" // save ret addr
			/* Save non-volatile to frame */
			"push %rbx\n\t"
			"push %rbx\n\t"
			"push %r12\n\t"
			"push %r13\n\t"
			"push %r14\n\t"
			"push %r15\n\t"
			"jmp *%r8\n\t"); // back to caller
}

__attribute__((naked, optimize("O0")))
void _co_swap_context(struct _co_scheduler *sched)
{
	(void) sched;
	__asm__ volatile("pop %r8\n\t" // save ret addr
			"mov 0x10(%rdi), %r9\n\t"
			"xchg 0x8(%r9), %rsp\n\t" // rsp <=> sched->running->rsp
			"xchg 0x10(%r9), %rbp\n\t" // rbp <=> sched->running->rbp
			"jmp *%r8\n\t"); // back to caller
}

__attribute__((naked, optimize("O0")))
void* _co_resume_yield(struct _co_scheduler *sched, void *yieldval)
{
	(void) sched;
	(void) yieldval;
	__asm__ volatile("pop %%r8\n\t" // save ret addr
			"mov 0x10(%%rdi), %%rdi\n\t" // sched.running value (ptr)
			"xchg %%r8, (%%rdi)\n\t" // sched.running.yield_point <=> ret
			"mov 0x20(%%rdi), %%r9\n\t"
			"add $%c0, %%r9\n\t" // r9 = sched.running.heap_frame + FRAME_SIZE
			"cmp %%rsp, %%r9\n\t"
			"jne 1f\n\t"
			"push (%%rdi)\n\t"
			"1:\n\t"
			"mov 0x28(%%rdi), %%r9\n\t"
			"test %%r9, %%r9\n\t" // sched.running.arg ?
			"jz 2f\n\t"
			"mov %%r9, %%rdi\n\t"
			"2:\n\t"
			"mov %%rsi, %%rax\n\t"
			"jmp *%%r8\n\t"
			: : "i"(FRAME_SIZE));  // back to yield_point pre swap
}

__attribute__((naked, optimize("O0")))
void* yield(void* yieldval)
{
	(void)yieldval;
	if(!_co_sched->running){
		fprintf(stderr,
			"ERROR: yield call when no coroutine is running\n");
		exit(1);
	}
	__asm__ volatile("push %rdi\n\t"
			"push %rdi\n\t");
	_co_load_context();
	__asm__ volatile("mov 0x30(%rsp), %rbx\n\t");
	_co_swap_context(_co_sched);
	__asm__ volatile("mov %%rbx, %%rsi\n\t"
			"mov _co_sched@GOTPCREL(%%rip), %%r9\n\t" // r9 = &_co_sched
			"mov (%%r9), %%r9\n\t"
			"mov 0x10(%%r9), %%r9\n\t"
			"mov 0x20(%%r9), %%r9\n\t"
			"add $%c0, %%r9\n\t"
			"cmp %%rsp, %%r9\n\t" //rsp == heap_frame + FRAME_SIZE ?
			"je 1f\n\t"
			"call _co_restore_context\n\t"
			"1:\n\t"
			"mov _co_sched@GOTPCREL(%%rip), %%rdi\n\t"
			"mov (%%rdi), %%rdi\n\t"
			"call _co_resume_yield\n\t"
			: : "i"(FRAME_SIZE));
	// TODO(garipew): Checking every yield if coroutine returned doesn't
	// seem elegant.
	__asm__ volatile("mov _co_sched@GOTPCREL(%%rip), %%rdi\n\t"
			"mov (%%rdi), %%rdi\n\t"
			"mov 0x10(%%rdi), %%r9\n\t"
			"mov 0x20(%%r9), %%r9\n\t"
			"add $%c0, %%r9\n\t"
			"cmp %%rsp, %%r9\n\t"
			"jne 1f\n\t"
			"mov %%rax, %%rbx\n\t"
			"call _co_swap_context\n\t"
			"mov %%rbx, %%rsi\n\t"
			"call _co_restore_context\n\t"
			"mov %%rsi, %%rax\n\t"
			"1:\n\t"
			"pop %%rdi\n\t"
			"pop %%rdi\n\t"
			"ret\n\t"
			: : "i"(FRAME_SIZE));
}

void* (coroutine_step)(coroutine *co, struct optsched optsched)
{
	if(!co){
		return NULL; // coroutine does not exist
	}
	coroutine *tmp;
	_co_sched = optsched.sched;
	_co_sched->running = co;
	for(tmp = _co_sched->start; tmp && tmp->next != co; tmp = tmp->next);
	if(co == _co_sched->end){
		_co_sched->end = tmp;
	}
	if(tmp){
		tmp->next = co->next;
	}else if(co == _co_sched->start){
		_co_sched->start = _co_sched->start->next;
	}else{
		_co_sched->running = NULL;
		return NULL; // coroutine isn't scheduled, possibly already returned
	}
	co->next = NULL;

	void *yieldval = yield(NULL);
	tmp = _co_sched->running;
	_co_sched->running = NULL;
	if(tmp->rsp == tmp->heap_frame + FRAME_SIZE){
		return yieldval; // coroutine returned
	}
	if(!_co_sched->start){
		_co_sched->start = tmp;
		_co_sched->end = tmp;
		return yieldval; // coroutine yielded, it is the only scheduled
	}
	_co_sched->end->next = tmp;
	_co_sched->end = tmp;
	return yieldval; // coroutine yielded
}

void _co_wake_next(struct optsched optsched)
{
	struct _co_scheduler *sched = optsched.sched;
	for( ; sched->start; ){
		coroutine_step(sched->start, .sched=optsched.sched);
	}
	arena_free(&_co_arena);
	arena_free(&_co_frame);
}
