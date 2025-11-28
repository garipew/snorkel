#include <snorkel.h>
#include <stdio.h>

void* yield_val(){
	yield((void*)0xdeadbeef);
	yield((void*)0xcafebabe);
	return NULL;
}

int main(){
	coroutine *a = coroutine_create(yield_val, NULL);
	void *yieldval = coroutine_step(a);
	printf("%p\n", yieldval);
	yieldval = coroutine_step(a);
	printf("%p\n", yieldval);
}
