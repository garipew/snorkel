#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <snorkel.h>

void co_example_even(){
	for(int i = 0; i < 10; i+=2){
		printf("%d\n", i);
		yield;
	}
}

void co_example_odd(){
	for(int i = 1; i < 11; i+=2){
		printf("%d\n", i);
		yield;
	}
}

void co_example_clap(){
	for(int i = 1; i < 10; i++){
		printf("*clap*\n");
		yield;
	}
}

int main(){
	coroutine_create(co_example_even);
	coroutine_create(co_example_clap);
	coroutine_create(co_example_odd);
	coroutine_create(co_example_clap);
	coroutine_start();
	return 0;
}
