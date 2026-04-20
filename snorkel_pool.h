#ifndef SNORKEL_POOL_H
#define SNORKEL_POOL_H

#include <pthread.h>

typedef struct Worker Worker;
struct Worker {
	pthread_t thread;
	Worker *prev;
};

typedef struct Task Task;
struct Task {
	unsigned short priority;
	void* (*chore)(void*);
	void* arg;

	Task *left;
	Task *right;
	Task *parent;
};

struct Pool {
	Worker *workers;
	Task *todo;
};

int populate_pool(struct Pool*, void*, size_t);
int register_task(struct Pool*, unsigned short, void* (*)(void*), void*, size_t);

#endif // SNORKEL_POOL_H

#ifdef SNORKEL_IMPLEMENTATION

// TODO(garipew): All of these should lock:
// 	- Heapfy, insert and remove on todo
// 	- push and pop on workers

// TODO(garipew): get_task to assign free Worker to Task

// TODO(garipew): populate_pool and register_task

#endif // SNORKEL_IMPLEMENTATION
