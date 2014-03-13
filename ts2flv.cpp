#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "H264.h"
#include "ts2flv.h"

#define TS_TIMESCALE  			90000
#define FLV_TIMESCALE 			1000
// TS_TIMESCALE/FLV_TIMESCALE
#define TS_FLV_TIME_RATE 		90

#define AAC_FRAME_SAMPLE_NUM 	1024


static u_int32_t sample_rate_table[] = 
{
	96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000,
};


#define memory_release()	\
{								\
	dequeh_release(&m_video_es_deque, video_es_release); \
	dequeh_release(&m_audio_es_deque, audio_es_release); \
	dequeh_release(&m_video_pes_deque, pes_release);	\
	dequeh_release(&m_audio_pes_deque, pes_release);	\
}

int avc_parse_nalu(VIDEO_ES_T* esp, DEQUEH_T* dequep)
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
				dequeh_append(dequep, nalup);
			}
			nalu.ptr = (u_int8_t*)&(datap[data_pos+0]);
		}
		data_pos ++;
	}

	if(nalu.ptr != NULL)
	{
		nalu.len = &(datap[data_pos+0])-nalu.ptr;
		NALU_T* nalup = nalu_copy(&nalu);
		dequeh_append(dequep, nalup);
	}
	
	return 0;
}


TS2FLV::TS2FLV()
{
	m_pid_pmt 	  			= 0;
	m_pid_network 			= 0;

	m_pid_audio   			= 0;
	m_pid_video 			= 0;

	m_audio_stream_type		= 0;
	m_video_stream_type		= 0;

	m_audio_pes_deque.count	= 0;
	m_audio_pes_deque.headp	= NULL;
	m_video_pes_deque.count = 0;
	m_video_pes_deque.headp = NULL;

	m_audio_es_deque.count 	= 0;
	m_audio_es_deque.headp 	= NULL;	
	m_video_es_deque.count 	= 0;
	m_video_es_deque.headp 	= NULL;

	memset(&m_pat_buffer, 0, sizeof(m_pat_buffer));
	memset(&m_pmt_buffer, 0, sizeof(m_pmt_buffer));
	memset(&m_audio_buffer, 0, sizeof(m_audio_buffer));
	memset(&m_video_buffer, 0, sizeof(m_video_buffer));

	memset(&m_audio_config, 0, sizeof(m_audio_config));
	memset(m_video_config_buffer, 0, sizeof(m_video_config_buffer));
	m_video_config_pos 	= 0;
	m_video_configp 	= (AVCDecoderConfigurationRecord*)m_video_config_buffer;

	m_start_timestamp 	= 0;
}

TS2FLV::~TS2FLV()
{
	// todo:
}


int TS2FLV::audio_pes2es(PES_T* pesp)
{
	int ret = 0;

	int frame_index = 0;
	int data_pos = 0;
	u_int8_t* datap = pesp->ptr;
	int data_len = pesp->len;
	while(data_pos < data_len)
	{
		if(data_len - data_pos < 7)
		{
			fprintf(stderr, "%s: data_len[%d]-data_pos[%d]<7\n", __FUNCTION__, data_len, data_pos);
			break;
		}
		
		ADTS_HEADER* headerp = (ADTS_HEADER*)(datap+data_pos);
		u_int32_t	sync_word 				= headerp->syncword_11_4 << 4 | 
												headerp->syncword_3_0;
		if(sync_word != 0x0FFF)
		{
			fprintf(stderr, "%s: sync_word != 0x0FFF\n", __FUNCTION__);
			break;
		}

		//m_audio_config.audioObjectType 				= headerp->profile;
		m_audio_config.audioObjectType 				= 2; // todo: from profile to audioObjectType.
		m_audio_config.samplingFrequencyIndex_3_1 	= headerp->sampling_frequency_index >> 1 & 0x0007;
		m_audio_config.samplingFrequencyIndex_0 	= headerp->sampling_frequency_index >> 0 & 0x0001;
		m_audio_config.channelConfiguration			= headerp->channel_configuration_2_2 << 2 | headerp->channel_configuration_1_0;
		m_audio_config.frameLengthFlag 				= 0;
		m_audio_config.dependsOnCoreCoder			= 0;
		m_audio_config.extensionFlag				= 0;

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
			__FUNCTION__, sync_word, headerp->profile, headerp->sampling_frequency_index, m_audio_config.channelConfiguration, adts_buffer_fullness,
			blocks_in_frame, frame_length, frame_length, adts_header_len);	

		u_int32_t sample_rate = sample_rate_table[headerp->sampling_frequency_index];
		
		AUDIO_ES_T es = {0};
		es.ptr = datap+data_pos+adts_header_len;
		es.len = raw_data_len;
		es.pts = pesp->pts + AAC_FRAME_SAMPLE_NUM*TS_TIMESCALE/sample_rate*frame_index;
		AUDIO_ES_T* esp = audio_es_copy(&es);
		dequeh_append(&m_audio_es_deque, esp);
		
		data_pos += frame_length;
		frame_index ++;
	}

	return ret;
}


