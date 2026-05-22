#ifndef SNORKEL_POOL_H
#define SNORKEL_POOL_H

#include <pthread.h>

typedef struct {
	unsigned short prio;
	void* (*chore)(void*);
	void* arg;
} Task;

typedef struct {
	pthread_mutex_t m;
	pthread_cond_t task_available;
	pthread_cond_t tasks_complete;

	pthread_t *workers;
	size_t working;
	size_t worker_count;

	Task *tasks;
	size_t task_count;
	size_t task_size;

	unsigned short keepalive;
} Pool;

struct snorkel_pool_allocator {
	void* (*alloc)(size_t);
	void* (*realloc)(void*, size_t);
	void (*free)(void*);
};

extern struct snorkel_pool_allocator allocator;
void snorkel_pool_inject_allocators(void* (*)(size_t), void* (*)(void*, size_t), void (*)(void*));

// Create a new Pool
Pool* create_pool(size_t, size_t);

// Add new task to Pool
int register_task(Pool*, unsigned short, void* (*)(void*), void*);

// Wait all tasks complete on Pool
void wait_pool(Pool*);

// Send finish message to workers
void kill_pool(Pool*);

// Free resources
void free_resources(Pool*);

#endif // SNORKEL_POOL_H

#ifdef SNORKEL_IMPLEMENTATION

struct snorkel_pool_allocator allocator;

#define swap(T, arr, i, j)               \
	do {                             \
		T tmp;                   \
		tmp = (arr)[(i)];        \
		(arr)[(i)] = (arr)[(j)]; \
		(arr)[(j)] = tmp;        \
	} while(0)

int resize_task_buffer(Pool *pool) {
	pool->task_size *= 2;
	void *new_buff = allocator.realloc(pool->tasks, sizeof(*pool->tasks) * pool->task_size);
	if(!new_buff) {
		pool->task_size /= 2;
		return 1;
	}
	pool->tasks = new_buff;
	return 0;
}

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

Task* heap_push(Pool *pool, int prio, void* (*chore)(void*), void *arg) {
	Task *t = NULL;
	if(pool->task_count >= pool->task_size && resize_task_buffer(pool)) {
		return t;
	}

	t = &(pool->tasks[pool->task_count]);
	t->prio = prio;
	t->chore = chore;
	t->arg = arg;

	bubble_up(pool->tasks, pool->task_count++);

	return t;
}

Task heap_pop(Pool *pool) {
	Task result = {0};
	if(pool->task_count == 0) {
		return result;
	}
	result = pool->tasks[0];
	pool->tasks[0] = pool->tasks[--pool->task_count];

	bubble_down(pool->tasks, 0, pool->task_count);

	return result;
}

void* get_next_task(void *arg) {
	Pool *pool = arg;
	Task result;
	while(1) {
		pthread_mutex_lock(&pool->m);

		while(pool->keepalive && pool->task_count <= 0) {
			pthread_cond_wait(&pool->task_available, &pool->m);
		}
		if(!pool->keepalive) {
			break;
		}
		result = heap_pop(pool);
		pool->working++;

		pthread_mutex_unlock(&pool->m);

		if(result.chore) {
			result.chore(result.arg);
		}

		pthread_mutex_lock(&pool->m);
		pool->working--;
		if(pool->working == 0 && pool->task_count == 0) {
			pthread_cond_signal(&pool->tasks_complete);
		}
		pthread_mutex_unlock(&pool->m);
	}

	pthread_mutex_unlock(&pool->m);
	return NULL;
}

void snorkel_pool_inject_allocators(void* (*alloc)(size_t), void* (*realloc)(void*, size_t), void (*free)(void*)) {
	allocator.alloc = alloc;
	allocator.realloc = realloc;
	allocator.free = free;
}

Pool* create_pool(size_t workers, size_t tasks) {
	Pool *p = NULL;
	if(!allocator.alloc || !(p = allocator.alloc(sizeof(*p)))) {
		goto exit;
	}

	pthread_mutex_init(&p->m, NULL);
	pthread_cond_init(&p->task_available, NULL);
	pthread_cond_init(&p->tasks_complete, NULL);

	p->keepalive = 1;

	p->working = 0;
	p->worker_count = workers;
	if(p->worker_count == 0) {
		p->worker_count++;
	}
	p->workers = allocator.alloc(sizeof(*p->workers) * p->worker_count);
	if(!p->workers) {
		goto clean_pool;
	}

	p->task_size = tasks;
	if(p->task_size == 0) {
		p->task_size++;
	}
	p->tasks = allocator.alloc(sizeof(*p->tasks) * p->task_size);
	if(!p->tasks) {
		goto clean_workers;
	}

	for(int i = 0; i < p->worker_count; i++) {
		pthread_create(&p->workers[i], NULL, get_next_task, p);
	}
	goto exit;

clean_workers:
	allocator.free(p->workers);
	p->workers = NULL;
clean_pool:
	allocator.free(p);
	p = NULL;
exit:
	return p;
}

int register_task(Pool *p, unsigned short prio, void* (*chore)(void*), void *arg) {
	if(p == NULL) {
		return -1;
	}

	pthread_mutex_lock(&p->m);

	int retval = -2;
	Task *result = heap_push(p, prio, chore, arg);
	if(result) {
		pthread_cond_signal(&p->task_available);
		retval = 0;
	}

	pthread_mutex_unlock(&p->m);
	return retval;
}

void wait_pool(Pool *p) {
	if(!p) {
		return;
	}

	pthread_mutex_lock(&p->m);
	while(p->working > 0 || p->task_count > 0) {
		pthread_cond_wait(&p->tasks_complete, &p->m);
	}
	pthread_mutex_unlock(&p->m);
}

void kill_pool(Pool *p) {
	if(!p) {
		return;
	}
	pthread_mutex_lock(&p->m);

	p->keepalive = 0;
	p->task_count = 0;
	pthread_cond_broadcast(&p->task_available);

	pthread_mutex_unlock(&p->m);
}

void free_resources(Pool *p) {
	pthread_mutex_lock(&p->m);
	size_t worker_count = p->worker_count;
	pthread_mutex_unlock(&p->m);

	kill_pool(p);
	for(int i = 0; i < worker_count; i++) {
		pthread_join(p->workers[i], NULL);
	}

	pthread_mutex_destroy(&p->m);
	pthread_cond_destroy(&p->task_available);
	pthread_cond_destroy(&p->tasks_complete);

	allocator.free(p->workers);
	allocator.free(p->tasks);
	allocator.free(p);
}

#endif // SNORKEL_IMPLEMENTATION
