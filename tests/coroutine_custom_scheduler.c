#define SNORKEL_IMPLEMENTATION
#include "../snorkel_co.h"
#include <stdio.h>
#include <stdlib.h>

struct snorkel_scheduler my_scheduler = {0};

void* co_arg(){
	if(get_scheduler() == &my_scheduler){
		printf("OK!\n");
	}
	return NULL;
}

int main(){
	set_alloc(malloc, .sched=&my_scheduler);
	set_free(free, .sched=&my_scheduler);
	coroutine_create(co_arg, NULL, .sched=&my_scheduler);
	coroutine_start(.sched=&my_scheduler);
	return 0;
}
