
#ifndef __BUFFER_H__
#define __BUFFER_H__

typedef struct buffer_t
{
	u_int8_t* 	ptr;
	int 		size;
	int			len;
} BUFFER_T;

void 	buffer_release(void* ptr);
int 	buffer_append(BUFFER_T* bufferp, u_int8_t* datap, int len);

#endif

