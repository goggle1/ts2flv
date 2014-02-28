
#ifndef __FLV_H__
#define __FLV_H__

#include <sys/types.h>

#define TAG_TYPE_AUDIO 	8
#define TAG_TYPE_VIDEO 	9
#define TAG_TYPE_SCRIPT 18

#define AVC_SEQ_HEADER   0 // if codecid == 7 ,0=avc sequence header
#define AVC_NALU         1 // if codecid == 7
#define AVC_END_SEQ      2 // if codecid == 7 end of sequence (lower level NALU sequence ender si not required or  supoported)

#pragma pack(1)

typedef struct flv_header
{
#if __BYTE_ORDER == __LITTLE_ENDIAN  
    u_int32_t signature_f			:8;
    u_int32_t signature_l			:8;
    u_int32_t signature_v			:8;
    u_int32_t version				:8;
   
    u_int32_t typeflags_video		:1;
    u_int32_t typeflags_reserved1	:1;
    u_int32_t typeflags_audio		:1;
    u_int32_t typeflags_reserved5	:5;

    //u_int32_t header_len; /* the length of this header in bytes*/
    u_int32_t   header_len_31_24	:8;
    u_int32_t   header_len_23_16	:8;
    u_int32_t   header_len_15_8		:8;
    u_int32_t   header_len_7_0		:8;
#elif __BYTE_ORDER == __BIG_ENDIAN        
    u_int32_t signature_f			:8;
    u_int32_t signature_l			:8;
    u_int32_t signature_v			:8;
    u_int32_t version				:8;
     
    u_int32_t typeflags_reserved5	:5;
    u_int32_t typeflags_audio		:1;
    u_int32_t typeflags_reserved1	:1;
    u_int32_t typeflags_video		:1;
    
    //u_int32_t header_len; /* the length of this header in bytes*/
    u_int32_t   header_len_31_24	:8;
    u_int32_t   header_len_23_16	:8;
    u_int32_t   header_len_15_8		:8;
    u_int32_t   header_len_7_0		:8;    
#endif
} FLV_HEADER;

typedef struct flv_tag
{
#if __BYTE_ORDER == __LITTLE_ENDIAN  
    u_int32_t tag_type				:5;	// 8:audio tag; 9:video tag; 18:script tag
    u_int32_t filter				:1;	// indicates if packets are filtered
    u_int32_t reserved2				:2;
    
    u_int32_t data_size_23_16		:8;
    u_int32_t data_size_15_8		:8;
    u_int32_t data_size_7_0			:8;

    u_int32_t timestamp_23_16		:8;
    u_int32_t timestamp_15_8		:8;
    u_int32_t timestamp_7_0			:8;
    u_int32_t timestamp_extended	:8;

    // stream_id:24 always 0
    u_int32_t stream_id_23_16		:8;
    u_int32_t stream_id_15_8		:8;
    u_int32_t stream_id_7_0			:8;
#elif __BYTE_ORDER == __BIG_ENDIAN 
    u_int32_t reserved2				:2;
    u_int32_t filter				:1;	// indicates if packets are filtered
    u_int32_t tag_type				:5; // 8:audio tag; 9:video tag; 18:script tag

    u_int32_t data_size_23_16		:8;
    u_int32_t data_size_15_8		:8;
    u_int32_t data_size_7_0			:8;

    u_int32_t timestamp_23_16		:8;
    u_int32_t timestamp_15_8		:8;
    u_int32_t timestamp_7_0			:8;
    u_int32_t timestamp_extended	:8;

    // stream_id:24 always 0
    u_int32_t stream_id_23_16		:8;
    u_int32_t stream_id_15_8		:8;
    u_int32_t stream_id_7_0			:8;
#endif
} FLV_TAG;

typedef struct video_tag
{
#  if __BYTE_ORDER == __LITTLE_ENDIAN       
    u_int8_t codec_id				:4; // 7=avc
    u_int8_t frame_format			:4; // 1=key frame; 2=inter frame; 3=disposable interframe
# elif __BYTE_ORDER == __BIG_ENDIAN    
    u_int8_t frame_format			:4; // 1=key frame; 2=inter frame; 3=disposable interframe
    u_int8_t codec_id				:4; // 7=avc
# endif    
}VIDEO_TAG;

typedef struct avc_codec_header
{
    u_int32_t avc_packet_type:8;
    u_int32_t composition_time23_16:8;
    u_int32_t composition_time15_8:8;
    u_int32_t composition_time7_0:8;
}AVC_CODEC_HEADER;


#pragma pack()

int		flv_write_header(int fd);
int 	flv_write_video(int fd, u_int32_t timestamp, u_int8_t* datap, int data_len);


#endif

