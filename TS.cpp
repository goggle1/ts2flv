
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include "TS.h"
#include "deque.h"
#include "buffer.h"

u_int32_t 	g_pid_pmt 	  	= 0;
u_int32_t 	g_pid_network 	= 0;
u_int32_t 	g_pid_audio   	= 0;
u_int32_t 	g_pid_video 	= 0;
DEQUE_T*	g_audio_deque 	= NULL;
DEQUE_T*	g_video_deque 	= NULL;
BUFFER_T	g_now_audio_pes = {0};
BUFFER_T	g_now_video_pes = {0};

int ts_parse(u_int8_t* ts_buffer)
{
	TS_HEADER* ts_headerp = (TS_HEADER*)ts_buffer;
	if(ts_headerp->sync_byte != 0x47)
	{
		fprintf(stderr, "%s: sync_byte=0x%02X, !=0x47\n", __FUNCTION__, ts_headerp->sync_byte);
		return -1;
	}
	if(ts_headerp->transport_error_indicator != 0x00)
	{
		fprintf(stderr, "%s: transport_error_indicator=0x%02X, !=0x00\n", __FUNCTION__, ts_headerp->transport_error_indicator);
		return -1;
	}	

	u_int32_t pid = ts_headerp->PID_12_8 << 8 | ts_headerp->PID_7_0;
	fprintf(stdout, "%s: pid=%u[0x%04X]\n", __FUNCTION__, pid, pid);
	
	if(ts_headerp->adaption_field_control == 0x00 || ts_headerp->adaption_field_control == 0x02)
	{
		fprintf(stdout, "%s: adaption_field_control=0x%02X, ![0x00,0x02]\n", __FUNCTION__, ts_headerp->adaption_field_control);
		//return -1;
	}

	int ts_pos = 4;
	if(ts_headerp->adaption_field_control == 0x01)
	{
		ts_pos += 1;
	}
	else if(ts_headerp->adaption_field_control == 0x03)
	{
		int adaptation_field_length = ts_buffer[ts_pos];
		ts_pos += adaptation_field_length;
		ts_pos += 1;
	}
		
	if(pid == 0x00)
	{
		// PAT
		PAT_PACKET* packetp = (PAT_PACKET*)(ts_buffer + ts_pos);
		int section_length = packetp->section_length_11_8 << 8 | packetp->section_length_7_0;
		int programs_length = section_length - 5 - 4;
		int programs_num = programs_length / sizeof(PROGRAM_T);

		int index = 0;
		for(index = 0;index<programs_num; index++)
		{
			PROGRAM_T* programp = &(packetp->programs[index]);
			int program_number = programp->program_number_15_8 << 8 | programp->program_number_7_0;
			if (program_number == 0x00)
			{ 
			 	g_pid_network = programp->network_PID_12_8 << 8 | programp->network_PID_7_0;
			 	fprintf(stdout, "%s: g_pid_network=%u[0x%04X]\n", __FUNCTION__, g_pid_network, g_pid_network);
			}
			else
			{			 
			 	g_pid_pmt = programp->network_PID_12_8 << 8 | programp->network_PID_7_0;
			 	fprintf(stdout, "%s: g_pid_pmt=%u[0x%04X]\n", __FUNCTION__, g_pid_pmt, g_pid_pmt);
			}
		}
	}
	//else if(g_pid_pmt != 0x00 && pid == g_pid_pmt)
	else if(pid == g_pid_pmt)
	{
		// PMT
		PMT_PACKET* packetp = (PMT_PACKET*)(ts_buffer + ts_pos);
		int section_length = packetp->section_length_11_8 << 8 | packetp->section_length_7_0;
		int program_info_length = packetp->program_info_length_11_8 << 8 | packetp->program_info_length_7_0;
		ts_pos += sizeof(PMT_PACKET);
		ts_pos += program_info_length;

		int components_length = section_length - 9 - program_info_length - 4;
		int components_pos = 0;
		while(components_pos < components_length)
		{		
			STREAM_T* streamp = (STREAM_T*)(ts_buffer + ts_pos + components_pos);
			int stream_type = streamp->stream_type;
			int elementary_PID = streamp->elementary_PID_12_8<<8 | streamp->elementary_PID_7_0;
			int ES_info_length = streamp->ES_info_length_11_8<<8 | streamp->ES_info_length_7_0;			
			fprintf(stdout, "%s: STREAM_T stream_type=%u[0x%04X], elementary_PID=%u[0x%04X], ES_info_length=%u[0x%04X]\n", 
				__FUNCTION__, stream_type, stream_type, elementary_PID, elementary_PID, ES_info_length, ES_info_length);
			if(stream_type == STREAM_AUDIO_AAC)
			{
				g_pid_audio = elementary_PID;
			}
			else if(stream_type == STREAM_VIDEO_H264)
			{
				g_pid_video = elementary_PID;
			}
				
			components_pos += sizeof(STREAM_T);			
			components_pos += ES_info_length;
		}
	}
	//else if(g_pid_audio != 0x00 && pid == g_pid_audio)
	else if(pid == g_pid_audio)
	{
		u_int8_t* datap = ts_buffer + ts_pos;
		int len = TS_PACKET_SIZE - ts_pos;		
		if(ts_headerp->payload_unit_start_indicator)
		{			
			g_now_audio_pes.len = 0;
			buffer_append(&g_now_audio_pes, datap, len);
			fprintf(stdout, "%s: audio frame begin, len=%d\n", __FUNCTION__, g_now_audio_pes.len);
		}
		else
		{	
			buffer_append(&g_now_audio_pes, datap, len);
			fprintf(stdout, "%s: audio frame ....., len=%d\n", __FUNCTION__, g_now_audio_pes.len);
		}
	}
	else if(pid == g_pid_video)
	{
		u_int8_t* datap = ts_buffer + ts_pos;
		int len = TS_PACKET_SIZE - ts_pos;		
		if(ts_headerp->payload_unit_start_indicator)
		{			
			g_now_video_pes.len = 0;
			buffer_append(&g_now_video_pes, datap, len);
			fprintf(stdout, "%s: video frame begin, len=%d\n", __FUNCTION__, g_now_video_pes.len);
		}
		else
		{	
			buffer_append(&g_now_video_pes, datap, len);
			fprintf(stdout, "%s: video frame ....., len=%d\n", __FUNCTION__, g_now_video_pes.len);
		}
	}
	else
	{
		fprintf(stdout, "%s: unknown pid=%d[0x%04X] \n", __FUNCTION__, pid, pid);
	}

	return 0;
}


