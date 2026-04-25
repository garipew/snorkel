#ifndef SNORKEL_POOL_H
#define SNORKEL_POOL_H

#include <pthread.h>

typedef struct Task Task;
struct Task {
	unsigned short prio;
	void* (*chore)(void*);
	void* arg;
};

struct TODO {
	Task *tasks;
	size_t size;
	size_t len;
	pthread_mutex_t m;
};

struct Pool {
	pthread_t *workers;
	size_t worker_count;
	struct TODO todo;
};

enum tag {
	TASK,
	ERROR
};

typedef enum {
	OK,
	NO_MEM,
	EMPTY
} Error;

typedef struct {
	union content {
		Task task;
		Error error;
	} content;
	enum tag tag;
} Result;

int populate_pool(struct Pool*, void*, size_t);
int register_task(struct Pool*, unsigned short, void* (*)(void*), void *);
int resize_task_container(struct Pool*, void*, size_t);
void join_pool(struct Pool*);

#endif // SNORKEL_POOL_H

#ifdef SNORKEL_IMPLEMENTATION

#include <time.h>

#define swap(T, arr, i, j)               \
	do {		                 \
		T tmp;                   \
		tmp = (arr)[(i)];        \
		(arr)[(i)] = (arr)[(j)]; \
		(arr)[(j)] = tmp; 	 \
	} while(0)

void bubble_up(Task *heap, int idx) {
	while(idx > 0) {
		int parent = (idx-1)/2;
		if(heap[parent].prio >= heap[idx].prio) {
			break;
		}
		swap(Task, heap, idx, parent);
		idx = parent;
	}
}

void bubble_down(Task *heap, int idx, int len) {
	while(1) {
		int right = (idx + 1) * 2;
		int left = right - 1;
		int max = left;
		if(left >= len) {
			break;
		}
		if(right < len && heap[right].prio > heap[left].prio) {
			max = right;
		}
		if(heap[idx].prio >= heap[max].prio) {
			break;
		}
		swap(Task, heap, idx, max);
		idx = max;
	}
}

Result heap_push(struct TODO *todo, int prio, void* (*chore)(void*), void *arg) {
	pthread_mutex_lock(&todo->m);

	Result result = {0};
	if(todo->len >= todo->size) {
		result.tag = ERROR;
		result.content.error = NO_MEM;
		goto exit;
	}
	Task *t = &(todo->tasks[todo->len]);
	t->prio = prio;
	t->chore = chore;
	t->arg = arg;
	result.content.task = *t;

	bubble_up(todo->tasks, todo->len++);

exit:
	pthread_mutex_unlock(&todo->m);
	return result;
}

Result heap_pop(struct TODO *todo) {
	pthread_mutex_lock(&todo->m);

	Result result = {0};
	if(todo->len == 0) {
		result.tag = ERROR;
		result.content.error = EMPTY;
		goto exit;
	}
	result.content.task = todo->tasks[0];
	todo->tasks[0] = todo->tasks[--todo->len];

	bubble_down(todo->tasks, 0, todo->len);

exit:
	pthread_mutex_unlock(&todo->m);
	return result;
}

void* get_next_task(void *p) {
	struct Pool *pool = p;
	struct timespec time = {0};
	time.tv_nsec = 1000000;
	Result result;
	while(1) {
		result = heap_pop(&pool->todo);
		if(result.tag == ERROR) {
			nanosleep(&time, NULL);
			continue;
		}
		result.content.task.chore(result.content.task.arg);
	}
	return NULL;
}

void join_pool(struct Pool *pool) {
	for(int i = 0; i < pool->worker_count; i++) {
		pthread_join(pool->workers[i], NULL);
	}
}

// TODO(garipew): Break this into two functions:
// 	- resize_pool
// 	- run_workers
int populate_pool(struct Pool *pool, void *mem, size_t bytes) {
	if(pool == NULL) {
		return -1;
	}
	pthread_mutex_init(&pool->todo.m, NULL);
	int worker_count = bytes / sizeof(*pool->workers);
	for(int i = 0; i < worker_count; i++) {
		pthread_t *w = &((pthread_t*)mem)[i];
		pthread_create(w, NULL, get_next_task, (void*)pool);
	}
	pool->workers = mem;
	pool->worker_count = worker_count;
	return worker_count;
}

int register_task(struct Pool *p, unsigned short prio, void* (*chore)(void*), void *arg) {
	if(p == NULL) {
		return -1;
	}
	Result result = heap_push(&p->todo, prio, chore, arg);
	if(result.tag == ERROR) {
		return result.content.error;
	}
	return 0;
}

int resize_task_container(struct Pool *p, void *mem, size_t bytes) {
	int retval = 0;
	Task *new_buff = mem;
	size_t new_len = bytes / sizeof(Task);
	size_t idx = 0;
	if(!p->todo.tasks) {
		retval = -1;
		goto assign;
	}
	while(idx < new_len) {
		if(idx >= p->todo.len) {
			break;
		}
		new_buff[idx] = p->todo.tasks[idx];
	}
assign:
	p->todo.tasks = new_buff;
	p->todo.size = new_len;
	p->todo.len = idx;
	return retval;
}

#endif // SNORKEL_IMPLEMENTATION
