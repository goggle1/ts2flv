
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "H264.h"
#include "dequeH.h"

#include "TS.h"


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


