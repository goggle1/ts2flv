
#ifndef __CONTINUE_DATA_H__
#define __CONTINUE_DATA_H__

#include <sys/types.h>

int continue_data_read_timestamp(u_int64_t* valuep);
int continue_data_write_timestamp(u_int64_t value);

#endif

