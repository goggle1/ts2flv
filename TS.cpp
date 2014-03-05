
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "H264.h"
#include "TS.h"
#include "flv.h"
#include "deque.h"
#include "buffer.h"

#define TS_TIMESCALE  			90000
#define FLV_TIMESCALE 			1000
// TS_TIMESCALE/FLV_TIMESCALE
#define TS_FLV_TIME_RATE 		90

#define AAC_FRAME_SAMPLE_NUM 	1024

u_int32_t 	g_pid_pmt 	  		= 0;
u_int32_t 	g_pid_network 		= 0;

u_int32_t 	g_pid_audio   		= 0;
u_int32_t 	g_pid_video 		= 0;

u_int32_t 	g_audio_stream_type	= 0;
u_int32_t 	g_video_stream_type	= 0;

DEQUE_T		g_audio_pes_deque 	= {0};
DEQUE_T		g_video_pes_deque 	= {0};

DEQUE_T		g_audio_es_deque 	= {0};
DEQUE_T		g_video_es_deque 	= {0};

BUFFER_T	g_now_pat 			= {0};
BUFFER_T	g_now_pmt 			= {0};
BUFFER_T	g_now_audio_pes 	= {0};
BUFFER_T	g_now_video_pes 	= {0};

AUDIO_SPECIFIC_CONFIG 			g_audio_config = {0};
u_int8_t						g_video_config_buffer[1024] = {0};
int								g_video_config_pos = 0;
AVCDecoderConfigurationRecord* 	g_video_configp = (AVCDecoderConfigurationRecord*)g_video_config_buffer;

u_int64_t	g_start_timestamp 	= 0;

static u_int32_t sample_rate_table[] = 
{
	96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000,
};


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

