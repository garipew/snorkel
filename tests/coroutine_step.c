#define SNORKEL_IMPLEMENTATION
#include "../snorkel_co.h"
#include <stdio.h>
#include <stdlib.h>

void* co_step(){
	for(int i = 0; i < 10; i++){
		printf("%d\n", i);
		yield(NULL);
	}
	return NULL;
}

int main(){
	set_alloc(malloc);
	set_free(free);
	coroutine *a = coroutine_create(co_step, NULL);
	coroutine *b = coroutine_create(co_step, NULL);
	coroutine_step(a);
	coroutine_step(a);
	coroutine_step(a);
	coroutine_step(a);
	coroutine_step(b);
	return 0;
}

