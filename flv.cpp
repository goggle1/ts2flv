
#include <unistd.h>
#include <string.h>

#include "flv.h"

int flv_write_header(int fd, int has_audio, int has_video)
{
	int ret = 0;
	
	FLV_HEADER header 	= {0};
	header.signature_f 	= 'F';
	header.signature_l 	= 'L';
	header.signature_v 	= 'V';
	header.version		= 0x01;

	header.typeflags_reserved5	= 0;
    header.typeflags_audio		= has_audio?1:0;
    header.typeflags_reserved1	= 0;
    header.typeflags_video		= has_video?1:0;

	int header_len = sizeof(FLV_HEADER);
	u_int32_t length = header_len;
	header.header_len_31_24 = length>>24 & 0x000000FF;
	header.header_len_23_16 = length>>16 & 0x000000FF;
	header.header_len_15_8	= length>> 8 & 0x000000FF;
	header.header_len_7_0	= length>> 0 & 0x000000FF;

	ret = write(fd, &header, header_len);
	if(ret != header_len)
	{
		return -1;
	}

	u_int32_t previous_tag_size = 0;
	u_int8_t  temp[4] = {0};
	temp[0] 	= previous_tag_size>>24 & 0x000000FF;
	temp[1]		= previous_tag_size>>16 & 0x000000FF;
	temp[2]		= previous_tag_size>> 8 & 0x000000FF;
	temp[3]		= previous_tag_size>> 0 & 0x000000FF;
	ret = write(fd, &temp, 4);
	if(ret != 4)
	{
		return -1;
	}
	
	return 0;
}


int flv_memo_header(u_int8_t* buffer, int buffer_size, int has_audio, int has_video)
{
	int buffer_position = 0;
	
	FLV_HEADER header 	= {0};
	header.signature_f 	= 'F';
	header.signature_l 	= 'L';
	header.signature_v 	= 'V';
	header.version		= 0x01;

	header.typeflags_reserved5	= 0;
    header.typeflags_audio		= has_audio?1:0;
    header.typeflags_reserved1	= 0;
    header.typeflags_video		= has_video?1:0;

	int header_len = sizeof(FLV_HEADER);
	u_int32_t length = header_len;
	header.header_len_31_24 = length>>24 & 0x000000FF;
	header.header_len_23_16 = length>>16 & 0x000000FF;
	header.header_len_15_8	= length>> 8 & 0x000000FF;
	header.header_len_7_0	= length>> 0 & 0x000000FF;

	if(buffer_position + header_len > buffer_size)
	{
		return 0;
	}	
	memcpy(buffer+buffer_position, &header, header_len);	
	buffer_position += header_len;

	u_int32_t previous_tag_size = 0;
	u_int8_t  temp[4] = {0};
	temp[0] 	= previous_tag_size>>24 & 0x000000FF;
	temp[1]		= previous_tag_size>>16 & 0x000000FF;
	temp[2]		= previous_tag_size>> 8 & 0x000000FF;
	temp[3]		= previous_tag_size>> 0 & 0x000000FF;
	if(buffer_position + (int)sizeof(previous_tag_size) > buffer_size)
	{
		return 0;
	}	
	memcpy(buffer+buffer_position, temp, sizeof(previous_tag_size));	
	buffer_position += sizeof(previous_tag_size);
	
	return buffer_position;
}