void audio_es_release(void* datap)
{
	AUDIO_ES_T* onep = (AUDIO_ES_T*)datap;
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

void video_es_release(void* datap)
{
	VIDEO_ES_T* onep = (VIDEO_ES_T*)datap;
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


void nalu_release(void* datap)
{
	NALU_T* onep = (NALU_T*)datap;
	if(onep == NULL)
	{
		return;
	}

	/*
	if(onep->ptr != NULL)
	{
		free(onep->ptr);
	}
	*/

	free(onep);
}

NALU_T* nalu_copy(NALU_T* nalup)
{
	NALU_T* newp = (NALU_T*)malloc(sizeof(NALU_T));
	if(newp == NULL)
	{
		return NULL;
	}
	newp->ptr = nalup->ptr;
	newp->len = nalup->len;
	return newp;
}

AUDIO_ES_T* audio_es_copy(AUDIO_ES_T* onep)
{
	AUDIO_ES_T* newp = (AUDIO_ES_T*)malloc(sizeof(AUDIO_ES_T));
	if(newp == NULL)
	{
		return NULL;
	}
	newp->pts = onep->pts;
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

VIDEO_ES_T* video_es_copy(PES_T* onep)
{
	VIDEO_ES_T* newp = (VIDEO_ES_T*)malloc(sizeof(VIDEO_ES_T));
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
	newp->key_frame = 0;
	return newp;
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


int ts_parse_timestamp(u_int8_t* buffer, int len, u_int64_t* outp)
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

int ts_parse_pes(u_int8_t* pes_buffer, int pes_len, PES_T* pesp)
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
		ts_parse_timestamp(pes_buffer+sizeof(PES_HEADER), sizeof(TIMESTAMP_T), &(pesp->pts));
	}
	if(headerp->PTS_DTS_flags & 0x01)
	{
		// dts
		ts_parse_timestamp(pes_buffer+sizeof(PES_HEADER)+sizeof(TIMESTAMP_T), sizeof(TIMESTAMP_T), &(pesp->dts));
	}
	return 0;
}


int ts_parse_pmt(u_int8_t* buffer, int len)
{
	int ts_pos = 0;
	PMT_PACKET* packetp = (PMT_PACKET*)(buffer);
	int section_length = packetp->section_length_11_8 << 8 | packetp->section_length_7_0;
	int program_info_length = packetp->program_info_length_11_8 << 8 | packetp->program_info_length_7_0;
	ts_pos += sizeof(PMT_PACKET);
	ts_pos += program_info_length;

	int components_length = section_length - 9 - program_info_length - 4; // 9: PMT_PACKET left, 4: crc32 size
	int components_pos = 0;
	while(components_pos < components_length)
	{		
		STREAM_T* streamp = (STREAM_T*)(buffer + ts_pos + components_pos);
		int stream_type = streamp->stream_type;
		int elementary_PID = streamp->elementary_PID_12_8<<8 | streamp->elementary_PID_7_0;
		int ES_info_length = streamp->ES_info_length_11_8<<8 | streamp->ES_info_length_7_0;			
		fprintf(stdout, "%s: STREAM_T stream_type=%u[0x%04X], elementary_PID=%u[0x%04X], ES_info_length=%u[0x%04X]\n", 
			__FUNCTION__, stream_type, stream_type, elementary_PID, elementary_PID, ES_info_length, ES_info_length);
		if(stream_type == STREAM_AUDIO_AAC)
		{
			g_audio_stream_type = STREAM_AUDIO_AAC;
			g_pid_audio = elementary_PID;
		}
		else if(stream_type == STREAM_VIDEO_H264)
		{
			g_video_stream_type = STREAM_VIDEO_H264;
			g_pid_video = elementary_PID;
		}
			
		components_pos += sizeof(STREAM_T);			
		components_pos += ES_info_length;
	}
		
	return 0;
}


int ts_parse_pat(u_int8_t* buffer, int len)
{
	PAT_PACKET* packetp = (PAT_PACKET*)(buffer);
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
		
	return 0;
}


int ts_find_pat_pmt(u_int8_t* ts_buffer)
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
			ts_pos += 0;
		}
		else if(headerp->adaption_field_control == 0x03)
		{
			int adaptation_field_length = ts_buffer[ts_pos];
			ts_pos += 1;
			ts_pos += adaptation_field_length;		
		}
		
		if(headerp->payload_unit_start_indicator == 0x01)
		{
			int pointer_field = ts_buffer[ts_pos];
			ts_pos += 1;
			ts_pos += pointer_field;
		}

		u_int8_t* datap = ts_buffer + ts_pos;
		int len = TS_PACKET_SIZE - ts_pos;
		if(headerp->payload_unit_start_indicator)
		{	
			buffer_append(&g_now_pat, datap, len);
			fprintf(stdout, "%s: pat begin, len=%d\n", __FUNCTION__, g_now_pat.len);
		}
		else
		{	
			buffer_append(&g_now_pat, datap, len);
			fprintf(stdout, "%s: pat ....., len=%d\n", __FUNCTION__, g_now_pat.len);
		}

		ts_parse_pat(g_now_pat.ptr, g_now_pat.len);
		g_now_pat.len = 0;	
		
	}
	//else if(g_pid_pmt != 0x00 && pid == g_pid_pmt)
	else if(pid == g_pid_pmt)
	{
		// PMT
		if(headerp->adaption_field_control == 0x01)
		{
			ts_pos += 0;
		}
		else if(headerp->adaption_field_control == 0x03)
		{
			int adaptation_field_length = ts_buffer[ts_pos];
			ts_pos += 1;
			ts_pos += adaptation_field_length;			
		}

		if(headerp->payload_unit_start_indicator == 0x01)
		{
			int pointer_field = ts_buffer[ts_pos];
			ts_pos += 1;
			ts_pos += pointer_field;
		}

		u_int8_t* datap = ts_buffer + ts_pos;
		int len = TS_PACKET_SIZE - ts_pos;
		if(headerp->payload_unit_start_indicator)
		{	
			buffer_append(&g_now_pmt, datap, len);
			fprintf(stdout, "%s: pmt begin, len=%d\n", __FUNCTION__, g_now_pmt.len);
		}
		else
		{	
			buffer_append(&g_now_pmt, datap, len);
			fprintf(stdout, "%s: pmt ....., len=%d\n", __FUNCTION__, g_now_pmt.len);
		}

		ts_parse_pmt(g_now_pmt.ptr, g_now_pmt.len);
		g_now_pmt.len = 0;

		return 1;
		
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
			ts_pos += 0;
		}
		else if(headerp->adaption_field_control == 0x03)
		{
			int adaptation_field_length = ts_buffer[ts_pos];
			ts_pos += 1;
			ts_pos += adaptation_field_length;		
		}
		
		if(headerp->payload_unit_start_indicator == 0x01)
		{
			int pointer_field = ts_buffer[ts_pos];
			ts_pos += 1;
			ts_pos += pointer_field;
		}

		u_int8_t* datap = ts_buffer + ts_pos;
		int len = TS_PACKET_SIZE - ts_pos;
		if(headerp->payload_unit_start_indicator)
		{	
			buffer_append(&g_now_pat, datap, len);
			fprintf(stdout, "%s: pat begin, len=%d\n", __FUNCTION__, g_now_pat.len);
		}
		else
		{	
			buffer_append(&g_now_pat, datap, len);
			fprintf(stdout, "%s: pat ....., len=%d\n", __FUNCTION__, g_now_pat.len);
		}

		ts_parse_pat(g_now_pat.ptr, g_now_pat.len);
		g_now_pat.len = 0;	
		
	}
	//else if(g_pid_pmt != 0x00 && pid == g_pid_pmt)
	else if(pid == g_pid_pmt)
	{
		// PMT
		if(headerp->adaption_field_control == 0x01)
		{
			ts_pos += 0;
		}
		else if(headerp->adaption_field_control == 0x03)
		{
			int adaptation_field_length = ts_buffer[ts_pos];
			ts_pos += 1;
			ts_pos += adaptation_field_length;			
		}

		if(headerp->payload_unit_start_indicator == 0x01)
		{
			int pointer_field = ts_buffer[ts_pos];
			ts_pos += 1;
			ts_pos += pointer_field;
		}

		u_int8_t* datap = ts_buffer + ts_pos;
		int len = TS_PACKET_SIZE - ts_pos;
		if(headerp->payload_unit_start_indicator)
		{	
			buffer_append(&g_now_pmt, datap, len);
			fprintf(stdout, "%s: pmt begin, len=%d\n", __FUNCTION__, g_now_pmt.len);
		}
		else
		{	
			buffer_append(&g_now_pmt, datap, len);
			fprintf(stdout, "%s: pmt ....., len=%d\n", __FUNCTION__, g_now_pmt.len);
		}

		ts_parse_pmt(g_now_pmt.ptr, g_now_pmt.len);
		g_now_pmt.len = 0;
		
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
			ts_pos += 1;
			ts_pos += adaptation_field_length;			
		}
		
		u_int8_t* datap = ts_buffer + ts_pos;
		int len = TS_PACKET_SIZE - ts_pos;		
		if(headerp->payload_unit_start_indicator)
		{			
			if(g_now_audio_pes.len != 0)
			{
				PES_T pes = {0};
				ts_parse_pes(g_now_audio_pes.ptr, g_now_audio_pes.len, &pes);
				PES_T* newp = pes_copy(&pes);
				deque_append(&g_audio_pes_deque, newp);
			}
			g_now_audio_pes.len = 0;
			buffer_append(&g_now_audio_pes, datap, len);
			fprintf(stdout, "%s: audio frame begin, len=%d\n", __FUNCTION__, g_now_audio_pes.len);
		}
		else
		{	
			if(g_now_audio_pes.len > 0)
			{
				buffer_append(&g_now_audio_pes, datap, len);
				fprintf(stdout, "%s: audio frame ....., len=%d\n", __FUNCTION__, g_now_audio_pes.len);
			}
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
			ts_pos += 1;
			ts_pos += adaptation_field_length;			
		}
		
		u_int8_t* datap = ts_buffer + ts_pos;
		int len = TS_PACKET_SIZE - ts_pos;		
		if(headerp->payload_unit_start_indicator)
		{
			if(g_now_video_pes.len != 0)
			{
				PES_T pes = {0};
				ts_parse_pes(g_now_video_pes.ptr, g_now_video_pes.len, &pes);
				PES_T* newp = pes_copy(&pes);
				deque_append(&g_video_pes_deque, newp);
			}
			g_now_video_pes.len = 0;
			buffer_append(&g_now_video_pes, datap, len);
			fprintf(stdout, "%s: video frame begin, len=%d\n", __FUNCTION__, g_now_video_pes.len);
		}
		else
		{	
			if(g_now_video_pes.len > 0)
			{
				buffer_append(&g_now_video_pes, datap, len);
				fprintf(stdout, "%s: video frame ....., len=%d\n", __FUNCTION__, g_now_video_pes.len);
			}
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
			ts_pos += 1;
			ts_pos += adaptation_field_length;			
		}
	}

	return 0;
}


