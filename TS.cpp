
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "TS.h"
#include "flv.h"
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

u_int64_t	g_start_timestamp = 0;

int audio_get_aac(u_int8_t* datap, int len, AUDIO_SPECIFIC_CONFIG* configp, BUFFER_T* bufferp)
{
	int ret = 0;
	
	int pos = 0;
	while(pos < len)
	{
		ADTS_HEADER* headerp = (ADTS_HEADER*)(datap+pos);
		u_int32_t	sync_word 				= headerp->syncword_11_4 << 4 | 
												headerp->syncword_3_0;

		//configp->audioObjectType 			= headerp->profile;
		configp->audioObjectType 			= 2;
		configp->samplingFrequencyIndex_3_1 = headerp->sampling_frequency_index >> 1 & 0x0007;
		configp->samplingFrequencyIndex_0 	= headerp->sampling_frequency_index >> 0 & 0x0001;
		configp->channelConfiguration		= headerp->channel_configuration_2_2 << 2 | headerp->channel_configuration_1_0;
		configp->frameLengthFlag 			= 0;
		configp->dependsOnCoreCoder			= 0;
		configp->extensionFlag				= 0;

		u_int32_t	frame_length 			= headerp->frame_length_12_11 << 11 |
												headerp->frame_length_10_3 << 3 |
												headerp->frame_length_2_0;
		u_int32_t   adts_buffer_fullness 	= headerp->adts_buffer_fullness_10_6 << 6 |
												headerp->adts_buffer_fullness_5_0;
		u_int32_t	blocks_in_frame			= headerp->number_of_raw_data_blocks_in_frame;
		u_int32_t	adts_header_len			= 7;
		if(headerp->protection_absent == 0)
		{
			adts_header_len = 9;
		}
		u_int32_t	raw_data_len 			= frame_length - adts_header_len;


		fprintf(stdout, "%s: sync_word=0x%04X, profile=0x%02X, frequency=0x%02X, channels=0x%02X, fullness=0x%02X, "
			"blocks_in_frame=%u, frame_length=%u[0x%04X], header_len=%u\n",
			__FUNCTION__, sync_word, headerp->profile, headerp->sampling_frequency_index, configp->channelConfiguration, adts_buffer_fullness,
			blocks_in_frame, frame_length, frame_length, adts_header_len);
		ret = buffer_append(bufferp, datap+pos+adts_header_len, raw_data_len);
		pos += frame_length;		
	}

	return 0;	
}


int timestamp_parse(u_int8_t* buffer, int len, u_int64_t* outp)
{
	TIMESTAMP_T* timestampp = (TIMESTAMP_T*)buffer;
	u_int64_t	value = timestampp->timestamp_32_30 << 30 | 
						timestampp->timestamp_29_22 << 22 | 
						timestampp->timestamp_21_15 << 15 | 
						timestampp->timestamp_14_7 << 7 |
						timestampp->timestamp_6_0;
	*outp = value;
	fprintf(stdout, "%s: pts_or_dts_flag=0x%02X, timestamp=%lu \n", __FUNCTION__, timestampp->pts_or_dts_flag, value);
	return 0;
}

PES_T* pes_copy(PES_T* onep)
{
	PES_T* newp = (PES_T*)malloc(sizeof(PES_T));
	if(newp == NULL)
	{
		return NULL;
	}
	newp->pts = onep->pts;
	newp->dts = onep->dts;
	newp->ptr = (u_int8_t*)malloc(onep->len);
	if(newp->ptr == NULL)
	{
		free(newp);		
		return NULL;
	}
	
	memcpy(newp->ptr, onep->ptr, onep->len);
	newp->len = onep->len;
	return newp;
}

void pes_release(void* datap)
{
	PES_T* onep = (PES_T*)datap;
	if(onep == NULL)
	{
		return;
	}

	if(onep->ptr != NULL)
	{
		free(onep->ptr);
	}

	free(onep);
}


int pes_parse(u_int8_t* pes_buffer, int pes_len, PES_T* pesp)
{
	PES_HEADER* headerp = (PES_HEADER*)pes_buffer;
	fprintf(stdout, "%s: stream_id=%u[0x%02X] \n", __FUNCTION__, headerp->stream_id, headerp->stream_id);
	fprintf(stdout, "%s: PTS_DTS_flags=0x%02X \n", __FUNCTION__, headerp->PTS_DTS_flags);
	fprintf(stdout, "%s: PES_header_data_length=%u[0x%02X] \n", __FUNCTION__, headerp->PES_header_data_length, headerp->PES_header_data_length);

	pesp->ptr = pes_buffer + sizeof(PES_HEADER) + headerp->PES_header_data_length;
	pesp->len = pes_len - sizeof(PES_HEADER) - headerp->PES_header_data_length;
	if(headerp->PTS_DTS_flags & 0x02)
	{
		// pts
		timestamp_parse(pes_buffer+sizeof(PES_HEADER), sizeof(TIMESTAMP_T), &(pesp->pts));
	}
	if(headerp->PTS_DTS_flags & 0x01)
	{
		// dts
		timestamp_parse(pes_buffer+sizeof(PES_HEADER)+sizeof(TIMESTAMP_T), sizeof(TIMESTAMP_T), &(pesp->dts));
	}
	return 0;
}


