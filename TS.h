#ifndef __TS_H__
#define __TS_H__

#include <sys/types.h>

#define TS_PACKET_SIZE	188

#define STREAM_AUDIO_AAC	0x0F
#define STREAM_AUDIO_MP3	0x03
#define STREAM_VIDEO_H264	0x1b

#define NALU_TYPE_SLICE 1
#define NALU_TYPE_DPA 2
#define NALU_TYPE_DPB 3
#define NALU_TYPE_DPC 4
#define NALU_TYPE_IDR 5
#define NALU_TYPE_SEI 6
#define NALU_TYPE_SPS 7
#define NALU_TYPE_PPS 8
#define NALU_TYPE_AUD 9  //?????
#define NALU_TYPE_EOSEQ 10
#define NALU_TYPE_EOSTREAM 11
#define NALU_TYPE_FILL 12


#pragma pack(1)

typedef struct ts_header
{
#if __BYTE_ORDER == __LITTLE_ENDIAN       
	u_int32_t sync_byte                    :8; 
	
	u_int32_t PID_12_8                     :5;
	u_int32_t transport_priority           :1; 
	u_int32_t payload_unit_start_indicator :1;
	u_int32_t transport_error_indicator    :1;	
	
	u_int32_t PID_7_0                      :8;

	u_int32_t continuity_counter           :4;
	u_int32_t adaption_field_control       :2; 
	u_int32_t transport_scrambling_control :2; 		
#elif __BYTE_ORDER == __BIG_ENDIAN 
	u_int32_t sync_byte                    :8;   

	u_int32_t transport_error_indicator    :1;      
	u_int32_t payload_unit_start_indicator :1;               
	u_int32_t transport_priority           :1;      
	//u_int32_t PID                          :13;     
	u_int32_t PID_12_8                     :5;

	u_int32_t PID_7_0                      :8;

	u_int32_t transport_scrambling_control :2;      
	u_int32_t adaption_field_control       :2;      
	u_int32_t continuity_counter           :4;
#endif

} TS_HEADER;

typedef struct program_t
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
	u_int32_t program_number_15_8			 : 8;
	u_int32_t program_number_7_0			 : 8;
	u_int32_t network_PID_12_8				 : 5;
	u_int32_t reserved_3 					 : 3;		
	u_int32_t network_PID_7_0				 : 8;
#elif __BYTE_ORDER == __BIG_ENDIAN 
	u_int32_t program_number_15_8			 : 8;
	u_int32_t program_number_7_0			 : 8;
	u_int32_t reserved_3 					 : 3;
	u_int32_t network_PID_12_8				 : 5;	
	u_int32_t network_PID_7_0				 : 8;
#endif

} PROGRAM_T;

// Program Association Table
typedef struct pat_packet
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
	u_int32_t table_id                        : 8; 
	u_int32_t section_length_11_8             : 4;
	u_int32_t reserved_1                      : 2;
	u_int32_t zero                            : 1;
	u_int32_t section_syntax_indicator        : 1; 
	u_int32_t section_length_7_0              : 8;
	u_int32_t transport_stream_id_15_8        : 8;
	u_int32_t transport_stream_id_7_0         : 8;
	u_int32_t current_next_indicator          : 1; 
	u_int32_t version_number                  : 5; 
	u_int32_t reserved_2                      : 2; 
	u_int32_t section_number                  : 8; 
	u_int32_t last_section_number             : 8; 
	PROGRAM_T programs[0];
	// u_int32_t	crc32;
#elif __BYTE_ORDER == __BIG_ENDIAN 
	u_int32_t table_id                        : 8; 
	u_int32_t section_syntax_indicator        : 1; 
	u_int32_t zero                            : 1; 
	u_int32_t reserved_1                      : 2; 
	u_int32_t section_length_11_8             : 4;
	u_int32_t section_length_7_0              : 8;
	u_int32_t transport_stream_id_15_8        : 8;
	u_int32_t transport_stream_id_7_0         : 8;
	u_int32_t reserved_2                      : 2; 
	u_int32_t version_number                  : 5; 
	u_int32_t current_next_indicator          : 1; 
	u_int32_t section_number                  : 8; 
	u_int32_t last_section_number             : 8; 
	PROGRAM_T programs[0];
	// u_int32_t	crc32;
#endif

} PAT_PACKET;