int audio_pes2es(PES_T* pesp)
{
	int ret = 0;

	int frame_index = 0;
	int data_pos = 0;
	u_int8_t* datap = pesp->ptr;
	int data_len = pesp->len;
	while(data_pos < data_len)
	{
		ADTS_HEADER* headerp = (ADTS_HEADER*)(datap+data_pos);
		u_int32_t	sync_word 				= headerp->syncword_11_4 << 4 | 
												headerp->syncword_3_0;

		//g_audio_config.audioObjectType 				= headerp->profile;
		g_audio_config.audioObjectType 				= 2; // todo: from profile to audioObjectType.
		g_audio_config.samplingFrequencyIndex_3_1 	= headerp->sampling_frequency_index >> 1 & 0x0007;
		g_audio_config.samplingFrequencyIndex_0 	= headerp->sampling_frequency_index >> 0 & 0x0001;
		g_audio_config.channelConfiguration			= headerp->channel_configuration_2_2 << 2 | headerp->channel_configuration_1_0;
		g_audio_config.frameLengthFlag 				= 0;
		g_audio_config.dependsOnCoreCoder			= 0;
		g_audio_config.extensionFlag				= 0;

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
			__FUNCTION__, sync_word, headerp->profile, headerp->sampling_frequency_index, g_audio_config.channelConfiguration, adts_buffer_fullness,
			blocks_in_frame, frame_length, frame_length, adts_header_len);	

		u_int32_t sample_rate = sample_rate_table[headerp->sampling_frequency_index];
		
		AUDIO_ES_T es = {0};
		es.ptr = datap+data_pos+adts_header_len;
		es.len = raw_data_len;
		es.pts = pesp->pts + AAC_FRAME_SAMPLE_NUM*TS_TIMESCALE/sample_rate*frame_index;
		AUDIO_ES_T* esp = audio_es_copy(&es);
		deque_append(&g_audio_es_deque, esp);
		
		data_pos += frame_length;
		frame_index ++;
	}

	return ret;
}