int TS2FLV::video_pes2es(PES_T* pesp)
{
	int ret = 0;

	VIDEO_ES_T* esp = video_es_copy(pesp);

	DEQUEH_T nalu_deque = {0};
	avc_parse_nalu(esp, &nalu_deque);
	DEQUEH_NODE* headp = nalu_deque.headp;
	DEQUEH_NODE* nodep = headp;
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
			m_video_configp->configurationVersion 		= 1;
			m_video_configp->AVCProfileIndication 		= sps[1];
			m_video_configp->profile_compatibility 		= sps[2];
			m_video_configp->AVCLevelIndication			= sps[3];
			m_video_configp->reserved_1					= 0x3F;
			m_video_configp->lengthSizeMinusOne			= 3;
			m_video_configp->numOfSequenceParameterSets	= 1;
			m_video_config_pos = sizeof(AVCDecoderConfigurationRecord);
			
			PARAMETER_SET* setp = (PARAMETER_SET*)&(m_video_config_buffer[m_video_config_pos]);
			int sps_len = nalup->len - 4;
			setp->sequenceParameterSetLength_15_8	= sps_len >> 8 & 0x00FF;
			setp->sequenceParameterSetLength_7_0 	= sps_len >> 0 & 0x00FF;
			memcpy(setp->sequenceParameterSetNALUnit, sps, sps_len);
			m_video_config_pos += sizeof(PARAMETER_SET);
			m_video_config_pos += sps_len;
		}
		else if(nalu_type == NALU_TYPE_PPS)
		{
			u_int8_t* pps = &(nalup->ptr[4]);
			m_video_config_buffer[m_video_config_pos] = 1;
			m_video_config_pos ++;
			
			PARAMETER_SET* setp = (PARAMETER_SET*)&(m_video_config_buffer[m_video_config_pos]);
			int pps_len = nalup->len - 4;
			setp->sequenceParameterSetLength_15_8	= pps_len >> 8 & 0x00FF;
			setp->sequenceParameterSetLength_7_0 	= pps_len >> 0 & 0x00FF;
			memcpy(setp->sequenceParameterSetNALUnit, pps, pps_len);
			m_video_config_pos += sizeof(PARAMETER_SET);
			m_video_config_pos += pps_len;
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
		
	dequeh_append(&m_video_es_deque, esp);

	dequeh_release(&nalu_deque, nalu_release);
	
	return ret;
}


