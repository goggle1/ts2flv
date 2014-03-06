
#ifndef __CONTINUE_DATA_H__
#define __CONTINUE_DATA_H__

#include <sys/types.h>

#pragma pack(1)

typedef struct continue_data_t
{
	u_int8_t	signature[4];
	u_int8_t	count[4]; // AUDIO_FRAM_T count
	u_int8_t	timestamp[8];	
	u_int8_t	data[0];
} CONTINUE_DATA_T;

typedef struct audio_frame_t
{
	u_int8_t	timestamp[8];
	u_int8_t	length[4];
	u_int8_t	data[0];
} AUDIO_FRAME_T;

#pragma pack()

int continue_data_read(char* channel);
int continue_data_write(char* channel);

#endif