int avc_parse_nalu(VIDEO_ES_T* esp, DEQUE_T* dequep)
{
	int data_pos = 0;
	u_int8_t* datap = esp->ptr;
	int data_len = esp->len;

	NALU_T nalu = {0};
	
	while(data_pos < data_len)
	{
		if(datap[data_pos+0] == 0x00 &&
			datap[data_pos+1] == 0x00 &&
			datap[data_pos+2] == 0x00 &&
			datap[data_pos+3] == 0x01)
		{
			if(nalu.ptr != NULL)
			{
				nalu.len = &(datap[data_pos+0])-nalu.ptr;
				NALU_T* nalup = nalu_copy(&nalu);
				deque_append(dequep, nalup);
			}
			nalu.ptr = (u_int8_t*)&(datap[data_pos+0]);
		}
		data_pos ++;
	}

	if(nalu.ptr != NULL)
	{
		nalu.len = &(datap[data_pos+0])-nalu.ptr;
		NALU_T* nalup = nalu_copy(&nalu);
		deque_append(dequep, nalup);
	}
	
	return 0;
}


int video_pes2es(PES_T* pesp)
{
	int ret = 0;

	VIDEO_ES_T* esp = video_es_copy(pesp);

	DEQUE_T nalu_deque = {0};
	avc_parse_nalu(esp, &nalu_deque);
	DEQUE_NODE* headp = nalu_deque.headp;
	DEQUE_NODE* nodep = headp;
	while(nodep != NULL)
	{
		NALU_T* nalup = (NALU_T*)nodep->elementp;

		u_int32_t nalu_len = nalup->len - 4;
		nalup->ptr[0] = nalu_len >> 24 & 0x000000FF;
		nalup->ptr[1] = nalu_len >> 16 & 0x000000FF;
		nalup->ptr[2] = nalu_len >>  8 & 0x000000FF;
		nalup->ptr[3] = nalu_len >>  0 & 0x000000FF;
		
		u_int8_t nalu_type = nalup->ptr[4];
		nalu_type = nalu_type & 0x1F;
		if(nalu_type == NALU_TYPE_SPS)
		{
			u_int8_t* sps = &(nalup->ptr[4]);
			g_video_configp->configurationVersion 		= 1;
			g_video_configp->AVCProfileIndication 		= sps[1];
			g_video_configp->profile_compatibility 		= sps[2];
			g_video_configp->AVCLevelIndication			= sps[3];
			g_video_configp->reserved_1					= 0x3F;
			g_video_configp->lengthSizeMinusOne			= 3;
			g_video_configp->numOfSequenceParameterSets	= 1;
			g_video_config_pos = sizeof(AVCDecoderConfigurationRecord);
			
			PARAMETER_SET* setp = (PARAMETER_SET*)&(g_video_config_buffer[g_video_config_pos]);
			int sps_len = nalup->len - 4;
			setp->sequenceParameterSetLength_15_8	= sps_len >> 8 & 0x00FF;
			setp->sequenceParameterSetLength_7_0 	= sps_len >> 0 & 0x00FF;
			memcpy(setp->sequenceParameterSetNALUnit, sps, sps_len);
			g_video_config_pos += sizeof(PARAMETER_SET);
			g_video_config_pos += sps_len;
		}
		else if(nalu_type == NALU_TYPE_PPS)
		{
			u_int8_t* pps = &(nalup->ptr[4]);
			g_video_config_buffer[g_video_config_pos] = 1;
			g_video_config_pos ++;
			
			PARAMETER_SET* setp = (PARAMETER_SET*)&(g_video_config_buffer[g_video_config_pos]);
			int pps_len = nalup->len - 4;
			setp->sequenceParameterSetLength_15_8	= pps_len >> 8 & 0x00FF;
			setp->sequenceParameterSetLength_7_0 	= pps_len >> 0 & 0x00FF;
			memcpy(setp->sequenceParameterSetNALUnit, pps, pps_len);
			g_video_config_pos += sizeof(PARAMETER_SET);
			g_video_config_pos += pps_len;
		}
		else if(nalu_type == NALU_TYPE_IDR)
		{
			esp->key_frame = 1;
			fprintf(stdout, "%s: nalu_type is IDR[0x%02X]\n", __FUNCTION__, nalu_type);
		}
		else if(nalu_type == NALU_TYPE_SLICE)
		{
			u_int8_t* datap = &(nalup->ptr[5]);
			u_int32_t first_mb_in_slice = 0;
			u_int32_t slice_type = 0;
			int bits_start_pos 	= 0;
			int bits_end_pos 	= 0;
			bits_read_ue(datap, bits_start_pos, &first_mb_in_slice, &bits_end_pos);
			bits_start_pos 		= bits_end_pos;
			bits_end_pos 		= 0;
			bits_read_ue(datap, bits_start_pos, &slice_type, &bits_end_pos);
			fprintf(stdout, "%s: first_mb_in_slice=0x%02X, slice_type=0x%02X\n", __FUNCTION__, first_mb_in_slice, slice_type);
			if(slice_type == SLICE_TYPE_I ||
				slice_type == SLICE_TYPE_I2 ||
				slice_type == SLICE_TYPE_SI ||
				slice_type == SLICE_TYPE_SI2)
			{
				esp->key_frame = 1;
			}
		}
		
		
		if(nodep->nextp == headp)
		{
			break;
		}
		nodep = nodep->nextp;
	}
		
	deque_append(&g_video_es_deque, esp);

	deque_release(&nalu_deque, nalu_release);
	
	return ret;
}


