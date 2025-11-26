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

#define FRAME_SIZE 1048576 /* 1MB */

typedef struct coroutine coroutine;
struct coroutine {
	u8 *yield_point;
	u8 *rsp;
	u8 *rbp;
	coroutine *next;

	u8 *heap_frame;
};

typedef struct {
	coroutine *start;
	coroutine *end;
	coroutine *running;
} scheduler;

extern scheduler _co_scheduler;
extern Arena _co_arena;
extern Arena _co_frame;

#define yield \
	_co_yield(__func__)

#define coroutine_start() \
	_co_scheduler_wake_next(&_co_scheduler)

void coroutine_create(void (*)(void));
void _co_yield(const char*);
void _co_restore_context();
void _co_load_context();
void _co_swap_context(scheduler*);
void _co_resume_yield(scheduler*);
void _co_scheduler_wake_next(scheduler*);
#endif // SNORKEL_H
