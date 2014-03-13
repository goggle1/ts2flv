
#ifndef __TS2FLV_H__
#define __TS2FLV_H__

#include "dequeH.h"
#include "buffer.h"
#include "TS.h"
#include "flv.h"

class TS2FLV
{
public: 
	TS2FLV();
	~TS2FLV();

	int 		file2file(char* ts_file, char* flv_file);
	int 		buff2buff(u_int8_t*	 ts_memoryp, int ts_len, u_int8_t* flv_memoryp, int flv_size);
	
protected:	
	int 		flv_memo_es(u_int8_t* flv_memoryp, int flv_size);
	int 		pes_to_es();
	
	int 		audio_pes2es(PES_T* pesp);
	int 		video_pes2es(PES_T* pesp);	
	int 		ts_find_pat_pmt(u_int8_t* ts_buffer);
	int 		ts_parse_pmt(u_int8_t* buffer, int len);
	int 		ts_parse_pat(u_int8_t* buffer, int len);
	int 		ts_parse(u_int8_t* ts_buffer);
	
	u_int32_t 	m_ts_index;
	
	u_int32_t 	m_pid_pmt;
	u_int32_t 	m_pid_network;

	u_int32_t 	m_pid_audio;
	u_int32_t 	m_pid_video;

	u_int32_t 	m_audio_stream_type;
	u_int32_t 	m_video_stream_type;

	DEQUEH_T	m_audio_pes_deque;
	DEQUEH_T	m_video_pes_deque;

	DEQUEH_T	m_audio_es_deque;
	DEQUEH_T	m_video_es_deque;

	BUFFER_T	m_pat_buffer;
	BUFFER_T	m_pmt_buffer;
	BUFFER_T	m_audio_buffer;
	BUFFER_T	m_video_buffer;

	AUDIO_SPECIFIC_CONFIG 			m_audio_config;
	u_int8_t						m_video_config_buffer[1024];
	int								m_video_config_pos;
	AVCDecoderConfigurationRecord* 	m_video_configp;
	
	u_int64_t						m_start_timestamp;
};

int 		make_null_flv(char* flv_file);

#endif
