#include <snorkel.h>
#include <stdio.h>

void co_larger(){
	for(int i = 0; i < 10; i++){
		printf("%d\n", i);
		yield;
	}
}

void co_smaller(){
	for(int i = 0; i < 5; i++){
		printf("%d\n", i);
		yield;
	}
}

int main(){
	coroutine_create(co_larger);
	coroutine_create(co_smaller);
	coroutine_start();
	return 0;
}