#define RELEASE()	\
{								\
	if(g_video_deque != NULL)	\
	{							\
		deque_release(g_video_deque, buffer_release);	\
		g_video_deque = NULL;	\
	}							\
	if(g_audio_deque != NULL)	\
	{							\
		deque_release(g_audio_deque, buffer_release);	\
		g_audio_deque = NULL;	\
	}							\
	if(flv_fd != -1)			\
    {							\
    	close(flv_fd);			\
    	flv_fd = -1;			\
    }							\
    if(ts_fd != -1)				\
    {							\
    	close(ts_fd);			\
    	ts_fd = -1;				\
    }							\
}

int ts2flv(char* ts_file, char* flv_file)
{
	int ret  	= -1;
	
	int ts_fd 	= -1;
	int flv_fd 	= -1;
	
	ts_fd = open(ts_file, O_RDONLY);
    if(ts_fd == -1)
    {
        fprintf(stderr, "open %s error! \n", ts_file);	
        return -1;
    }

    flv_fd = open(flv_file, O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
    if(flv_fd == -1)
    {
    	fprintf(stderr, "open %s error! \n", flv_file);	
    	RELEASE();
        return -1;
    }

	g_audio_deque = deque_init();
	if(g_audio_deque == NULL)
	{
		RELEASE();
		return -1;
	}
	
	g_video_deque = deque_init();
	if(g_video_deque == NULL)
	{
		RELEASE();
		return -1;
	}
	
    u_int8_t ts_buffer[TS_PACKET_SIZE];
    ssize_t read_size = 0;
    while(1)
    {
    	read_size = read(ts_fd, (void*)ts_buffer, TS_PACKET_SIZE);
    	if(read_size < TS_PACKET_SIZE)
    	{
    		break;
    	}

    	ret = ts_parse(ts_buffer);    	
    	if(ret != 0)
    	{
    		break;
    	}	    	
    }

    RELEASE();
    
	return ret;
}


