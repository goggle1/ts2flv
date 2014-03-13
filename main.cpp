
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "ts2flv.h"

void print_usage(char* program_name)
{
	fprintf(stdout, "%s [in.ts] [out.flv]\n", program_name);
}

int main(int argc, char* argv[])
{
#if 0	
	make_null_flv(flv_file);
	return 0;
#endif

	if(argc < 3)
	{
		print_usage(argv[0]);
		exit(1);
	}
	
	char* 	ts_file 	= argv[1];
	char* 	flv_file 	= argv[2];

	TS2FLV ts2flv;
	int ret = ts2flv.file2file(ts_file, flv_file);

	return 0;
}