int flv_write_aac_header(int fd, u_int32_t timestamp, AUDIO_SPECIFIC_CONFIG* configp)
{
	int ret = 0;

	FLV_TAG tag 	= {0};	
	tag.reserved2	= 0;
	tag.filter		= 0;
	tag.tag_type	= TAG_TYPE_AUDIO;

	u_int32_t data_size = sizeof(AUDIO_TAG) + sizeof(u_int8_t) + sizeof(AUDIO_SPECIFIC_CONFIG);
	tag.data_size_23_16 	= data_size >> 16 & 0x000000FF;
	tag.data_size_15_8		= data_size >>	8 & 0x000000FF;
	tag.data_size_7_0		= data_size >>	0 & 0x000000FF;

	//u_int32_t timestamp = 0;
	tag.timestamp_extended	= timestamp >> 24 & 0x000000FF;
	tag.timestamp_23_16 	= timestamp >> 16 & 0x000000FF;
	tag.timestamp_15_8		= timestamp >>	8 & 0x000000FF;
	tag.timestamp_7_0		= timestamp >>	0 & 0x000000FF;

	u_int32_t stream_id = 0;
	tag.stream_id_23_16 	= stream_id >> 16 & 0x000000FF;
	tag.stream_id_15_8		= stream_id >>	8 & 0x000000FF;
	tag.stream_id_7_0		= stream_id >>	0 & 0x000000FF;

	int len = 0;
	
	len = sizeof(FLV_TAG);
	ret = write(fd, &tag, len);
	if(ret != len)
	{
		return -1;
	}

	AUDIO_TAG audio_tag = {0};
	audio_tag.sound_format	= AUDIO_SOUNDFORMAT_AAC; 
	audio_tag.sound_rate	= AUDIO_SOUNDRATE_44KHZ;
	audio_tag.sound_size	= AUDIO_SOUNDSIZE_16;
	audio_tag.sound_type	= AUDIO_SOUNDTYPE_STEREO;

	len = sizeof(AUDIO_TAG);
	ret = write(fd, &audio_tag, len);
	if(ret != len)
	{
		return -1;
	}
	
	u_int8_t aac_data_type = AAC_SEQENCE_HEADER;

	len = sizeof(u_int8_t);
	ret = write(fd, &aac_data_type, len);
	if(ret != len)
	{
		return -1;
	}

	len = sizeof(AUDIO_SPECIFIC_CONFIG);
	ret = write(fd, configp, len);
	if(ret != len)
	{
		return -1;
	}

	u_int32_t previous_tag_size = data_size;
	u_int8_t  temp[4] = {0};
	temp[0] 	= previous_tag_size>>24 & 0x000000FF;
	temp[1] 	= previous_tag_size>>16 & 0x000000FF;
	temp[2] 	= previous_tag_size>> 8 & 0x000000FF;
	temp[3] 	= previous_tag_size>> 0 & 0x000000FF;
	ret = write(fd, &temp, 4);
	if(ret != 4)
	{
		return -1;
	}
	
	return 0;
}


int flv_memo_aac_header(u_int8_t* buffer, int buffer_size, u_int32_t timestamp, AUDIO_SPECIFIC_CONFIG* configp)
{
	int buffer_position = 0;
	
	FLV_TAG tag 	= {0};	
	tag.reserved2	= 0;
	tag.filter		= 0;
	tag.tag_type	= TAG_TYPE_AUDIO;

	u_int32_t data_size = sizeof(AUDIO_TAG) + sizeof(u_int8_t) + sizeof(AUDIO_SPECIFIC_CONFIG);
	tag.data_size_23_16 	= data_size >> 16 & 0x000000FF;
	tag.data_size_15_8		= data_size >>	8 & 0x000000FF;
	tag.data_size_7_0		= data_size >>	0 & 0x000000FF;

	//u_int32_t timestamp = 0;
	tag.timestamp_extended	= timestamp >> 24 & 0x000000FF;
	tag.timestamp_23_16 	= timestamp >> 16 & 0x000000FF;
	tag.timestamp_15_8		= timestamp >>	8 & 0x000000FF;
	tag.timestamp_7_0		= timestamp >>	0 & 0x000000FF;

	u_int32_t stream_id = 0;
	tag.stream_id_23_16 	= stream_id >> 16 & 0x000000FF;
	tag.stream_id_15_8		= stream_id >>	8 & 0x000000FF;
	tag.stream_id_7_0		= stream_id >>	0 & 0x000000FF;

	int len = 0;
	
	len = sizeof(FLV_TAG);
	if(buffer_position + len > buffer_size)
	{
		return 0;
	}	
	memcpy(buffer+buffer_position, &tag, len);	
	buffer_position += len;

	AUDIO_TAG audio_tag = {0};
	audio_tag.sound_format	= AUDIO_SOUNDFORMAT_AAC; 
	audio_tag.sound_rate	= AUDIO_SOUNDRATE_44KHZ;
	audio_tag.sound_size	= AUDIO_SOUNDSIZE_16;
	audio_tag.sound_type	= AUDIO_SOUNDTYPE_STEREO;

	len = sizeof(AUDIO_TAG);
	if(buffer_position + len > buffer_size)
	{
		return 0;
	}	
	memcpy(buffer+buffer_position, &audio_tag, len);	
	buffer_position += len;
	
	u_int8_t aac_data_type = AAC_SEQENCE_HEADER;

	len = sizeof(u_int8_t);
	if(buffer_position + len > buffer_size)
	{
		return 0;
	}	
	memcpy(buffer+buffer_position, &aac_data_type, len);	
	buffer_position += len;

	len = sizeof(AUDIO_SPECIFIC_CONFIG);
	if(buffer_position + len > buffer_size)
	{
		return 0;
	}	
	memcpy(buffer+buffer_position, configp, len);	
	buffer_position += len;

	u_int32_t previous_tag_size = data_size;
	u_int8_t  temp[4] = {0};
	temp[0] 	= previous_tag_size>>24 & 0x000000FF;
	temp[1] 	= previous_tag_size>>16 & 0x000000FF;
	temp[2] 	= previous_tag_size>> 8 & 0x000000FF;
	temp[3] 	= previous_tag_size>> 0 & 0x000000FF;
	len = sizeof(previous_tag_size);
	if(buffer_position + len > buffer_size)
	{
		return 0;
	}	
	memcpy(buffer+buffer_position, temp, len);	
	buffer_position += len;	
	
	return buffer_position;
}


