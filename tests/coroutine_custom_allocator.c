#define SNORKEL_IMPLEMENTATION
#include "../snorkel_co.h"
#include "../snorkel_arena.h"
#include <stdio.h>

Arena a = {0};

void* my_alloc(size_t n) {
	return arena_alloc(&a, n);
}

void my_free(void* ptr) {
	(void)ptr;
	return;
}

void* co_arg(void *arg){
	int n = (uintptr_t)arg;
	for(int i = 0; i < n; i++){
		printf("%d\n", i);
		yield(NULL);
	}
	return NULL;
}

int main() {
	set_alloc(my_alloc);
	set_free(my_free);
	coroutine_create(co_arg, (void*)10);
	coroutine_create(co_arg, (void*)5);
	coroutine_start();
	arena_free(&a);
	return 0;
}
