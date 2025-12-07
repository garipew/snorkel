#include <snorkel.h>
#include <stdio.h>

struct _co_scheduler my_scheduler = {0};

void* co_arg(void *arg){
	int n = (uintptr_t)arg;
	for(int i = 0; i < n; i++){
		printf("%d\n", i);
		yield(NULL);
	}
	return NULL;
}

int main(){
	coroutine_create(co_arg, (void*)10, .sched=&my_scheduler);
	coroutine_create(co_arg, (void*)5, .sched=&my_scheduler);
	coroutine_start(.sched=&my_scheduler);
	return 0;
}
