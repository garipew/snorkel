#define SNORKEL_IMPLEMENTATION
#include "../snorkel_co.h"
#include <stdio.h>
#include <stdlib.h>

void* yield_val(){
	yield((void*)0xdeadbeef);
	yield((void*)0xcafebabe);
	return NULL;
}

int main(){
	set_alloc(malloc);
	set_free(free);
	coroutine *a = coroutine_create(yield_val, NULL);
	void *yieldval = coroutine_step(a);
	printf("%p\n", yieldval);
	yieldval = coroutine_step(a);
	printf("%p\n", yieldval);
}
