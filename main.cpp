
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
	
	char* file_ts = argv[1];
	char* file_flv = argv[2];
	
	int ret = ts2flv(file_ts, file_flv);
	
	return ret;
}