typedef struct stream_t
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
	u_int32_t stream_type                     : 8;
	u_int32_t elementary_PID_12_8             : 5;
	u_int32_t reserved_5                      : 3;	
	u_int32_t elementary_PID_7_0              : 8;
	u_int32_t ES_info_length_11_8             : 4;
	u_int32_t reserved_6                      : 4;		
	u_int32_t ES_info_length_7_0              : 8;	
#elif __BYTE_ORDER == __BIG_ENDIAN 
	u_int32_t stream_type                     : 8;
	u_int32_t reserved_5                      : 3;
	u_int32_t elementary_PID_12_8             : 5;
	u_int32_t elementary_PID_7_0              : 8;
	u_int32_t reserved_6                      : 4;
	u_int32_t ES_info_length_11_8             : 4;	
	u_int32_t ES_info_length_7_0              : 8;	
#endif

} STREAM_T;

// Program Map Table
typedef struct pmt_packet
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
	u_int32_t table_id                        : 8;
	u_int32_t section_length_11_8             : 4;
	u_int32_t reserved_1                      : 2;
	u_int32_t zero                            : 1;
	u_int32_t section_syntax_indicator        : 1;
	u_int32_t section_length_7_0              : 8;
	u_int32_t program_number_15_8             : 8;
	u_int32_t program_number_7_0              : 8;
	u_int32_t current_next_indicator          : 1;
	u_int32_t version_number                  : 5;
	u_int32_t reserved_2                      : 2;
	u_int32_t section_number                  : 8;
	u_int32_t last_section_number             : 8;
	u_int32_t PCR_PID_12_8                    : 5;
	u_int32_t reserved_3                      : 3;	
	u_int32_t PCR_PID_7_0                     : 8;
	u_int32_t program_info_length_11_8        : 4;
	u_int32_t reserved_4                      : 4;	
	u_int32_t program_info_length_7_0         : 8;
	//STREAM_T	components[0];
	//u_int32_t CRC_32                          : 32;
#elif __BYTE_ORDER == __BIG_ENDIAN
	u_int32_t table_id                        : 8;
	u_int32_t section_syntax_indicator        : 1;
	u_int32_t zero                            : 1;
	u_int32_t reserved_1                      : 2;
	u_int32_t section_length_11_8             : 4;
	u_int32_t section_length_7_0              : 8;
	u_int32_t program_number_15_8             : 8;
	u_int32_t program_number_7_0              : 8;
	u_int32_t reserved_2                      : 2;
	u_int32_t version_number                  : 5;
	u_int32_t current_next_indicator          : 1;
	u_int32_t section_number                  : 8;
	u_int32_t last_section_number             : 8;
	u_int32_t reserved_3                      : 3;
	u_int32_t PCR_PID_12_8                    : 5;
	u_int32_t PCR_PID_7_0                     : 8;
	u_int32_t reserved_4                      : 4;
	u_int32_t program_info_length_11_8        : 4;
	u_int32_t program_info_length_7_0         : 8;
	//STREAM_T	components[0];
	//u_int32_t CRC_32                          : 32;
#endif

} PMT_PACKET;

typedef struct program_clock_reference
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
	u_int32_t program_clock_reference_base_32_25	:8;
	u_int32_t program_clock_reference_base_24_17	:8;
	u_int32_t program_clock_reference_base_16_9 	:8;
	u_int32_t program_clock_reference_base_8_1		:8;
	u_int32_t program_clock_reference_extension_8_8	:1;
	u_int32_t reserved								:6;
	u_int32_t program_clock_reference_base_0		:1;
	u_int32_t program_clock_reference_extension_7_0 :8;
	
#elif __BYTE_ORDER == __BIG_ENDIAN
	u_int32_t program_clock_reference_base_32_25	:8;
	u_int32_t program_clock_reference_base_24_17	:8;
	u_int32_t program_clock_reference_base_16_9		:8;
	u_int32_t program_clock_reference_base_8_1		:8;
	u_int32_t program_clock_reference_base_0 		:1;
	u_int32_t reserved 								:6;
	u_int32_t program_clock_reference_extension_8_8	:1;
	u_int32_t program_clock_reference_extension_7_0	:8;
#endif

}PROGRAM_CLOCK_REFERENCE ;