int flv_write_audio(int fd, u_int32_t timestamp, u_int8_t* datap, int data_len)
{
	int ret = 0;

	FLV_TAG tag 	= {0};	
	tag.reserved2	= 0;
	tag.filter		= 0;
	tag.tag_type	= TAG_TYPE_AUDIO;

	u_int32_t data_size = sizeof(AUDIO_TAG) + sizeof(u_int8_t) + data_len;
	tag.data_size_23_16 	= data_size >> 16 & 0x000000FF;
	tag.data_size_15_8		= data_size >>	8 & 0x000000FF;
	tag.data_size_7_0		= data_size >>	0 & 0x000000FF;

	//u_int32_t timestamp = 0;
	tag.timestamp_extended	= timestamp >> 24 & 0x000000FF;
	tag.timestamp_23_16 	= timestamp >> 16 & 0x000000FF;
	tag.timestamp_15_8		= timestamp >>	8 & 0x000000FF;
	tag.timestamp_7_0		= timestamp >>	0 & 0x000000FF;

	u_int32_t stream_id = 0;
	tag.stream_id_23_16 	= stream_id >> 16 & 0x000000FF;
	tag.stream_id_15_8		= stream_id >>	8 & 0x000000FF;
	tag.stream_id_7_0		= stream_id >>	0 & 0x000000FF;

	int len = 0;
	
	len = sizeof(FLV_TAG);
	ret = write(fd, &tag, len);
	if(ret != len)
	{
		return -1;
	}

	AUDIO_TAG audio_tag = {0};
	audio_tag.sound_format	= AUDIO_SOUNDFORMAT_AAC; 
	audio_tag.sound_rate	= AUDIO_SOUNDRATE_44KHZ;
	audio_tag.sound_size	= AUDIO_SOUNDSIZE_16;
	audio_tag.sound_type	= AUDIO_SOUNDTYPE_STEREO;

	len = sizeof(AUDIO_TAG);
	ret = write(fd, &audio_tag, len);
	if(ret != len)
	{
		return -1;
	}
		
	u_int8_t aac_data_type = AAC_RAW_DATA;

	len = sizeof(u_int8_t);
	ret = write(fd, &aac_data_type, len);
	if(ret != len)
	{
		return -1;
	}

	len = data_len;
	ret = write(fd, datap, len);
	if(ret != len)
	{
		return -1;
	}

	u_int32_t previous_tag_size = data_size;
	u_int8_t  temp[4] = {0};
	temp[0] 	= previous_tag_size>>24 & 0x000000FF;
	temp[1] 	= previous_tag_size>>16 & 0x000000FF;
	temp[2] 	= previous_tag_size>> 8 & 0x000000FF;
	temp[3] 	= previous_tag_size>> 0 & 0x000000FF;
	ret = write(fd, &temp, 4);
	if(ret != 4)
	{
		return -1;
	}
	
	return 0;
}


