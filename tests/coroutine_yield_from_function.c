#include <snorkel.h>
#include <stdio.h>

void* function(){
	yield(NULL);
	printf("1\n");
	return NULL;
}

int main(){
	function();
	printf("2\n");
	return 0;
}