int ts_parse(u_int8_t* ts_buffer)
{
	static u_int32_t s_ts_index = 0;
	
	TS_HEADER* headerp = (TS_HEADER*)ts_buffer;
	if(headerp->sync_byte != 0x47)
	{
		fprintf(stderr, "%s: sync_byte=0x%02X, !=0x47\n", __FUNCTION__, headerp->sync_byte);
		return -1;
	}
	if(headerp->transport_error_indicator != 0x00)
	{
		fprintf(stderr, "%s: transport_error_indicator=0x%02X, !=0x00\n", __FUNCTION__, headerp->transport_error_indicator);
		return -1;
	}	

	u_int32_t pid = headerp->PID_12_8 << 8 | headerp->PID_7_0;
	fprintf(stdout, "%s: index=%u, pid=%u[0x%04X]\n", __FUNCTION__, s_ts_index, pid, pid);
	s_ts_index ++;
	
	if(headerp->adaption_field_control == 0x00)
	{
		fprintf(stdout, "%s: adaption_field_control=0x%02X, ==0x00\n", __FUNCTION__, headerp->adaption_field_control);
		return -1;
	}

	int ts_pos = sizeof(TS_HEADER);	
		
	if(pid == 0x00)
	{
		// PAT
		if(headerp->adaption_field_control == 0x01)
		{
			ts_pos += 1;
		}
		else if(headerp->adaption_field_control == 0x03)
		{
			int adaptation_field_length = ts_buffer[ts_pos];
			ts_pos += adaptation_field_length;
			ts_pos += 1;
		}
		PAT_PACKET* packetp = (PAT_PACKET*)(ts_buffer + ts_pos);
		int section_length = packetp->section_length_11_8 << 8 | packetp->section_length_7_0;
		int programs_length = section_length - 5 - 4; // 5: PAT_PACEKT left, 4: crc32 size
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
		if(headerp->adaption_field_control == 0x01)
		{
			ts_pos += 1;
		}
		else if(headerp->adaption_field_control == 0x03)
		{
			int adaptation_field_length = ts_buffer[ts_pos];
			ts_pos += adaptation_field_length;
			ts_pos += 1;
		}
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
		if(headerp->adaption_field_control == 0x01)
		{
			ts_pos += 0;
		}
		else if(headerp->adaption_field_control == 0x03)
		{
			int adaptation_field_length = ts_buffer[ts_pos];
			ts_pos += adaptation_field_length;
			ts_pos += 1;
		}
		
		u_int8_t* datap = ts_buffer + ts_pos;
		int len = TS_PACKET_SIZE - ts_pos;		
		if(headerp->payload_unit_start_indicator)
		{			
			if(g_now_audio_pes.len != 0)
			{
				PES_T pes = {0};
				pes_parse(g_now_audio_pes.ptr, g_now_audio_pes.len, &pes);
				PES_T* newp = pes_copy(&pes);
				deque_append(g_audio_deque, newp);
			}
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
		if(headerp->adaption_field_control == 0x01)
		{
			ts_pos += 0;
		}
		else if(headerp->adaption_field_control == 0x03)
		{
			int adaptation_field_length = ts_buffer[ts_pos];
			ts_pos += adaptation_field_length;
			ts_pos += 1;
		}
		
		u_int8_t* datap = ts_buffer + ts_pos;
		int len = TS_PACKET_SIZE - ts_pos;		
		if(headerp->payload_unit_start_indicator)
		{
			if(g_now_video_pes.len != 0)
			{
				PES_T pes = {0};
				pes_parse(g_now_video_pes.ptr, g_now_video_pes.len, &pes);
				PES_T* newp = pes_copy(&pes);
				deque_append(g_video_deque, newp);
			}
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
		if(headerp->adaption_field_control == 0x01)
		{
			ts_pos += 0;
		}
		else if(headerp->adaption_field_control == 0x02)
		{
			ADAPTATION_FIELD* fieldp = (ADAPTATION_FIELD*)(ts_buffer + ts_pos);
			if(fieldp->PCR_flag)
			{
				ts_pos += sizeof(ADAPTATION_FIELD);
				PROGRAM_CLOCK_REFERENCE* pcrp = (PROGRAM_CLOCK_REFERENCE*)(ts_buffer + ts_pos);
				u_int64_t pcr_base 	= pcrp->program_clock_reference_base_32_25 << 25 |
										pcrp->program_clock_reference_base_24_17 << 17 |
										pcrp->program_clock_reference_base_16_9 << 9 |
										pcrp->program_clock_reference_base_8_1 << 1 | 
										pcrp->program_clock_reference_base_0;
				u_int32_t pcr_ext 	= pcrp->program_clock_reference_extension_8_8 << 8 |
										pcrp->program_clock_reference_extension_7_0;
				
				fprintf(stdout, "%s: pcr_base=%lu, pcr_ext=%u \n", __FUNCTION__, pcr_base, pcr_ext);
			}
		}
		else if(headerp->adaption_field_control == 0x03)
		{
			int adaptation_field_length = ts_buffer[ts_pos];
			ts_pos += adaptation_field_length;
			ts_pos += 1;
		}
	}

	return 0;
}


int pes_queue_merge(int fd)
{	
#if 0
	DEQUE_NODE* nodep = NULL;
	nodep = g_audio_deque->headp;
	while(nodep != NULL)
	{
		PES_T* pesp = (PES_T*)(nodep->elementp);
		fprintf(stdout, "%s: audio dts=%lu, pts=%lu\n", __FUNCTION__, pesp->dts, pesp->pts);
		if(nodep->nextp == g_audio_deque->headp)
		{
			break;
		}
		nodep = nodep->nextp;		
	}
	nodep = g_video_deque->headp;
	while(nodep != NULL)
	{
		PES_T* pesp = (PES_T*)(nodep->elementp);
		fprintf(stdout, "%s: video dts=%lu, pts=%lu, pts-dts=%lu\n", __FUNCTION__, pesp->dts, pesp->pts, pesp->pts - pesp->dts);
		if(nodep->nextp == g_video_deque->headp)
		{
			break;
		}
		nodep = nodep->nextp;		
	}
#endif

	PES_T* audio_pesp = (PES_T*)deque_remove_head(g_audio_deque);	
	PES_T* video_pesp = (PES_T*)deque_remove_head(g_video_deque);
	if(audio_pesp->pts <= video_pesp->dts)
	{
		g_start_timestamp = audio_pesp->pts;
	}
	else
	{
		g_start_timestamp = video_pesp->dts;
	}
		
	while(1)
	{	
		if(audio_pesp == NULL)
		{
			break;
		}
		if(video_pesp == NULL)
		{
			break;
		}
		if(audio_pesp->pts <= video_pesp->dts)
		{
			fprintf(stdout, "%s: audio pts=%lu \n", __FUNCTION__, audio_pesp->pts);				
			AUDIO_SPECIFIC_CONFIG aac_config = {0};
			BUFFER_T buffer = {0};
			audio_get_aac(audio_pesp->ptr, audio_pesp->len, &aac_config, &buffer);
			flv_write_audio(fd, (audio_pesp->pts-g_start_timestamp)/90, &aac_config, buffer.ptr, buffer.len);
			free(buffer.ptr);
			audio_pesp = (PES_T*)deque_remove_head(g_audio_deque);			
		}
		else
		{
			u_int64_t ts_diff = video_pesp->pts - video_pesp->dts;
			fprintf(stdout, "%s: video dts=%lu, pts=%lu, pts-dts=%lu \n", __FUNCTION__, video_pesp->dts, video_pesp->pts, ts_diff);	
			flv_write_video(fd, (video_pesp->dts-g_start_timestamp)/90, ts_diff/90, video_pesp->ptr, video_pesp->len);
			video_pesp = (PES_T*)deque_remove_head(g_video_deque);
		}
	}

	while(audio_pesp != NULL)
	{
		fprintf(stdout, "%s: audio pts=%lu \n", __FUNCTION__, audio_pesp->pts);	
		AUDIO_SPECIFIC_CONFIG aac_config = {0};
		BUFFER_T buffer = {0};
		audio_get_aac(audio_pesp->ptr, audio_pesp->len, &aac_config, &buffer);
		flv_write_audio(fd, (audio_pesp->pts-g_start_timestamp)/90, &aac_config, buffer.ptr, buffer.len);
		free(buffer.ptr);
		audio_pesp = (PES_T*)deque_remove_head(g_audio_deque);
	}

	while(video_pesp != NULL)
	{
		u_int64_t ts_diff = video_pesp->pts - video_pesp->dts;
		fprintf(stdout, "%s: video dts=%lu, pts=%lu, pts-dts=%lu \n", __FUNCTION__, video_pesp->dts, video_pesp->pts, ts_diff);	
		flv_write_video(fd, (video_pesp->dts-g_start_timestamp)/90, ts_diff/90, video_pesp->ptr, video_pesp->len);
		video_pesp = (PES_T*)deque_remove_head(g_video_deque);
	}

	return 0;
}


#define RELEASE()	\
{								\
	if(g_video_deque != NULL)	\
	{							\
		deque_release(g_video_deque, pes_release);	\
		g_video_deque = NULL;	\
	}							\
	if(g_audio_deque != NULL)	\
	{							\
		deque_release(g_audio_deque, pes_release);	\
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

    if(g_now_audio_pes.len != 0)
	{
		PES_T pes = {0};
		pes_parse(g_now_audio_pes.ptr, g_now_audio_pes.len, &pes);
		PES_T* newp = pes_copy(&pes);
		deque_append(g_audio_deque, newp);
	}
	if(g_now_video_pes.len != 0)
	{
		PES_T pes = {0};
		pes_parse(g_now_video_pes.ptr, g_now_video_pes.len, &pes);
		PES_T* newp = pes_copy(&pes);
		deque_append(g_video_deque, newp);
	}
		
	flv_write_header(flv_fd);
	pes_queue_merge(flv_fd);
	
    RELEASE();
    
	return ret;
}


