#define SNORKEL_IMPLEMENTATION
#include "../snorkel_co.h"
#include <stdio.h>
#include <stdlib.h>

void* co_larger(){
	for(int i = 0; i < 10; i++){
		printf("%d\n", i);
	}
	return NULL;
}

void* co_smaller(){
	for(int i = 0; i < 5; i++){
		printf("%d\n", i);
	}
	return NULL;
}

int main(){
	set_alloc(malloc);
	set_free(free);
	coroutine_create(co_larger, NULL);
	coroutine_create(co_smaller, NULL);
	coroutine_start();
	return 0;
}