int pes_queue_to_es_queue()
{
	DEQUE_NODE* nodep = NULL;
	nodep = g_audio_pes_deque.headp;
	while(nodep != NULL)
	{
		PES_T* pesp = (PES_T*)(nodep->elementp);
		fprintf(stdout, "%s: audio dts=%lu, pts=%lu\n", __FUNCTION__, pesp->dts, pesp->pts);
		audio_pes2es(pesp);
		if(nodep->nextp == g_audio_pes_deque.headp)
		{
			break;
		}
		nodep = nodep->nextp;		
	}

	nodep = g_video_pes_deque.headp;
	while(nodep != NULL)
	{
		PES_T* pesp = (PES_T*)(nodep->elementp);
		fprintf(stdout, "%s: video dts=%lu, pts=%lu, pts-dts=%lu\n", __FUNCTION__, pesp->dts, pesp->pts, pesp->pts - pesp->dts);
		video_pes2es(pesp);
		if(nodep->nextp == g_video_pes_deque.headp)
		{
			break;
		}
		nodep = nodep->nextp;		
	}

	return 0;
}


int es_queue_merge_to_flv(int fd)
{
	AUDIO_ES_T* audio_esp = (AUDIO_ES_T*)deque_remove_head(&g_audio_es_deque);	
	VIDEO_ES_T* video_esp = (VIDEO_ES_T*)deque_remove_head(&g_video_es_deque);
	if(audio_esp->pts <= video_esp->dts)
	{
		g_start_timestamp = audio_esp->pts;
	}
	else
	{
		g_start_timestamp = video_esp->dts;
	}
	fprintf(stdout, "%s: g_start_timestamp=%lu \n", __FUNCTION__, g_start_timestamp);

	flv_write_aac_header(fd, (g_start_timestamp-g_start_timestamp)/TS_FLV_TIME_RATE, &g_audio_config);
	flv_write_avc_header(fd, (g_start_timestamp-g_start_timestamp)/TS_FLV_TIME_RATE, g_video_configp, g_video_config_pos);
		
	while(1)
	{	
		if(audio_esp == NULL)
		{
			break;
		}
		if(video_esp == NULL)
		{
			break;
		}
		if(audio_esp->pts <= video_esp->dts)
		{
			fprintf(stdout, "%s: audio pts=%lu \n", __FUNCTION__, audio_esp->pts);							
			flv_write_audio(fd, (audio_esp->pts-g_start_timestamp)/TS_FLV_TIME_RATE, audio_esp->ptr, audio_esp->len);
			audio_es_release(audio_esp);
			audio_esp = NULL;
			audio_esp = (AUDIO_ES_T*)deque_remove_head(&g_audio_es_deque);			
		}
		else
		{
			u_int64_t ts_diff = video_esp->pts - video_esp->dts;
			fprintf(stdout, "%s: video dts=%lu, pts=%lu, pts-dts=%lu \n", __FUNCTION__, video_esp->dts, video_esp->pts, ts_diff);	
			flv_write_video(fd, (video_esp->dts-g_start_timestamp)/TS_FLV_TIME_RATE, ts_diff/TS_FLV_TIME_RATE, video_esp->ptr, video_esp->len, video_esp->key_frame);
			video_es_release(video_esp);
			video_esp = NULL;
			video_esp = (VIDEO_ES_T*)deque_remove_head(&g_video_es_deque);
		}
	}

	while(audio_esp != NULL)
	{
		fprintf(stdout, "%s: audio pts=%lu \n", __FUNCTION__, audio_esp->pts);							
		flv_write_audio(fd, (audio_esp->pts-g_start_timestamp)/TS_FLV_TIME_RATE, audio_esp->ptr, audio_esp->len);
		audio_es_release(audio_esp);
		audio_esp = NULL;
		audio_esp = (AUDIO_ES_T*)deque_remove_head(&g_audio_es_deque);
	}

	while(video_esp != NULL)
	{
		u_int64_t ts_diff = video_esp->pts - video_esp->dts;
		fprintf(stdout, "%s: video dts=%lu, pts=%lu, pts-dts=%lu \n", __FUNCTION__, video_esp->dts, video_esp->pts, ts_diff);	
		flv_write_video(fd, (video_esp->dts-g_start_timestamp)/TS_FLV_TIME_RATE, ts_diff/TS_FLV_TIME_RATE, video_esp->ptr, video_esp->len, video_esp->key_frame);
		video_es_release(video_esp);
		video_esp = NULL;
		video_esp = (VIDEO_ES_T*)deque_remove_head(&g_video_es_deque);
	}

	return 0;
}