typedef struct adaptation_field
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
	u_int32_t adaptation_field_length 				:8;
	u_int32_t adaptation_field_extension_flag 		:1;
	u_int32_t transport_private_data_flag 			:1;
	u_int32_t splicing_point_flag 					:1;
	u_int32_t OPCR_flag 							:1;
	u_int32_t PCR_flag 								:1;
	u_int32_t elementary_stream_priority_indicator 	:1;
	u_int32_t random_access_indicator 				:1;	
	u_int32_t discontinuity_indicator 				:1;	
#elif __BYTE_ORDER == __BIG_ENDIAN
	u_int32_t adaptation_field_length 				:8;
	u_int32_t discontinuity_indicator 				:1;
	u_int32_t random_access_indicator 				:1;
	u_int32_t elementary_stream_priority_indicator 	:1;
	u_int32_t PCR_flag 								:1;
	u_int32_t OPCR_flag 							:1;
	u_int32_t splicing_point_flag 					:1;
	u_int32_t transport_private_data_flag 			:1;
	u_int32_t adaptation_field_extension_flag 		:1;
#endif

} ADAPTATION_FIELD;


typedef struct pes_header
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
	u_int32_t packet_start_code_prefix_23_16	:8;
	u_int32_t packet_start_code_prefix_15_8		:8;
	u_int32_t packet_start_code_prefix_7_0		:8;
	u_int32_t stream_id							:8;
	u_int32_t PES_packet_length_15_8			:8;
	u_int32_t PES_packet_length_7_0				:8;

	u_int32_t original_or_copy					:1;
	u_int32_t copyright							:1;
	u_int32_t data_alignment_indicator			:1;
	u_int32_t PES_priority						:1; 
	u_int32_t PES_scrambling_control			:2;
	u_int32_t reserved_bits_10					:2;

	u_int32_t PES_extension_flag				:1;
	u_int32_t PES_CRC_flag						:1;
	u_int32_t additional_copy_info_flag			:1;
	u_int32_t DSM_trick_mode_flag				:1;
	u_int32_t ES_rate_flag						:1;
	u_int32_t ESCR_flag							:1;
	u_int32_t PTS_DTS_flags						:2;	

	u_int32_t PES_header_data_length			:8;
	// PES Header Fields
	// PES Packet Date Block
#elif __BYTE_ORDER == __BIG_ENDIAN
	u_int32_t packet_start_code_prefix_23_16	:8;
	u_int32_t packet_start_code_prefix_15_8		:8;
	u_int32_t packet_start_code_prefix_7_0		:8;
	u_int32_t stream_id							:8;
	u_int32_t PES_packet_length_15_8			:8;
	u_int32_t PES_packet_length_7_0				:8;
	
	u_int32_t reserved_bits_10					:2;
	u_int32_t PES_scrambling_control			:2;
	u_int32_t PES_priority						:1; 
	u_int32_t data_alignment_indicator			:1;
	u_int32_t copyright							:1;
	u_int32_t original_or_copy					:1;
	
	u_int32_t PTS_DTS_flags						:2;
	u_int32_t ESCR_flag							:1;
	u_int32_t ES_rate_flag						:1;
	u_int32_t DSM_trick_mode_flag				:1;
	u_int32_t additional_copy_info_flag			:1;
	u_int32_t PES_CRC_flag						:1;
	u_int32_t PES_extension_flag				:1;
	
	u_int32_t PES_header_data_length			:8;
	// PES Header Fields
	// PES Packet Date Block
#endif

} PES_HEADER;

typedef struct timestamp_t
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
	u_int32_t marker_0						:1;
	u_int32_t timestamp_32_30				:3;
	u_int32_t pts_or_dts_flag				:4;
	
	u_int32_t timestamp_29_22				:8;

	u_int32_t marker_1						:1;
	u_int32_t timestamp_21_15				:7;
	
	u_int32_t timestamp_14_7				:8;

	u_int32_t marker_2						:1;
	u_int32_t timestamp_6_0 				:7;
#elif __BYTE_ORDER == __BIG_ENDIAN
	u_int32_t pts_or_dts_flag				:4;
	u_int32_t timestamp_32_30				:3;
	u_int32_t marker_0						:1;
	
	u_int32_t timestamp_29_22				:8;
	
	u_int32_t timestamp_21_15				:7;
	u_int32_t marker_1						:1;
	
	u_int32_t timestamp_14_7				:8;
	
	u_int32_t timestamp_6_0					:7;
	u_int32_t marker_2						:1;
