
#include <stdio.h>
#include <stdlib.h>

#include "TS.h"

void print_usage(char* program_name)
{
	fprintf(stdout, "%s [index] [out.flv] [in.ts] {in_2.ts}\n", program_name);
}

int main(int argc, char* argv[])
{
	if(argc < 4)
	{
		print_usage(argv[0]);
		exit(1);
	}

	int		start_index = atoi(argv[1]);
	char* 	flv_file 	= argv[2];
	char* 	ts_file 	= argv[3];
	char*   ts_2_file	= argv[4];

#if 0	
	make_null_flv(flv_file);
	return 0;
#endif
	
	int ret = ts2flv(start_index, flv_file, ts_file, ts_2_file);
	
	return ret;
}