int flv_memo_audio(u_int8_t* buffer, int buffer_size, u_int32_t timestamp, u_int8_t* datap, int data_len)
{
	int buffer_position = 0;
	
	FLV_TAG tag 	= {0};	
	tag.reserved2	= 0;
	tag.filter		= 0;
	tag.tag_type	= TAG_TYPE_AUDIO;

	u_int32_t data_size = sizeof(AUDIO_TAG) + sizeof(u_int8_t) + data_len;
	tag.data_size_23_16 	= data_size >> 16 & 0x000000FF;
	tag.data_size_15_8		= data_size >>	8 & 0x000000FF;
	tag.data_size_7_0		= data_size >>	0 & 0x000000FF;

	//u_int32_t timestamp = 0;
	tag.timestamp_extended	= timestamp >> 24 & 0x000000FF;
	tag.timestamp_23_16 	= timestamp >> 16 & 0x000000FF;
	tag.timestamp_15_8		= timestamp >>	8 & 0x000000FF;
	tag.timestamp_7_0		= timestamp >>	0 & 0x000000FF;

	u_int32_t stream_id = 0;
	tag.stream_id_23_16 	= stream_id >> 16 & 0x000000FF;
	tag.stream_id_15_8		= stream_id >>	8 & 0x000000FF;
	tag.stream_id_7_0		= stream_id >>	0 & 0x000000FF;

	int len = 0;
	
	len = sizeof(FLV_TAG);	
	if(buffer_position + len > buffer_size)
	{
		return 0;
	}	
	memcpy(buffer+buffer_position, &tag, len);	
	buffer_position += len;

	AUDIO_TAG audio_tag = {0};
	audio_tag.sound_format	= AUDIO_SOUNDFORMAT_AAC; 
	audio_tag.sound_rate	= AUDIO_SOUNDRATE_44KHZ;
	audio_tag.sound_size	= AUDIO_SOUNDSIZE_16;
	audio_tag.sound_type	= AUDIO_SOUNDTYPE_STEREO;

	len = sizeof(AUDIO_TAG);
	if(buffer_position + len > buffer_size)
	{
		return 0;
	}	
	memcpy(buffer+buffer_position, &audio_tag, len);	
	buffer_position += len;
			
	u_int8_t aac_data_type = AAC_RAW_DATA;

	len = sizeof(u_int8_t);
	if(buffer_position + len > buffer_size)
	{
		return 0;
	}	
	memcpy(buffer+buffer_position, &aac_data_type, len);	
	buffer_position += len;

	len = data_len;
	if(buffer_position + len > buffer_size)
	{
		return 0;
	}	
	memcpy(buffer+buffer_position, datap, len);	
	buffer_position += len;

	u_int32_t previous_tag_size = data_size;
	u_int8_t  temp[4] = {0};
	temp[0] 	= previous_tag_size>>24 & 0x000000FF;
	temp[1] 	= previous_tag_size>>16 & 0x000000FF;
	temp[2] 	= previous_tag_size>> 8 & 0x000000FF;
	temp[3] 	= previous_tag_size>> 0 & 0x000000FF;
	len = sizeof(previous_tag_size);
	if(buffer_position + len > buffer_size)
	{
		return 0;
	}	
	memcpy(buffer+buffer_position, temp, len);	
	buffer_position += len;
	
	return buffer_position;
}




