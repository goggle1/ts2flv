#ifndef __TS_H__
#define __TS_H__

#include <sys/types.h>

#define TS_PACKET_SIZE	188

#define STREAM_AUDIO_AAC	0x0F
#define STREAM_AUDIO_MP3	0x03
#define STREAM_VIDEO_H264	0x1b

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

typedef struct pes_header
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
	u_int32_t packet_start_code_prefix_23_16	:8;
	u_int32_t packet_start_code_prefix_15_8		:8;
	u_int32_t packet_start_code_prefix_7_0		:8;
	u_int32_t stream_id							:8;
	u_int32_t pes_packet_length_15_8			:8;
	u_int32_t pes_packet_length_7_0				:8;

	u_int32_t pes_header_flags_OC				:1;
	u_int32_t pes_header_flags_CR				:1;
	u_int32_t pes_header_flags_DA				:1;
	u_int32_t pes_header_flags_PR				:1; 
	u_int32_t pes_header_flags_SC				:2;
	u_int32_t reserved_bits_10					:2;

	u_int32_t pes_header_flags_EXT				:1;
	u_int32_t pes_header_flags_CRC				:1;
	u_int32_t pes_header_flags_AC				:1;
	u_int32_t pes_header_flags_TM				:1;
	u_int32_t pes_header_flags_RATE				:1;
	u_int32_t pes_header_flags_ESCR				:1;
	u_int32_t pes_header_flags_PD				:2;	

	u_int32_t pes_header_length					:8;
	// PES Header Fields
	// PES Packet Date Block
#elif __BYTE_ORDER == __BIG_ENDIAN
	u_int32_t packet_start_code_prefix_23_16	:8;
	u_int32_t packet_start_code_prefix_15_8		:8;
	u_int32_t packet_start_code_prefix_7_0		:8;
	u_int32_t stream_id							:8;
	u_int32_t pes_packet_length_15_8			:8;
	u_int32_t pes_packet_length_7_0				:8;
	
	u_int32_t reserved_bits_10					:2;
	u_int32_t pes_header_flags_SC				:2;
	u_int32_t pes_header_flags_PR				:1; 
	u_int32_t pes_header_flags_DA				:1;
	u_int32_t pes_header_flags_CR				:1;
	u_int32_t pes_header_flags_OC				:1;
	
	u_int32_t pes_header_flags_PD				:2;
	u_int32_t pes_header_flags_ESCR				:1;
	u_int32_t pes_header_flags_RATE				:1;
	u_int32_t pes_header_flags_TM				:1;
	u_int32_t pes_header_flags_AC				:1;
	u_int32_t pes_header_flags_CRC				:1;
	u_int32_t pes_header_flags_EXT				:1;
	
	u_int32_t pes_header_length					:8;
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

#pragma pack()

int ts2flv(char* ts_file, char* flv_file);

#endif

