#include <snorkel.h>
#include <stdio.h>

struct _co_scheduler my_scheduler = {0};

void* co_arg(){
	if(get_scheduler() == &my_scheduler){
		printf("OK!\n");
	}
	return NULL;
}

int main(){
	coroutine_create(co_arg, NULL, .sched=&my_scheduler);
	coroutine_start(.sched=&my_scheduler);
	return 0;
}