#endif

} TIMESTAMP_T;

/*
adts_fixed_header()
{
	syncword; 12 bslbf
	ID; 1 bslbf
	layer; 2 uimsbf
	protection_absent; 1 bslbf
	profile; 2 uimsbf
	sampling_frequency_index; 4 uimsbf
	private_bit; 1 bslbf
	channel_configuration; 3 uimsbf
	original/copy; 1 bslbf
	home; 1 bslbf
}

adts_variable_header()
{
	copyright_identification_bit; 1 bslbf
	copyright_identification_start; 1 bslbf
	frame_length; 13 bslbf
	adts_buffer_fullness; 11 bslbf
	number_of_raw_data_blocks_in_frame; 2 uimsfb
}
*/

typedef struct adts_header
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
	u_int32_t syncword_11_4					: 8;

	u_int32_t protection_absent				: 1;
	u_int32_t layer							: 2;
	u_int32_t ID							: 1;
	u_int32_t syncword_3_0					: 4;	

	u_int32_t channel_configuration_2_2		: 1;
	u_int32_t private_bit					: 1;
	u_int32_t sampling_frequency_index		: 4;
	u_int32_t profile						: 2;

	u_int32_t frame_length_12_11			: 2;
	u_int32_t copyright_identification_start: 1;
	u_int32_t copyright_identification_bit	: 1;
	u_int32_t home							: 1;
	u_int32_t original_or_copy				: 1;
	u_int32_t channel_configuration_1_0		: 2;	
	
	u_int32_t frame_length_10_3				: 8;

	u_int32_t adts_buffer_fullness_10_6		: 5;
	u_int32_t frame_length_2_0				: 3;
	
	u_int32_t number_of_raw_data_blocks_in_frame: 2;
	u_int32_t adts_buffer_fullness_5_0		: 6;	
#elif __BYTE_ORDER == __BIG_ENDIAN
	u_int32_t syncword_11_4					: 8;
	
	u_int32_t syncword_3_0					: 4;
	u_int32_t ID							: 1;
	u_int32_t layer							: 2;
	u_int32_t protection_absent				: 1;
	
	u_int32_t profile						: 2;
	u_int32_t sampling_frequency_index		: 4;
	u_int32_t private_bit					: 1;
	u_int32_t channel_configuration_2_2		: 1;
	
	u_int32_t channel_configuration_1_0		: 2;	
	u_int32_t original_or_copy				: 1;
	u_int32_t home							: 1;	
	u_int32_t copyright_identification_bit	: 1;
	u_int32_t copyright_identification_start: 1;
	u_int32_t frame_length_12_11			: 2;
	
	u_int32_t frame_length_10_3				: 8;
	
	u_int32_t frame_length_2_0				: 3;
	u_int32_t adts_buffer_fullness_10_6		: 5;
	
	u_int32_t adts_buffer_fullness_5_0		: 6;
	u_int32_t number_of_raw_data_blocks_in_frame: 2;
#endif
} ADTS_HEADER;


#pragma pack()


typedef struct pes_t
{
	u_int64_t	pts;
	u_int64_t	dts;
	u_int8_t*	ptr;
	u_int64_t	len;
} PES_T;

typedef struct audio_es_t
{
	u_int64_t	pts;	
	u_int8_t*	ptr;
	u_int64_t	len;
} AUDIO_ES_T;

typedef struct video_es_t
{
	u_int64_t	pts;
	u_int64_t	dts;
	u_int8_t*	ptr;
	u_int64_t	len;
	u_int8_t	key_frame;
} VIDEO_ES_T;

typedef struct nalu_t
{
	u_int8_t*	ptr;
	u_int64_t	len;
} NALU_T;

PES_T* 		pes_copy(PES_T* onep);
AUDIO_ES_T*	audio_es_copy(AUDIO_ES_T* onep);
VIDEO_ES_T* video_es_copy(PES_T* onep);
NALU_T* 	nalu_copy(NALU_T* nalup);


void 		pes_release(void* datap);
void 		audio_es_release(void* datap);
void 		video_es_release(void* datap);
void 		nalu_release(void* datap);

int 		ts_parse_pes(u_int8_t* pes_buffer, int pes_len, PES_T* pesp);


#endif

