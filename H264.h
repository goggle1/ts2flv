
#ifndef __H264_H__
#define __H264_H__

#include <sys/types.h>

/* slice_type    指明片的类型 
   slice_type          Name of slice_type 
   0                        P (P slice) 
   1                        B (B slice) 
   2                        I (I slice) 
   3                        SP (SP slice) 
   4                        SI (SI slice) 
   5                        P (P slice) 
   6                        B (B slice) 
   7                        I (I slice) 
   8                        SP (SP slice) 
   9                        SI (SI slice) 
*/ 

#define SLICE_TYPE_P	0
#define SLICE_TYPE_B	1
#define SLICE_TYPE_I	2
#define SLICE_TYPE_SP	3
#define SLICE_TYPE_SI	4
#define SLICE_TYPE_P2	5
#define SLICE_TYPE_B2	6
#define SLICE_TYPE_I2	7
#define SLICE_TYPE_SP2	8
#define SLICE_TYPE_SI2	9

int bits_read_ue(u_int8_t* bitsp, int start_pos, u_int32_t* valuep, int* end_posp);

#endif
