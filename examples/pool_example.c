#define SNORKEL_IMPLEMENTATION
#include "../snorkel_pool.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#define WORKERS_COUNT 8
#define TASKS_HINT 20

void* foo(void *arg) {
	sleep(1);
	printf("Task number %u\n", (uintptr_t)arg);
	return arg;
}

int main() {
	// Before utilizing any of the library functions,
	// an allocator should be injected with
	snorkel_pool_inject_allocators(malloc, realloc, free);

	// To create a pool pass the number of workers the
	// pool should have and a hint on how many tasks
	// should the pool hold at the same time
	Pool *p = create_pool(WORKERS_COUNT, TASKS_HINT);

	// Next, register the tasks, notice how it is possible
	// to register more tasks than TASKS_HINT
	for(uintptr_t i = 0; i < 2*TASKS_HINT; i++) {
		// Register task foo with priority 100 - i
		// on pool p and pass it i as argument
		register_task(p, 100 - i, foo, (void*)i);
	}

	// Wait until there's no task left on pool p
	wait_pool(p);

	// Send a message to all workers on pool p to
	// finish execution
	kill_pool(p);
	
	// Free resources allocated by pool p
	free_resources(p);
}
