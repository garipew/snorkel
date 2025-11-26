#include <snorkel.h>
#include <stdio.h>

void co_arg(void *arg){
	int n = (uintptr_t)arg;
	for(int i = 0; i < n; i++){
		printf("%d\n", i);
		yield;
	}
}

int main(){
	coroutine_create(co_arg, (void*)10);
	coroutine_create(co_arg, (void*)5);
	coroutine_start();
	return 0;
}
