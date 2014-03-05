
#include <stdio.h>
#include <stdlib.h>

#include "TS.h"

void print_usage(char* program_name)
{
	fprintf(stdout, "%s [example.ts] [example.flv]\n", program_name);
}

int main(int argc, char* argv[])
{
	if(argc < 3)
	{
		print_usage(argv[0]);
		exit(1);
	}
	
	char* ts_file = argv[1];
	char* flv_file = argv[2];

#if 0	
	make_null_flv(flv_file);
	return 0;
#endif
	
	int ret = ts2flv(ts_file, flv_file);
	
	return ret;
}