int TS2FLV::pes_to_es()
{
	DEQUEH_NODE* nodep = NULL;
	nodep = m_audio_pes_deque.headp;
	while(nodep != NULL)
	{
		PES_T* pesp = (PES_T*)(nodep->elementp);
		//fprintf(stdout, "%s: audio dts=%lu, pts=%lu\n", __FUNCTION__, pesp->dts, pesp->pts);
		audio_pes2es(pesp);
		if(nodep->nextp == m_audio_pes_deque.headp)
		{
			break;
		}
		nodep = nodep->nextp;		
	}

	nodep = m_video_pes_deque.headp;
	while(nodep != NULL)
	{
		PES_T* pesp = (PES_T*)(nodep->elementp);
		fprintf(stdout, "%s: video dts=%lu, pts=%lu, pts-dts=%lu\n", __FUNCTION__, pesp->dts, pesp->pts, pesp->pts - pesp->dts);
		video_pes2es(pesp);
		if(nodep->nextp == m_video_pes_deque.headp)
		{
			break;
		}
		nodep = nodep->nextp;		
	}

	return 0;
}


int TS2FLV::ts_find_pat_pmt(u_int8_t* ts_buffer)
{	
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
	fprintf(stdout, "%s: index=%u, pid=%u[0x%04X]\n", __FUNCTION__, m_ts_index, pid, pid);
	m_ts_index ++;
	
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
			buffer_append(&m_pat_buffer, datap, len);
			fprintf(stdout, "%s: pat begin, len=%d\n", __FUNCTION__, m_pat_buffer.len);
		}
		else
		{	
			buffer_append(&m_pat_buffer, datap, len);
			fprintf(stdout, "%s: pat ....., len=%d\n", __FUNCTION__, m_pat_buffer.len);
		}

		ts_parse_pat(m_pat_buffer.ptr, m_pat_buffer.len);
		m_pat_buffer.len = 0;	
		
	}
	else if(pid == m_pid_pmt)
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
			buffer_append(&m_pmt_buffer, datap, len);
			fprintf(stdout, "%s: pmt begin, len=%d\n", __FUNCTION__, m_pmt_buffer.len);
		}
		else
		{	
			buffer_append(&m_pmt_buffer, datap, len);
			fprintf(stdout, "%s: pmt ....., len=%d\n", __FUNCTION__, m_pmt_buffer.len);
		}

		ts_parse_pmt(m_pmt_buffer.ptr, m_pmt_buffer.len);
		m_pmt_buffer.len = 0;

		return 1;
		
	}

	return 0;
}


int TS2FLV::ts_parse_pmt(u_int8_t* buffer, int len)
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
			m_audio_stream_type = STREAM_AUDIO_AAC;
			m_pid_audio = elementary_PID;
		}
		else if(stream_type == STREAM_VIDEO_H264)
		{
			m_video_stream_type = STREAM_VIDEO_H264;
			m_pid_video = elementary_PID;
		}
			
		components_pos += sizeof(STREAM_T);			
		components_pos += ES_info_length;
	}
		
	return 0;
}


int TS2FLV::ts_parse_pat(u_int8_t* buffer, int len)
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
		 	m_pid_network = programp->network_PID_12_8 << 8 | programp->network_PID_7_0;
		 	fprintf(stdout, "%s: m_pid_network=%u[0x%04X]\n", __FUNCTION__, m_pid_network, m_pid_network);
		}
		else
		{			 
		 	m_pid_pmt = programp->network_PID_12_8 << 8 | programp->network_PID_7_0;
		 	fprintf(stdout, "%s: m_pid_pmt=%u[0x%04X]\n", __FUNCTION__, m_pid_pmt, m_pid_pmt);
		}
	}
		
	return 0;
}


