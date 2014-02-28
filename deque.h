
#ifndef __DEQUE_H__
#define __DEQUE_H__

typedef struct deque_node
{
	struct deque_node* nextp;
	struct deque_node* prevp;
	void*	elementp;
} DEQUE_NODE;

typedef struct deque_t
{
	int 		count;
	DEQUE_NODE*	headp;
} DEQUE_T;

typedef void (*RELEASE_FUNCTION) (void* elementp);

DEQUE_T* 	deque_init();
int 		deque_append(DEQUE_T* dequep, void* elementp);
void 		deque_release(DEQUE_T* dequep, RELEASE_FUNCTION release_it);
void*	 	deque_remove_head(DEQUE_T* dequep);

#endif

