#ifndef SNORKEL_H
#define SNORKEL_H

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

typedef struct {
	size_t size;
	size_t len;
	char *bytes;
} string;

void* arena_grow(Arena*, size_t);
void* arena_alloc(Arena*, size_t, size_t);
void arena_free(Arena*);
void arena_reset(Arena*);
string* arena_create_string(Arena*, size_t);
string* string_concat(Arena*, string*, string*);
string* string_concat_bytes(Arena*, string*, char*, size_t);
int string_find(string*, size_t, char*, size_t);
void string_to_bytes(string*, char*, size_t, size_t);
string* string_ensure_terminator(Arena*, string*);
string* string_substr(Arena*, string*, int, int);

////////////////////////////////////////////
///	Coroutines
///////////////////////////////////////////

typedef struct coroutine coroutine;
struct coroutine {
	uintptr_t yield_point;
	uintptr_t start;
	uintptr_t end;
	uintptr_t return_point;
	coroutine *next;

	void (*signature)(void);

	size_t frame_size;
	u8 *heap_frame;
};

typedef struct {
	coroutine *start;
	coroutine *end;
	coroutine *running;
} scheduler;

extern scheduler _co_scheduler;
extern Arena _co_arena;
#define coroutine_start() \
	_start_internal(_co_scheduler.start)

// As long as c is the first argument of the function that calls load_frame,
// it should be fine...
#define load_registers(c)\
	__asm__("push %rax\n\t"\
                "push %rbx\n\t"\
		"mov %rdi, %rax\n\t" /* rdi is c but also, &c->yield_point*/ \
		"mov 0x8(%rbp), %rax\n\t" \
		"mov %rax, (%rdi)\n\t" /* c->yield_point == return addr */ \
		"mov %rdi, %rax\n\t" \
		"add $0x8, %rax\n\t" /* rax is &c->start */ \
		"mov %rbp, (%rax)\n\t" /* c->start = rbp+0x10 rbp and ret are pushed */ \
		"addb $0x10, (%rax)\n\t" \
		"mov %rdi, %rbx\n\t"\
		"add $0x10, %rbx\n\t" /* rbx is &c->end */ \
		"mov %rbp, %rax\n\t" \
		"mov (%rax), %rax\n\t" \
		"mov %rax, (%rbx)\n\t" /* c->end = rbp */ \
		"addb $0x10, (%rbx)\n\t" /* c->end = rbp + 0x10 to include the return address */\
		"pop %rbx\n\t"\
		"pop %rax\n\t")

#define yield \
	_yield_internal(_co_scheduler.running, _co_scheduler.start)

void coroutine_create(void (*)(void));
void _start_internal(coroutine *);
void _yield_internal(coroutine *, coroutine *);
#endif // SNORKEL_H