int TS2FLV::ts_parse(u_int8_t* ts_buffer)
{	
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
	fprintf(stdout, "%s: index=%u, pid=%u[0x%04X]\n", __FUNCTION__, m_ts_index, pid, pid);
	m_ts_index ++;
	
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
			buffer_append(&m_pat_buffer, datap, len);
			fprintf(stdout, "%s: pat begin, len=%d\n", __FUNCTION__, m_pat_buffer.len);
		}
		else
		{	
			buffer_append(&m_pat_buffer, datap, len);
			fprintf(stdout, "%s: pat ....., len=%d\n", __FUNCTION__, m_pat_buffer.len);
		}

		ts_parse_pat(m_pat_buffer.ptr, m_pat_buffer.len);
		m_pat_buffer.len = 0;	
		
	}	
	else if(pid == m_pid_pmt)
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
			buffer_append(&m_pmt_buffer, datap, len);
			fprintf(stdout, "%s: pmt begin, len=%d\n", __FUNCTION__, m_pmt_buffer.len);
		}
		else
		{	
			buffer_append(&m_pmt_buffer, datap, len);
			fprintf(stdout, "%s: pmt ....., len=%d\n", __FUNCTION__, m_pmt_buffer.len);
		}

		ts_parse_pmt(m_pmt_buffer.ptr, m_pmt_buffer.len);
		m_pmt_buffer.len = 0;
		
	}
	else if(pid == m_pid_audio)
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
			if(m_audio_buffer.len != 0)
			{
				PES_T pes = {0};
				ts_parse_pes(m_audio_buffer.ptr, m_audio_buffer.len, &pes);
				PES_T* newp = pes_copy(&pes);
				dequeh_append(&m_audio_pes_deque, newp);
			}
			m_audio_buffer.len = 0;
			buffer_append(&m_audio_buffer, datap, len);
			fprintf(stdout, "%s: audio frame begin, len=%d\n", __FUNCTION__, m_audio_buffer.len);
		}
		else
		{	
			if(m_audio_buffer.len > 0)
			{
				buffer_append(&m_audio_buffer, datap, len);
				fprintf(stdout, "%s: audio frame ....., len=%d\n", __FUNCTION__, m_audio_buffer.len);
			}
		}
	}
	else if(pid == m_pid_video)
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
			if(m_video_buffer.len != 0)
			{
				PES_T pes = {0};
				ts_parse_pes(m_video_buffer.ptr, m_video_buffer.len, &pes);
				PES_T* newp = pes_copy(&pes);
				dequeh_append(&m_video_pes_deque, newp);
			}
			m_video_buffer.len = 0;
			buffer_append(&m_video_buffer, datap, len);
			fprintf(stdout, "%s: video frame begin, len=%d\n", __FUNCTION__, m_video_buffer.len);
		}
		else
		{	
			if(m_video_buffer.len > 0)
			{
				buffer_append(&m_video_buffer, datap, len);
				fprintf(stdout, "%s: video frame ....., len=%d\n", __FUNCTION__, m_video_buffer.len);
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


int TS2FLV::flv_memo_es(u_int8_t* flv_memoryp, int flv_size)
{
	u_int8_t* 	flv_bufferp		= flv_memoryp;
    int			flv_position	= 0;
    int			flv_remain_size = flv_size;
    int 		flv_memo_size	= 0;
    
	AUDIO_ES_T* audio_esp = (AUDIO_ES_T*)dequeh_remove_head(&m_audio_es_deque);	
	VIDEO_ES_T* video_esp = (VIDEO_ES_T*)dequeh_remove_head(&m_video_es_deque);	
		
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
			flv_bufferp = flv_memoryp + flv_position;
			flv_memo_size = flv_memo_audio(flv_bufferp, flv_remain_size, (audio_esp->pts-m_start_timestamp)/TS_FLV_TIME_RATE, audio_esp->ptr, audio_esp->len);
			flv_position += flv_memo_size;
			flv_remain_size -= flv_memo_size;
			audio_es_release(audio_esp);
			audio_esp = NULL;
			audio_esp = (AUDIO_ES_T*)dequeh_remove_head(&m_audio_es_deque);			
		}
		else
		{
			u_int64_t ts_diff = video_esp->pts - video_esp->dts;
			fprintf(stdout, "%s: video dts=%lu, pts=%lu, pts-dts=%lu \n", __FUNCTION__, video_esp->dts, video_esp->pts, ts_diff);	
			flv_bufferp = flv_memoryp + flv_position;
			flv_memo_size = flv_memo_video(flv_bufferp, flv_remain_size, (video_esp->dts-m_start_timestamp)/TS_FLV_TIME_RATE, ts_diff/TS_FLV_TIME_RATE, video_esp->ptr, video_esp->len, video_esp->key_frame);
			flv_position += flv_memo_size;
			flv_remain_size -= flv_memo_size;
			video_es_release(video_esp);
			video_esp = NULL;
			video_esp = (VIDEO_ES_T*)dequeh_remove_head(&m_video_es_deque);
		}
	}

	while(audio_esp != NULL)
	{		
		fprintf(stdout, "%s: audio pts=%lu \n", __FUNCTION__, audio_esp->pts);	
		flv_bufferp = flv_memoryp + flv_position;
		flv_memo_size = flv_memo_audio(flv_bufferp, flv_remain_size, (audio_esp->pts-m_start_timestamp)/TS_FLV_TIME_RATE, audio_esp->ptr, audio_esp->len);
		flv_position += flv_memo_size;
		flv_remain_size -= flv_memo_size;
		audio_es_release(audio_esp);
		audio_esp = NULL;
		audio_esp = (AUDIO_ES_T*)dequeh_remove_head(&m_audio_es_deque);
	}

	while(video_esp != NULL)
	{
		u_int64_t ts_diff = video_esp->pts - video_esp->dts;
		fprintf(stdout, "%s: video dts=%lu, pts=%lu, pts-dts=%lu \n", __FUNCTION__, video_esp->dts, video_esp->pts, ts_diff);	
		flv_bufferp = flv_memoryp + flv_position;
		flv_memo_size = flv_memo_video(flv_bufferp, flv_remain_size, (video_esp->dts-m_start_timestamp)/TS_FLV_TIME_RATE, ts_diff/TS_FLV_TIME_RATE, video_esp->ptr, video_esp->len, video_esp->key_frame);
		flv_position += flv_memo_size;
		flv_remain_size -= flv_memo_size;
		video_es_release(video_esp);
		video_esp = NULL;
		video_esp = (VIDEO_ES_T*)dequeh_remove_head(&m_video_es_deque);
	}

	return flv_position;
}


