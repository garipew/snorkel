#ifndef SNORKEL_CO_H
#define SNORKEL_CO_H

#include <stddef.h>
#include <stdint.h>

typedef uint8_t u8;
#define FRAME_SIZE 1048576 /* 1MB */

typedef struct coroutine coroutine;
struct coroutine {
	u8 *yield_point;
	u8 *rsp;
	u8 *rbp;
	coroutine *next;

	u8 *heap_frame;
	void *arg;
};

struct snorkel_scheduler{
	coroutine *start;
	coroutine *end;
	coroutine *running;
	struct {
		void* (*alloc)(size_t);
		void (*free)(void*);
	} allocator;
};

struct optargs{
	struct snorkel_scheduler *sched;
};

extern struct snorkel_scheduler snorkel_sched_std;

#define coroutine_start(...) \
	coroutine_start((struct optargs){.sched=&snorkel_sched_std, __VA_ARGS__})

#define coroutine_step(co, ...) \
	coroutine_step(co, (struct optargs){.sched=&snorkel_sched_std,__VA_ARGS__})

#define coroutine_create(r, a, ...) \
	coroutine_create(r, a, (struct optargs){.sched=&snorkel_sched_std, __VA_ARGS__})

#define coroutine_collect(...) \
	coroutine_collect((struct optargs){__VA_ARGS__})

#define set_alloc(fn, ...) \
	set_alloc(fn, (struct optargs){.sched=&snorkel_sched_std, __VA_ARGS__})

#define set_free(fn, ...) \
	set_free(fn, (struct optargs){.sched=&snorkel_sched_std, __VA_ARGS__})

void (set_alloc)(void* (*)(size_t), struct optargs);
void (set_free)(void (*)(void*), struct optargs);

coroutine* (coroutine_create)(void* (*)(void*), void*, struct optargs);
void* yield(void*);
void* (coroutine_step)(coroutine*, struct optargs);
void (coroutine_start)(struct optargs);
void (coroutine_collect)(struct optargs);

#ifdef SNORKEL_TEST
void* get_scheduler();
#endif // SNORKEL_TEST
#endif // SNORKEL_CO_H

#ifdef SNORKEL_IMPLEMENTATION

#include <stdlib.h>
#include <stdio.h>

struct snorkel_scheduler snorkel_sched_std;
static __thread struct snorkel_scheduler *snorkel_sched;

void (set_alloc)(void* (*fn)(size_t), struct optargs optargs) {
	optargs.sched->allocator.alloc = fn;
}

void (set_free)(void (*fn)(void*), struct optargs optargs) {
	optargs.sched->allocator.free = fn;
}

coroutine* (coroutine_create)(void* (*routine)(void*), void *arg, struct optargs optargs)
{
	snorkel_sched = optargs.sched;
	void *frame = optargs.sched->allocator.alloc(FRAME_SIZE);
	coroutine *new = optargs.sched->allocator.alloc(sizeof(*new));
	new->yield_point = (void*)routine;
	new->heap_frame = frame;
	new->rsp = new->heap_frame + FRAME_SIZE;
	new->rbp = new->rsp;
	new->arg = arg;
	if(!snorkel_sched->start){
		snorkel_sched->start = new;
		snorkel_sched->end = new;
		return new;
	}
	snorkel_sched->end->next = new;
	snorkel_sched->end = new;
	return new;
}

// NOTE(garipew): Naked are also never inlined.
__attribute__((naked, optimize("O0")))
void snorkel_restore_context()
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
void snorkel_load_context()
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
void snorkel_swap_context(struct snorkel_scheduler *sched)
{
	(void) sched;
	__asm__ volatile("pop %r8\n\t" // save ret addr
			"mov 0x10(%rdi), %r9\n\t"
			"xchg 0x8(%r9), %rsp\n\t" // rsp <=> sched->running->rsp
			"xchg 0x10(%r9), %rbp\n\t" // rbp <=> sched->running->rbp
			"jmp *%r8\n\t"); // back to caller
}