int flv_write_avc_header(int fd, u_int32_t timestamp, AVCDecoderConfigurationRecord* configp, int data_len)
{
	int ret = 0;
	
	FLV_TAG tag 	= {0};	
	tag.reserved2 	= 0;
    tag.filter		= 0;
    tag.tag_type	= TAG_TYPE_VIDEO;

	u_int32_t data_size = sizeof(VIDEO_TAG) + sizeof(AVC_CODEC_HEADER) + data_len;
    tag.data_size_23_16		= data_size >> 16 & 0x000000FF;
    tag.data_size_15_8		= data_size >>  8 & 0x000000FF;
    tag.data_size_7_0		= data_size >>  0 & 0x000000FF;

	//u_int32_t timestamp = 0;
	tag.timestamp_extended	= timestamp >> 24 & 0x000000FF;
	tag.timestamp_23_16		= timestamp >> 16 & 0x000000FF;
    tag.timestamp_15_8		= timestamp >>  8 & 0x000000FF;
    tag.timestamp_7_0		= timestamp >>  0 & 0x000000FF;

	u_int32_t stream_id = 0;
    tag.stream_id_23_16		= stream_id >> 16 & 0x000000FF;
    tag.stream_id_15_8		= stream_id >>  8 & 0x000000FF;
    tag.stream_id_7_0		= stream_id >>  0 & 0x000000FF;

	int len = 0;
	
	len = sizeof(FLV_TAG);
	ret = write(fd, &tag, len);
	if(ret != len)
	{
		return -1;
	}

	VIDEO_TAG video_tag = {0};
	video_tag.codec_id 		= VIDEO_CODECID_AVC;
	video_tag.frame_format 	= VIDEO_FRAMETYPE_KEY_FRAME;

	len = sizeof(VIDEO_TAG);
	ret = write(fd, &video_tag, len);
	if(ret != len)
	{
		return -1;
	}

	u_int32_t composition_timestamp = 0;
	AVC_CODEC_HEADER header = {0};
	header.avc_packet_type 			= AVC_SEQ_HEADER;
	header.composition_time23_16 	= composition_timestamp >> 16 & 0x000000FF;
	header.composition_time15_8 	= composition_timestamp >>  8 & 0x000000FF;
	header.composition_time7_0 		= composition_timestamp >>  0 & 0x000000FF;

	len = sizeof(AVC_CODEC_HEADER);
	ret = write(fd, &header, len);
	if(ret != len)
	{
		return -1;
	}

	len = data_len;
	ret = write(fd, configp, len);
	if(ret != len)
	{
		return -1;
	}

	u_int32_t previous_tag_size = data_size;
	u_int8_t  temp[4] = {0};
	temp[0] 	= previous_tag_size>>24 & 0x000000FF;
	temp[1]		= previous_tag_size>>16 & 0x000000FF;
	temp[2]		= previous_tag_size>> 8 & 0x000000FF;
	temp[3]		= previous_tag_size>> 0 & 0x000000FF;
	ret = write(fd, &temp, 4);
	if(ret != 4)
	{
		return -1;
	}
	
	return 0;
}


int flv_memo_avc_header(u_int8_t* buffer, int buffer_size, u_int32_t timestamp, AVCDecoderConfigurationRecord* configp, int data_len)
{
	int buffer_position = 0;
	
	FLV_TAG tag 	= {0};	
	tag.reserved2 	= 0;
    tag.filter		= 0;
    tag.tag_type	= TAG_TYPE_VIDEO;

	u_int32_t data_size = sizeof(VIDEO_TAG) + sizeof(AVC_CODEC_HEADER) + data_len;
    tag.data_size_23_16		= data_size >> 16 & 0x000000FF;
    tag.data_size_15_8		= data_size >>  8 & 0x000000FF;
    tag.data_size_7_0		= data_size >>  0 & 0x000000FF;

	//u_int32_t timestamp = 0;
	tag.timestamp_extended	= timestamp >> 24 & 0x000000FF;
	tag.timestamp_23_16		= timestamp >> 16 & 0x000000FF;
    tag.timestamp_15_8		= timestamp >>  8 & 0x000000FF;
    tag.timestamp_7_0		= timestamp >>  0 & 0x000000FF;

	u_int32_t stream_id = 0;
    tag.stream_id_23_16		= stream_id >> 16 & 0x000000FF;
    tag.stream_id_15_8		= stream_id >>  8 & 0x000000FF;
    tag.stream_id_7_0		= stream_id >>  0 & 0x000000FF;

	int len = 0;
	
	len = sizeof(FLV_TAG);
	if(buffer_position + len > buffer_size)
	{
		return 0;
	}	
	memcpy(buffer+buffer_position, &tag, len);	
	buffer_position += len;

	VIDEO_TAG video_tag = {0};
	video_tag.codec_id 		= VIDEO_CODECID_AVC;
	video_tag.frame_format 	= VIDEO_FRAMETYPE_KEY_FRAME;

	len = sizeof(VIDEO_TAG);
	if(buffer_position + len > buffer_size)
	{
		return 0;
	}	
	memcpy(buffer+buffer_position, &video_tag, len);	
	buffer_position += len;

	u_int32_t composition_timestamp = 0;
	AVC_CODEC_HEADER header = {0};
	header.avc_packet_type 			= AVC_SEQ_HEADER;
	header.composition_time23_16 	= composition_timestamp >> 16 & 0x000000FF;
	header.composition_time15_8 	= composition_timestamp >>  8 & 0x000000FF;
	header.composition_time7_0 		= composition_timestamp >>  0 & 0x000000FF;

	len = sizeof(AVC_CODEC_HEADER);
	if(buffer_position + len > buffer_size)
	{
		return 0;
	}	
	memcpy(buffer+buffer_position, &header, len);	
	buffer_position += len;
	
	len = data_len;
	if(buffer_position + len > buffer_size)
	{
		return 0;
	}	
	memcpy(buffer+buffer_position, configp, len);	
	buffer_position += len;

	u_int32_t previous_tag_size = data_size;
	u_int8_t  temp[4] = {0};
	temp[0] 	= previous_tag_size>>24 & 0x000000FF;
	temp[1]		= previous_tag_size>>16 & 0x000000FF;
	temp[2]		= previous_tag_size>> 8 & 0x000000FF;
	temp[3]		= previous_tag_size>> 0 & 0x000000FF;
	len = sizeof(previous_tag_size);
	if(buffer_position + len > buffer_size)
	{
		return 0;
	}	
	memcpy(buffer+buffer_position, temp, len);	
	buffer_position += len;
	
	return buffer_position;
}


