#ifndef SNORKEL_H
#define SNORKEL_H

#define _GNU_SOURCE
#define REGION_SIZE 32768 // PAGE_SIZE * 8

#ifdef __GNUC__
#define ALIGNOF __alignof__
#else
#error "Compiler support not available yet"
#endif

#include <stdint.h>
#include <stddef.h>

typedef uint8_t u8;

////////////////////////////////////////////
///	Arenas
///////////////////////////////////////////

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
	u8 fixed_size;
} Arena;

void* arena_grow(Arena*, size_t);
void* arena_alloc(Arena*, size_t, size_t);
void arena_free(Arena*);
void arena_reset(Arena*);

////////////////////////////////////////////
///	Strings
///////////////////////////////////////////

typedef struct {
	size_t size;
	size_t len;
	char *bytes;
} string;

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

// NOTE(garipew): Threads spawned inside coroutine shall not outlive yield

typedef struct coroutine coroutine;
struct coroutine {
	u8 *yield_point;
	u8 *start;
	u8 *end;
	coroutine *next;

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

#define yield \
	__asm__("push %rbx\n\t" \
		"push %rbx\n\t" \
		"push %r12\n\t" \
		"push %r13\n\t" \
		"push %r14\n\t" \
		"push %r15\n\t"); \
	_load_context(&_co_scheduler); \
	get_yield_point(&_co_scheduler); \
	__asm__("leave\n\t" \
		"ret\n\t");

#define coroutine_start() \
	scheduler_wake_next(&_co_scheduler)

void coroutine_create(void (*)(void));
void _restore_context(scheduler*);
void _load_context(scheduler*);
void get_yield_point(scheduler*);
void scheduler_wake_next(scheduler*);
#endif // SNORKEL_H
