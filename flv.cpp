
#include <unistd.h>

#include "flv.h"

int flv_write_header(int fd)
{
	int ret = 0;
	
	FLV_HEADER header 	= {0};
	header.signature_f 	= 'F';
	header.signature_l 	= 'L';
	header.signature_v 	= 'V';
	header.version		= 0x01;

	header.typeflags_reserved5	= 0;
    header.typeflags_audio		= 0;
    header.typeflags_reserved1	= 0;
    header.typeflags_video		= 1;

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


int flv_write_video(int fd, u_int32_t timestamp, u_int8_t* datap, int data_len)
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
	video_tag.codec_id 		= 7;
	video_tag.frame_format 	= 1;

	len = sizeof(VIDEO_TAG);
	ret = write(fd, &video_tag, len);
	if(ret != len)
	{
		return -1;
	}

	AVC_CODEC_HEADER header = {0};
	header.avc_packet_type 			= AVC_NALU;
	header.composition_time23_16 	= 0;
	header.composition_time15_8 	= 0;
	header.composition_time7_0 		= 0;

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


