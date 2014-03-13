
#ifndef __FLV_H__
#define __FLV_H__

#include <sys/types.h>

#define TAG_TYPE_AUDIO 		8
#define TAG_TYPE_VIDEO 		9
#define TAG_TYPE_SCRIPT 	18

#define AAC_SEQENCE_HEADER  0
#define AAC_RAW_DATA        1

#define AVC_SEQ_HEADER   	0 // if codecid == 7 ,0=avc sequence header
#define AVC_NALU         	1 // if codecid == 7
#define AVC_END_SEQ      	2 // if codecid == 7 end of sequence (lower level NALU sequence ender si not required or  supoported)

// SoundFormat
#define AUDIO_SOUNDFORMAT_LINEAR_PCM_P  0 // platform endian
#define AUDIO_SOUNDFORMAT_ADPCM         1
#define AUDIO_SOUNDFORMAT_MP3           2
#define AUDIO_SOUNDFORMAT_LINEAR_PCM_L  3 // little endian
#define AUDIO_SOUNDFORMAT_16KHZ_MONO    4 // nellymoser 16KHZ mono
#define AUDIO_SOUNDFORMAT_8KHZ_MONO     5 // nellymoser 8KHZ mono
#define AUDIO_SOUNDFORMAT_NELLYMOSER    6
#define AUDIO_SOUNDFORMAT_G711_ALAW_PCM 7
#define AUDIO_SOUNDFORMAT_G711_MULAW_PCM 8
#define AUDIO_SOUNDFORMAT_RESERVED      9
#define AUDIO_SOUNDFORMAT_AAC           10
#define AUDIO_SOUNDFORMAT_SPEEX         11
#define AUDIO_SOUNDFORMAT_MP3_8KHZ      14
#define AUDIO_SOUNDFORMAT_DEVICE_SPECIFIC_SOUND 15

// soundRate
#define AUDIO_SOUNDRATE_5500HZ  0 // 5.5KHZ
#define AUDIO_SOUNDRATE_11KHZ   1 // 11KHZ
#define AUDIO_SOUNDRATE_22KHZ   2 // 22KHZ
#define AUDIO_SOUNDRATE_44KHZ   3 // 44KHZ

#define AUDIO_SOUNDSIZE_8   	0 // 8-bit samples
#define AUDIO_SOUNDSIZE_16  	1 // 16-bit samples

#define AUDIO_SOUNDTYPE_MONO    0 // Mono sound
#define AUDIO_SOUNDTYPE_STEREO  1 // STEREO sound

#define VIDEO_FRAMETYPE_KEY_FRAME   1 // for avc, a seekable frame
#define VIDEO_FRAMETYPE_INTER_FRAME 2 // for avc, a non-seekable frame
#define VIDEO_FRAMETYPE_DISPOSABLE_INTER_FARME 3 // H263 only 
#define VIDEO_FRAMETYPE_GENERATED_KEY_FRAME    4 // reserved for server use only
#define VIDEO_FRAMETYPE_VIDEO_INFO_COMMAND_FRAME   5 //


#define VIDEO_CODECID_SORNESON_H263 2
#define VIDEO_CODECID_SCREEN_VIDEO  3
#define VIDEO_CODECID_ON2_VP6       4
#define VIDEO_CODECID_ON2_VP6_ALPHA 5
#define VIDEO_CODECID_SCREEN_VIDEO_VERSION2 6
#define VIDEO_CODECID_AVC           7


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

typedef struct audio_tag
{
#  if __BYTE_ORDER == __LITTLE_ENDIAN
    u_int32_t sound_type			:1; // 0=Mono sound; 1= stereo sound
    u_int32_t sound_size			:1; // 0=8-bit samples; 1=16-bit samples
    u_int32_t sound_rate			:2; // 3=44kHz
    u_int32_t sound_format			:4; // 10 aac;
# elif __BYTE_ORDER == __BIG_ENDIAN   
    u_int32_t sound_format			:4; // 10 aac;
    u_int32_t sound_rate			:2; // 3=44kHz
    u_int32_t sound_size			:1; // 0=8-bit samples; 1=16-bit samples
    u_int32_t sound_type			:1; // 0=Mono sound; 1= stereo sound
# endif    
    //if sound_format== 10,the following values are defined: the first char is 
    // 0=AAC sequence header ; 1=AAC raw
    u_int8_t data[0]; // 
}AUDIO_TAG;

/*
  AudioSpecificConfig.audioObjectType = 2 (AAC LC) (5 bits)
    AudioSpecificConfig.samplingFrequencyIndex = 3 (4 bits)
    AudioSpecificConfig.channelConfiguration = 6 (4 bits)
    GASpecificConfig.frameLengthFlag = 0 (1 bit)
    GASpecificConfig.dependsOnCoreCoder = 0 (1 bit)
GASpecificConfig.extensionFlag = 0 (1 bit)
*/
typedef struct audio_specific_config
{
#if __BYTE_ORDER == __LITTLE_ENDIAN       
    u_int16_t   samplingFrequencyIndex_3_1	:3;
    u_int16_t   audioObjectType				:5; // 
    
    u_int16_t   extensionFlag				:1;
    u_int16_t   dependsOnCoreCoder			:1;
    u_int16_t   frameLengthFlag				:1;
    u_int16_t   channelConfiguration		:4;
    u_int16_t   samplingFrequencyIndex_0	:1;
    
#elif __BYTE_ORDER == __BIG_ENDIAN  
    u_int16_t   audioObjectType				:5; // 
    u_int16_t   samplingFrequencyIndex_3_1	:3;
    
    u_int16_t   samplingFrequencyIndex_0	:1;
    u_int16_t   channelConfiguration		:4;
    u_int16_t   frameLengthFlag				:1;
    u_int16_t   dependsOnCoreCoder			:1;
    u_int16_t   extensionFlag				:1;
#endif        
}AUDIO_SPECIFIC_CONFIG;