#define RELEASE()	\
{								\
	deque_release(&g_video_es_deque, video_es_release); \
	deque_release(&g_audio_es_deque, audio_es_release); \
	deque_release(&g_video_pes_deque, pes_release);	\
	deque_release(&g_audio_pes_deque, pes_release);	\
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

    u_int8_t ts_buffer[TS_PACKET_SIZE];
    ssize_t read_size = 0;    
	while(1)
	{
		read_size = read(ts_fd, (void*)ts_buffer, TS_PACKET_SIZE);
		if(read_size < TS_PACKET_SIZE)
		{
			break;
		}

		ret = ts_find_pat_pmt(ts_buffer);		
		if(ret != 0)
		{
			break;
		}			
	}
	if(ret < 0)
	{
		RELEASE();
        return -1;
	}
	else if(ret == 0)
	{
		RELEASE();
        return -1;
	}

	lseek(ts_fd, 0, SEEK_SET);
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
		ts_parse_pes(g_now_audio_pes.ptr, g_now_audio_pes.len, &pes);
		PES_T* newp = pes_copy(&pes);
		deque_append(&g_audio_pes_deque, newp);
	}
	if(g_now_video_pes.len != 0)
	{
		PES_T pes = {0};
		ts_parse_pes(g_now_video_pes.ptr, g_now_video_pes.len, &pes);
		PES_T* newp = pes_copy(&pes);
		deque_append(&g_video_pes_deque, newp);
	}

	pes_queue_to_es_queue();
		
	flv_write_header(flv_fd, 1, 1);
	es_queue_merge_to_flv(flv_fd);
	
    RELEASE();
    
	return ret;
}