int TS2FLV::buff2buff(u_int8_t * ts_memoryp, int ts_len, u_int8_t * flv_memoryp, int flv_size)
{
	int ret = 0;
	
	u_int8_t*	ts_bufferp	= ts_memoryp;
	int 		ts_position = 0;
	m_ts_index = 0;
	while(1)
	{
		ts_bufferp = ts_memoryp + ts_position;
		if(ts_position + TS_PACKET_SIZE > ts_len)
		{
			break;
		}

		ret = ts_find_pat_pmt(ts_bufferp);		
		if(ret != 0)
		{
			break;
		}
		ts_position += TS_PACKET_SIZE;
	}
	if(ret < 0)
	{
		memory_release();
		return -1;
	}
	else if(ret == 0)
	{
		memory_release();
		return -1;
	}
	
	ts_bufferp	= ts_memoryp;
	ts_position = 0;  
	m_ts_index = 0;
	while(1)
	{
		ts_bufferp = ts_memoryp + ts_position;
		if(ts_position + TS_PACKET_SIZE > ts_len)
		{
			break;
		}

		ret = ts_parse(ts_bufferp); 	
		if(ret != 0)
		{
			break;
		}	
		ts_position += TS_PACKET_SIZE;
	}	 
	if(m_audio_buffer.len != 0)
	{
		PES_T pes = {0};
		ts_parse_pes(m_audio_buffer.ptr, m_audio_buffer.len, &pes);
		PES_T* newp = pes_copy(&pes);
		dequeh_append(&m_audio_pes_deque, newp);
	}
	if(m_video_buffer.len != 0)
	{
		PES_T pes = {0};
		ts_parse_pes(m_video_buffer.ptr, m_video_buffer.len, &pes);
		PES_T* newp = pes_copy(&pes);
		dequeh_append(&m_video_pes_deque, newp);
	}

	pes_to_es();

	u_int8_t*	flv_bufferp 	= flv_memoryp;
	int 		flv_position	= 0;
	int 		flv_remain_size = flv_size;
	int 		flv_memo_size	= 0;
	//if(start_index == 0)
	{
	#if 0
		flv_bufferp = flv_memoryp + flv_position;		
		flv_memo_size = flv_memo_header(flv_bufferp, flv_remain_size, 1, 1);
		flv_position	+= flv_memo_size;
		flv_remain_size -= flv_memo_size;		
	#endif
		
		AUDIO_ES_T* audio_esp = (AUDIO_ES_T*)(m_audio_es_deque.headp->elementp);	
		VIDEO_ES_T* video_esp = (VIDEO_ES_T*)(m_video_es_deque.headp->elementp);	

		if(m_start_timestamp == 0)
		{
			if(audio_esp->pts <= video_esp->dts)
			{
				m_start_timestamp = audio_esp->pts;
			}
			else
			{
				m_start_timestamp = video_esp->dts;
			}
		}
		fprintf(stdout, "%s: m_start_timestamp=%lu \n", __FUNCTION__, m_start_timestamp);
		
		//m_start_timestamp = 0;
		
		flv_bufferp = flv_memoryp + flv_position;	
		flv_memo_size = flv_memo_aac_header(flv_bufferp, flv_remain_size, (audio_esp->pts-m_start_timestamp)/TS_FLV_TIME_RATE, &m_audio_config);
		flv_position	+= flv_memo_size;
		flv_remain_size -= flv_memo_size;	

		flv_bufferp = flv_memoryp + flv_position;
		flv_memo_size = flv_memo_avc_header(flv_bufferp, flv_remain_size, (video_esp->dts-m_start_timestamp)/TS_FLV_TIME_RATE, m_video_configp, m_video_config_pos);
		flv_position	+= flv_memo_size;
		flv_remain_size -= flv_memo_size;
	}

	flv_bufferp = flv_memoryp + flv_position;	
	flv_memo_size = flv_memo_es(flv_bufferp, flv_remain_size);
	flv_position	+= flv_memo_size;
	flv_remain_size -= flv_memo_size;
	
	memory_release();	
		
	return flv_position;
}