int flv_write_video(int fd, u_int32_t timestamp, u_int32_t composition_timestamp, u_int8_t* datap, int data_len, u_int8_t key_frame)
{
	int ret = 0;
	
	FLV_TAG tag 	= {0};	
	tag.reserved2 	= 0;
    tag.filter		= 0;
    tag.tag_type	= TAG_TYPE_VIDEO;

	u_int32_t data_size = sizeof(VIDEO_TAG) + sizeof(AVC_CODEC_HEADER) + data_len;
    tag.data_size_23_16		= data_size >> 16 & 0x000000FF;
    tag.data_size_15_8		= data_size >>  8 & 0x000000FF;
    tag.data_size_7_0		= data_size >>  0 & 0x000000FF;

	//u_int32_t timestamp = 0;
	tag.timestamp_extended	= timestamp >> 24 & 0x000000FF;
	tag.timestamp_23_16		= timestamp >> 16 & 0x000000FF;
    tag.timestamp_15_8		= timestamp >>  8 & 0x000000FF;
    tag.timestamp_7_0		= timestamp >>  0 & 0x000000FF;

	u_int32_t stream_id = 0;
    tag.stream_id_23_16		= stream_id >> 16 & 0x000000FF;
    tag.stream_id_15_8		= stream_id >>  8 & 0x000000FF;
    tag.stream_id_7_0		= stream_id >>  0 & 0x000000FF;

	int len = 0;
	
	len = sizeof(FLV_TAG);
	ret = write(fd, &tag, len);
	if(ret != len)
	{
		return -1;
	}

	VIDEO_TAG video_tag = {0};
	video_tag.codec_id 		= VIDEO_CODECID_AVC;
	video_tag.frame_format 	= key_frame?VIDEO_FRAMETYPE_KEY_FRAME : VIDEO_FRAMETYPE_INTER_FRAME;

	len = sizeof(VIDEO_TAG);
	ret = write(fd, &video_tag, len);
	if(ret != len)
	{
		return -1;
	}

	AVC_CODEC_HEADER header = {0};
	header.avc_packet_type 			= AVC_NALU;
	header.composition_time23_16 	= composition_timestamp >> 16 & 0x000000FF;
	header.composition_time15_8 	= composition_timestamp >>  8 & 0x000000FF;
	header.composition_time7_0 		= composition_timestamp >>  0 & 0x000000FF;

	len = sizeof(AVC_CODEC_HEADER);
	ret = write(fd, &header, len);
	if(ret != len)
	{
		return -1;
	}

	len = data_len;
	ret = write(fd, datap, len);
	if(ret != len)
	{
		return -1;
	}

	u_int32_t previous_tag_size = data_size;
	u_int8_t  temp[4] = {0};
	temp[0] 	= previous_tag_size>>24 & 0x000000FF;
	temp[1]		= previous_tag_size>>16 & 0x000000FF;
	temp[2]		= previous_tag_size>> 8 & 0x000000FF;
	temp[3]		= previous_tag_size>> 0 & 0x000000FF;
	ret = write(fd, &temp, 4);
	if(ret != 4)
	{
		return -1;
	}
	
	return 0;
}