typedef struct video_tag
{
#if __BYTE_ORDER == __LITTLE_ENDIAN       
    u_int8_t codec_id				:4; // 7=avc
    u_int8_t frame_format			:4; // 1=key frame; 2=inter frame; 3=disposable interframe
#elif __BYTE_ORDER == __BIG_ENDIAN    
    u_int8_t frame_format			:4; // 1=key frame; 2=inter frame; 3=disposable interframe
    u_int8_t codec_id				:4; // 7=avc
#endif    
}VIDEO_TAG;


typedef struct avc_codec_header
{
    u_int32_t avc_packet_type		:8;
    u_int32_t composition_time23_16	:8;
    u_int32_t composition_time15_8	:8;
    u_int32_t composition_time7_0	:8;
}AVC_CODEC_HEADER;

/* 根据这篇文章 http://blog.csdn.net/k1988/article/details/5654631
aligned(8) class AVCDecoderConfigurationRecord 
{
    unsigned int(8) configurationVersion = 1;
    unsigned int(8) AVCProfileIndication;
    unsigned int(8) profile_compatibility;
    unsigned int(8) AVCLevelIndication;
    bit(6) reserved = ‘111111’b;      -------->(1)
    unsigned int(2) lengthSizeMinusOne;
    bit(3) reserved = ‘111’b;         -------->(2)
    unsigned int(5) numOfSequenceParameterSets;
    for (i=0; i< numOfSequenceParameterSets; i++)
    {
        unsigned int(16) sequenceParameterSetLength ;
        bit(8*sequenceParameterSetLength) sequenceParameterSetNALUnit;
    } 
    unsigned int(8) numOfPictureParameterSets;
    for (i=0; i< numOfPictureParameterSets; i++) 
    {
        unsigned int(16) pictureParameterSetLength;
        bit(8*pictureParameterSetLength) pictureParameterSetNALUnit;
    } 
}
*/

typedef struct parameter_set
{
	u_int32_t sequenceParameterSetLength_15_8	:8;
	u_int32_t sequenceParameterSetLength_7_0	:8;
	u_int8_t  sequenceParameterSetNALUnit[0];
} PARAMETER_SET;

typedef struct AVCDecoderConfigurationRecord
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
	u_int32_t configurationVersion			:8;
	u_int32_t AVCProfileIndication			:8;
	u_int32_t profile_compatibility			:8;
	u_int32_t AVCLevelIndication			:8;
	u_int32_t lengthSizeMinusOne			:2;
	u_int32_t reserved_1					:6;	
	u_int32_t numOfSequenceParameterSets	:5;
	u_int32_t reserved_2					:3;		
#elif __BYTE_ORDER == __BIG_ENDIAN
	u_int32_t configurationVersion			:8;
	u_int32_t AVCProfileIndication			:8;
	u_int32_t profile_compatibility			:8;
	u_int32_t AVCLevelIndication			:8;
	u_int32_t reserved_1					:6;
	u_int32_t lengthSizeMinusOne			:2;
	u_int32_t reserved_2					:3;
	u_int32_t numOfSequenceParameterSets	:5;	
#endif
} AVCDecoderConfigurationRecord;



#pragma pack()

int		flv_write_header(int fd, int has_audio, int has_video);
int 	flv_write_aac_header(int fd, u_int32_t timestamp, AUDIO_SPECIFIC_CONFIG* configp);
int 	flv_write_avc_header(int fd, u_int32_t timestamp, AVCDecoderConfigurationRecord* configp, int len);
int 	flv_write_audio(int fd, u_int32_t timestamp, u_int8_t* datap, int data_len);
int 	flv_write_video(int fd, u_int32_t timestamp, u_int32_t composition_timestamp, u_int8_t* datap, int data_len, u_int8_t key_frame);

int		flv_memo_header(u_int8_t* bufferp, int size, int has_audio, int has_video);
int 	flv_memo_aac_header(u_int8_t* bufferp, int size, u_int32_t timestamp, AUDIO_SPECIFIC_CONFIG* configp);
int 	flv_memo_avc_header(u_int8_t* bufferp, int size, u_int32_t timestamp, AVCDecoderConfigurationRecord* configp, int len);
int 	flv_memo_audio(u_int8_t* bufferp, int size, u_int32_t timestamp, u_int8_t* datap, int data_len);
int 	flv_memo_video(u_int8_t* bufferp, int size, u_int32_t timestamp, u_int32_t composition_timestamp, u_int8_t* datap, int data_len, u_int8_t key_frame);


#endif

