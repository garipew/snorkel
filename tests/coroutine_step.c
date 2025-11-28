#include <snorkel.h>
#include <stdio.h>

void* co_step(){
	for(int i = 0; i < 10; i++){
		printf("%d\n", i);
		yield(NULL);
	}
	return NULL;
}

int main(){
	coroutine *a = coroutine_create(co_step, NULL);
	coroutine *b = coroutine_create(co_step, NULL);
	coroutine_step(a);
	coroutine_step(a);
	coroutine_step(a);
	coroutine_step(a);
	coroutine_step(b);
	return 0;
}