int flv_memo_video(u_int8_t* buffer, int buffer_size, u_int32_t timestamp, u_int32_t composition_timestamp, u_int8_t* datap, int data_len, u_int8_t key_frame)
{
	int buffer_position = 0;
	
	FLV_TAG tag 	= {0};	
	tag.reserved2 	= 0;
    tag.filter		= 0;
    tag.tag_type	= TAG_TYPE_VIDEO;

	u_int32_t data_size = sizeof(VIDEO_TAG) + sizeof(AVC_CODEC_HEADER) + data_len;
    tag.data_size_23_16		= data_size >> 16 & 0x000000FF;
    tag.data_size_15_8		= data_size >>  8 & 0x000000FF;
    tag.data_size_7_0		= data_size >>  0 & 0x000000FF;

	//u_int32_t timestamp = 0;
	tag.timestamp_extended	= timestamp >> 24 & 0x000000FF;
	tag.timestamp_23_16		= timestamp >> 16 & 0x000000FF;
    tag.timestamp_15_8		= timestamp >>  8 & 0x000000FF;
    tag.timestamp_7_0		= timestamp >>  0 & 0x000000FF;

	u_int32_t stream_id = 0;
    tag.stream_id_23_16		= stream_id >> 16 & 0x000000FF;
    tag.stream_id_15_8		= stream_id >>  8 & 0x000000FF;
    tag.stream_id_7_0		= stream_id >>  0 & 0x000000FF;

	int len = 0;
	
	len = sizeof(FLV_TAG);
	if(buffer_position + len > buffer_size)
	{
		return 0;
	}	
	memcpy(buffer+buffer_position, &tag, len);	
	buffer_position += len;
	
	VIDEO_TAG video_tag = {0};
	video_tag.codec_id 		= VIDEO_CODECID_AVC;
	video_tag.frame_format 	= key_frame?VIDEO_FRAMETYPE_KEY_FRAME : VIDEO_FRAMETYPE_INTER_FRAME;

	len = sizeof(VIDEO_TAG);
	if(buffer_position + len > buffer_size)
	{
		return 0;
	}	
	memcpy(buffer+buffer_position, &video_tag, len);	
	buffer_position += len;

	AVC_CODEC_HEADER header = {0};
	header.avc_packet_type 			= AVC_NALU;
	header.composition_time23_16 	= composition_timestamp >> 16 & 0x000000FF;
	header.composition_time15_8 	= composition_timestamp >>  8 & 0x000000FF;
	header.composition_time7_0 		= composition_timestamp >>  0 & 0x000000FF;

	len = sizeof(AVC_CODEC_HEADER);
	if(buffer_position + len > buffer_size)
	{
		return 0;
	}	
	memcpy(buffer+buffer_position, &header, len);	
	buffer_position += len;

	len = data_len;
	if(buffer_position + len > buffer_size)
	{
		return 0;
	}	
	memcpy(buffer+buffer_position, datap, len);	
	buffer_position += len;

	u_int32_t previous_tag_size = data_size;
	u_int8_t  temp[4] = {0};
	temp[0] 	= previous_tag_size>>24 & 0x000000FF;
	temp[1]		= previous_tag_size>>16 & 0x000000FF;
	temp[2]		= previous_tag_size>> 8 & 0x000000FF;
	temp[3]		= previous_tag_size>> 0 & 0x000000FF;
	len = sizeof(previous_tag_size);
	if(buffer_position + len > buffer_size)
	{
		return 0;
	}	
	memcpy(buffer+buffer_position, temp, len);	
	buffer_position += len;
	
	return buffer_position;
}


