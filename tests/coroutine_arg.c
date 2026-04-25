#define SNORKEL_IMPLEMENTATION
#include "../snorkel_co.h"
#include <stdio.h>
#include <stdlib.h>

void* co_arg(void *arg){
	int n = (uintptr_t)arg;
	for(int i = 0; i < n; i++){
		printf("%d\n", i);
		yield(NULL);
	}
	return NULL;
}

int main(){
	set_alloc(malloc);
	set_free(free);
	coroutine_create(co_arg, (void*)10);
	coroutine_create(co_arg, (void*)5);
	coroutine_start();
	return 0;
}
