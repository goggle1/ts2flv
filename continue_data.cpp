#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "deque.h"
#include "TS.h"
#include "continue_data.h"

extern u_int64_t	g_start_timestamp;
extern DEQUE_T		g_audio_es_deque;

u_int64_t net2host_64(u_int8_t data[8])
{
	u_int64_t value = 0;
	u_int64_t temp_value = 0;
	int index = 0;
	for(index=0; index<8; index++)
	{
		temp_value = data[index];
		value = value | temp_value << ((7-index)*8);
	}

	return value;
}


u_int32_t net2host_32(u_int8_t data[4])
{
	u_int32_t value = 0;
	u_int32_t temp_value = 0;
	int index = 0;
	for(index=0; index<4; index++)
	{
		temp_value = data[index];
		value = value | temp_value << ((3-index)*8);
	}

	return value;
}


int continue_data_read(char* channel)
{
	//char* data_file = "continue.data";
	char data_file[256] = {0};
	snprintf(data_file, 256, "%s.data", channel);
	data_file[255] = '\0';
	
	int fd = open(data_file, O_RDONLY);
    if(fd == -1)
    {
    	fprintf(stderr, "open %s error! \n", data_file);
        return -1;
    }

    CONTINUE_DATA_T continue_data = {{0}};
	ssize_t read_size = 0;    
	read_size = read(fd, (void*)&continue_data, sizeof(CONTINUE_DATA_T));
	if(read_size < (ssize_t)sizeof(CONTINUE_DATA_T))
	{
		close(fd);
		return -1;
	}
	
	g_start_timestamp = net2host_64(continue_data.timestamp);

	u_int32_t frame_count = net2host_32(continue_data.count);

	u_int32_t frame_index = 0;
	for(frame_index=0; frame_index<frame_count; frame_index++)
	{
		AUDIO_FRAME_T frame = {{0}};
		read_size = read(fd, (void*)&frame, sizeof(AUDIO_FRAME_T));
		if(read_size < (ssize_t)sizeof(AUDIO_FRAME_T))
		{
			close(fd);
			return -1;
		}

		AUDIO_ES_T es = {0};
		es.pts = net2host_64(frame.timestamp);
		es.len = net2host_32(frame.length);

		u_int8_t buffer[es.len];
		read_size = read(fd, (void*)buffer, sizeof(buffer));
		if(read_size < (ssize_t)sizeof(buffer))
		{
			close(fd);
			return -1;
		}
		es.ptr = buffer;

		
		fprintf(stdout, "%s: audio pts=%lu, audio len=%lu\n", __FUNCTION__, es.pts, es.len);	

		AUDIO_ES_T* newp = audio_es_copy(&es);
		if(newp != NULL)
		{
			deque_append(&g_audio_es_deque, newp);
		}
	}

	close(fd);

    return 0;
}


int continue_data_write(char* channel)
{
	//char* data_file = "continue.data";
	char data_file[256] = {0};
	snprintf(data_file, 256, "%s.data", channel);
	data_file[255] = '\0';
	
	int fd = open(data_file, O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
    if(fd == -1)
    {
    	fprintf(stderr, "open %s error! \n", data_file);
        return -1;
    }
    
	CONTINUE_DATA_T continue_data = {{0}};
	continue_data.signature[0]	= 'F';
	continue_data.signature[1]	= 'U';
	continue_data.signature[2]	= 'N';
	continue_data.signature[3]	= '\0';

	continue_data.count[0]		= g_audio_es_deque.count>>24 & 0x000000FF;
	continue_data.count[1]		= g_audio_es_deque.count>>16 & 0x000000FF;
	continue_data.count[2]		= g_audio_es_deque.count>> 8 & 0x000000FF;
	continue_data.count[3]		= g_audio_es_deque.count>> 0 & 0x000000FF;
	
	continue_data.timestamp[0] 	= g_start_timestamp>>56 & 0x000000FF;
	continue_data.timestamp[1]	= g_start_timestamp>>48 & 0x000000FF;
	continue_data.timestamp[2]	= g_start_timestamp>>40 & 0x000000FF;
	continue_data.timestamp[3]	= g_start_timestamp>>32 & 0x000000FF;
	continue_data.timestamp[4] 	= g_start_timestamp>>24 & 0x000000FF;
	continue_data.timestamp[5]	= g_start_timestamp>>16 & 0x000000FF;
	continue_data.timestamp[6]	= g_start_timestamp>> 8 & 0x000000FF;
	continue_data.timestamp[7]	= g_start_timestamp>> 0 & 0x000000FF;

	
	ssize_t write_size = write(fd, &continue_data, sizeof(CONTINUE_DATA_T));
	if(write_size != sizeof(CONTINUE_DATA_T))
	{
		close(fd);
		return -1;
	}

	AUDIO_ES_T* audio_esp = (AUDIO_ES_T*)deque_remove_head(&g_audio_es_deque);
	while(audio_esp != NULL)
	{		
		fprintf(stdout, "%s: audio pts=%lu, audio len=%lu\n", __FUNCTION__, audio_esp->pts, audio_esp->len);	

		u_int8_t	buffer[sizeof(AUDIO_FRAME_T)+audio_esp->len];
		AUDIO_FRAME_T* framep = (AUDIO_FRAME_T*)buffer;
		framep->timestamp[0] 	= audio_esp->pts>>56 & 0x000000FF;
		framep->timestamp[1]	= audio_esp->pts>>48 & 0x000000FF;
		framep->timestamp[2]	= audio_esp->pts>>40 & 0x000000FF;
		framep->timestamp[3]	= audio_esp->pts>>32 & 0x000000FF;
		framep->timestamp[4] 	= audio_esp->pts>>24 & 0x000000FF;
		framep->timestamp[5]	= audio_esp->pts>>16 & 0x000000FF;
		framep->timestamp[6]	= audio_esp->pts>> 8 & 0x000000FF;
		framep->timestamp[7]	= audio_esp->pts>> 0 & 0x000000FF;
		
		framep->length[0] = audio_esp->len>>24 & 0x000000FF;
		framep->length[1] = audio_esp->len>>16 & 0x000000FF;
		framep->length[2] = audio_esp->len>> 8 & 0x000000FF;
		framep->length[3] = audio_esp->len>> 0 & 0x000000FF;

		memcpy(framep->data, audio_esp->ptr, audio_esp->len);

		ssize_t write_size = write(fd, buffer, sizeof(buffer));
		if(write_size != (ssize_t)sizeof(buffer))
		{
			close(fd);
			return -1;
		}
		
		audio_es_release(audio_esp);
		audio_esp = NULL;
		audio_esp = (AUDIO_ES_T*)deque_remove_head(&g_audio_es_deque);
	}

	close(fd);

    return 0;
}


