#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "continue_data.h"

int continue_data_read_timestamp(u_int64_t* valuep)
{
	char* data_file = "continue.data";
	int fd = open(data_file, O_RDONLY);
    if(fd == -1)
    {
    	fprintf(stderr, "open %s error! \n", data_file);
        return -1;
    }
    
	u_int8_t  temp[8] = {0};
	ssize_t read_size = 0;    
	read_size = read(fd, (void*)temp, 8);
	if(read_size < 8)
	{
		close(fd);
		return -1;
	}

	u_int64_t value = 0;
	u_int64_t temp_value = 0;
	int index = 0;
	for(index=0; index<8; index++)
	{
		temp_value = temp[index];
		value = value | temp_value << ((7-index)*8);
	}
	
	fprintf(stdout, "%s: value=%lu[0x%08lX]\n", __FUNCTION__, value, value);
	
	*valuep = value;

	close(fd);

    return 0;
}

int continue_data_write_timestamp(u_int64_t value)
{
	char* data_file = "continue.data";
	int fd = open(data_file, O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
    if(fd == -1)
    {
    	fprintf(stderr, "open %s error! \n", data_file);
        return -1;
    }
    
	u_int8_t  temp[8] = {0};
	temp[0] 	= value>>56 & 0x000000FF;
	temp[1]		= value>>48 & 0x000000FF;
	temp[2]		= value>>40 & 0x000000FF;
	temp[3]		= value>>32 & 0x000000FF;
	temp[4] 	= value>>24 & 0x000000FF;
	temp[5]		= value>>16 & 0x000000FF;
	temp[6]		= value>> 8 & 0x000000FF;
	temp[7]		= value>> 0 & 0x000000FF;
	ssize_t write_size = write(fd, &temp, 8);
	if(write_size != 8)
	{
		close(fd);
		return -1;
	}

	fprintf(stdout, "%s: value=%lu[0x%08lX]\n", __FUNCTION__, value, value);

	close(fd);

    return 0;
}


