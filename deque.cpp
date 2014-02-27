
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

#include "deque.h"

DEQUE_T* 	deque_init()
{
	DEQUE_T* dequep = (DEQUE_T*)malloc(sizeof(DEQUE_T));
	if(dequep != NULL)
	{
		memset(dequep, 0, sizeof(DEQUE_T));
	}

	return dequep;
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
		nodep = nodep->nextp;
	}
	
	if(dequep != NULL)
	{
		free(dequep);
		dequep = NULL;
	}
}


