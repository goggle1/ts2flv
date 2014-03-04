
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

#include "deque.h"

int	 	deque_init(DEQUE_T* dequep)
{
	dequep->headp = NULL;
	dequep->count = 0;

	return 0;
}

int 		deque_append(DEQUE_T* dequep, void* elementp)
{
	if(dequep == NULL)
	{
		return -1;
	}
	
	DEQUE_NODE* nodep = (DEQUE_NODE*)malloc(sizeof(DEQUE_NODE));
	if(nodep == NULL)
	{
		return -1;
	}
	nodep->nextp = NULL;
	nodep->prevp = NULL;
	nodep->elementp = elementp;

	if(dequep->headp == NULL)
	{
		nodep->nextp = nodep;
		nodep->prevp = nodep;
		dequep->headp = nodep;
		dequep->count ++;
		return 0;
	}

	DEQUE_NODE* tailp = dequep->headp->prevp;
	tailp->nextp = nodep;
	nodep->prevp = tailp;
	nodep->nextp = dequep->headp;
	dequep->headp->prevp = nodep;
	dequep->count ++;
	return 0;
}

void 		deque_release(DEQUE_T* dequep, RELEASE_FUNCTION release_it)
{
	if(dequep == NULL)
	{
		return;
	}

	DEQUE_NODE* nodep = dequep->headp;
	while(nodep != NULL)
	{
		DEQUE_NODE* tempp = nodep->nextp;
	
		release_it(nodep->elementp);		
		free(nodep);

		if(tempp == dequep->headp)
		{
			break;
		}
		nodep = tempp;
	}
	
	dequep->headp = NULL;
	dequep->count = 0;
}


void* 	deque_remove_head(DEQUE_T* dequep)
{
	if(dequep == NULL)
	{
		return NULL;
	}

	if(dequep->headp == NULL)
	{
		return NULL;
	}
	
	if(dequep->count == 1)
	{
		DEQUE_NODE* tempp = dequep->headp;
		dequep->count = 0;
		dequep->headp = NULL;
		
		void* elementp = tempp->elementp;
		free(tempp);
		
		return elementp;
	}
	
	DEQUE_NODE* headp = dequep->headp;
	DEQUE_NODE* tailp = headp->prevp;
	DEQUE_NODE* tempp = headp->nextp;
	tempp->prevp = headp->prevp;
	tailp->nextp = tempp;
	
	dequep->count --;
	dequep->headp = tempp;

	void* elementp = headp->elementp;
	free(headp);

	return elementp;		
}