int TS2FLV::file2file(char * ts_file, char * flv_file)
{
	int ts_fd = open(ts_file, O_RDONLY);
	if(ts_fd == -1)
	{
		fprintf(stderr, "open %s error! \n", ts_file);	
		return -1;
	}
	long ts_len = lseek(ts_fd, 0, SEEK_END);
	if(ts_len == -1)
	{
		close(ts_fd);
		ts_fd = -1;
		return -1;
	}
	u_int8_t * ts_memoryp = (u_int8_t*)malloc(ts_len);
	if(ts_memoryp == NULL)
	{
		close(ts_fd);
		ts_fd = -1;
		return -1;
	}

	long flv_size = ts_len;
	u_int8_t * flv_memoryp = (u_int8_t*)malloc(flv_size);
	if(flv_memoryp == NULL)
	{
		free(ts_memoryp);
		ts_memoryp = NULL;
		
		close(ts_fd);
		ts_fd = -1;
		return -1;
	}

	lseek(ts_fd, 0, SEEK_SET);

	read(ts_fd, ts_memoryp, ts_len);
	close(ts_fd);

	int flv_len = buff2buff(ts_memoryp, ts_len, flv_memoryp, flv_size);

	int flv_fd = open(flv_file, O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
    if(flv_fd == -1)
    {
    	fprintf(stderr, "open %s error! \n", flv_file);	
    	free(ts_memoryp);
    	ts_memoryp = NULL;
    	free(flv_memoryp);
    	flv_memoryp = NULL;
        return -1;
    }

    write(flv_fd, flv_memoryp, flv_len);

    close(flv_fd);

    return 0;
}


int make_null_flv(char * flv_file)
{
	int ret = 0;
	int flv_fd = open(flv_file, O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
	if(flv_fd == -1)
	{
		fprintf(stderr, "open %s error! \n", flv_file); 
		return -1;
	}

	ret = flv_write_header(flv_fd, 1, 1);
	
	close(flv_fd);
	
	return ret;
}