__attribute__((naked, optimize("O0")))
void* snorkel_resume_yield(struct snorkel_scheduler *sched, void *yieldval)
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
	__asm__ volatile("push %rdi\n\t"
			"push %rdi\n\t");
	if(!snorkel_sched || !snorkel_sched->running){
		fprintf(stderr,
			"ERROR: yield call when no coroutine is running\n");
		exit(1);
	}
	snorkel_load_context();
	__asm__ volatile("mov 0x30(%rsp), %rbx\n\t");
	snorkel_swap_context(snorkel_sched);
	__asm__ volatile("mov %%rbx, %%rsi\n\t"
			"mov %1, %%r9\n\t"
			"mov 0x10(%%r9), %%r9\n\t"
			"mov 0x20(%%r9), %%r9\n\t"
			"add $%c0, %%r9\n\t"
			"cmp %%rsp, %%r9\n\t" //rsp == heap_frame + FRAME_SIZE ?
			"je 1f\n\t"
			"call snorkel_restore_context\n\t"
			"1:\n\t"
			"mov %1, %%rdi\n\t"
			"call snorkel_resume_yield\n\t"
			: : "i"(FRAME_SIZE), "r"(snorkel_sched));
	__asm__ volatile("push %rax\n\t"
			"push %rax\n\t");
	__asm__ volatile("mov %1, %%rdi\n\t"
			"mov 0x10(%%rdi), %%r9\n\t"
			"mov 0x20(%%r9), %%r9\n\t"
			"add $%c0, %%r9\n\t"
			"pop %%rax\n\t"
			"pop %%rax\n\t"
			"cmp %%rsp, %%r9\n\t"
			"jne 1f\n\t"
			"mov %%rax, %%rbx\n\t"
			"call snorkel_swap_context\n\t"
			"mov %%rbx, %%rsi\n\t"
			"call snorkel_restore_context\n\t"
			"mov %%rsi, %%rax\n\t"
			"1:\n\t"
			"pop %%rdi\n\t"
			"pop %%rdi\n\t"
			"ret\n\t"
			: : "i"(FRAME_SIZE), "r"(snorkel_sched) : "rax");
}

void* (coroutine_step)(coroutine *co, struct optargs optargs)
{
	if(!co){
		return NULL; // coroutine does not exist
	}
	coroutine *tmp;
	snorkel_sched = optargs.sched;
	snorkel_sched->running = co;
	for(tmp = snorkel_sched->start; tmp && tmp->next != co; tmp = tmp->next);
	if(co == snorkel_sched->end){
		snorkel_sched->end = tmp;
	}
	if(tmp){
		tmp->next = co->next;
	}else if(co == snorkel_sched->start){
		snorkel_sched->start = snorkel_sched->start->next;
	}else{
		snorkel_sched->running = NULL;
		return NULL; // coroutine isn't scheduled, possibly already returned
	}
	co->next = NULL;

	void *yieldval = yield(NULL);
	tmp = snorkel_sched->running;
	snorkel_sched->running = NULL;
	if(tmp->rsp == tmp->heap_frame + FRAME_SIZE){
		return yieldval; // coroutine returned
	}
	if(!snorkel_sched->start){
		snorkel_sched->start = tmp;
		snorkel_sched->end = tmp;
		return yieldval; // coroutine yielded, it is the only scheduled
	}
	snorkel_sched->end->next = tmp;
	snorkel_sched->end = tmp;
	return yieldval; // coroutine yielded
}

void (coroutine_start)(struct optargs optargs)
{
	snorkel_sched = optargs.sched;
	for( ; snorkel_sched->start; ){
		coroutine_step(snorkel_sched->start, .sched=snorkel_sched);
	}
}

void (coroutine_collect)(struct optargs optargs)
{
	for(coroutine *next, *i = optargs.sched->start; i != NULL; ) {
		next = i->next;
		optargs.sched->allocator.free(i);
		i = next;
	}
}

#ifdef SNORKEL_TEST
void* get_scheduler()
{
	return (void*)snorkel_sched;
}
#endif // SNORKEL_TEST

#endif // SNORKEL_IMPLEMENTATION
