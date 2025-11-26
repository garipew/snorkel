#include <snorkel.h>
#include <stdio.h>

void function(){
	yield;
	printf("1\n");
}

int main(){
	function();
	printf("2\n");
	return 0;
}
