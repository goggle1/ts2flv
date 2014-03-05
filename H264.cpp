
#include "H264.h"

int bits_read_ue(u_int8_t* bitsp, int start_pos, u_int32_t* valuep, int* end_posp)
{
	int temp_pos = start_pos;
	while(1)
	{
		int byte_pos		= temp_pos/8;
		int bit_pos_in_byte	= temp_pos%8;
		u_int8_t bit = bitsp[byte_pos] >> (7-bit_pos_in_byte) & 0x01;
		if(bit == 1)
		{
			break;
		}		
		temp_pos ++;		
	}	

	int bits_num = temp_pos - start_pos;	
	temp_pos ++;
	
	int bits_index = 0;	
	u_int32_t value = 0;
	for(bits_index=0; bits_index<bits_num; bits_index++)
	{
		int byte_pos		= (temp_pos+bits_index)/8;
		int bit_pos_in_byte = (temp_pos+bits_index)%8;
		u_int8_t bit = bitsp[byte_pos] >> (7-bit_pos_in_byte) & 0x01;
		value = value << 1 | bit;		
	}
	u_int32_t code_num = (1 << bits_num) - 1 + value;

	*valuep = code_num;
	*end_posp = temp_pos + bits_num;

	return 0;
}


