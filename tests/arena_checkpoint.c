#define SNORKEL_IMPLEMENTATION
#include "../snorkel_arena.h"
#include <stdint.h>

Arena g = {0};

int main(){
	uintptr_t *ptr = arena_alloc(&g, sizeof(*ptr));
	*ptr = (uintptr_t)g.current->avail;
	arena_flag(&g);
	if(g.checkpoint->addr == (void*)*ptr){
		printf("OK\n");
	}
	(void) arena_alloc(&g, sizeof(*ptr));
	(void) arena_alloc(&g, sizeof(*ptr));
	(void) arena_alloc(&g, sizeof(*ptr));
	(void) arena_alloc(&g, sizeof(*ptr));
	arena_restore(&g);
	if(g.current->avail == (void*)*ptr){
		printf("OK\n");
	}
	return 0;
}
