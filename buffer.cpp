
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "buffer.h"

void buffer_release(void* ptr)
{
	if(ptr == NULL)
	{
		return;
	}

	BUFFER_T* bufferp = (BUFFER_T*)ptr;
	if(bufferp->ptr != NULL)
	{
		free(bufferp->ptr);
	}

	if(ptr !=  NULL)
	{
		free(ptr);
		ptr = NULL;
	}
}

int buffer_append(BUFFER_T* bufferp, u_int8_t* datap, int len)
{
	int buffer_left = bufferp->size - bufferp->len;
	if(buffer_left < len)
	{
		void* temp = realloc(bufferp->ptr, bufferp->size + len);
		if(temp == NULL)
		{
			fprintf(stderr, "%s: realloc failed! size=%d\n", __FUNCTION__, bufferp->size + len);
			return -2;
		}
		bufferp->ptr = (u_int8_t*)temp;
		bufferp->size += len;				
	}
	memcpy(bufferp->ptr+bufferp->len, datap, len);
	bufferp->len += len;
			
	return 0;
}



