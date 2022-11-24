#ifndef _mt_time_h
#define _mt_time_h

void mt_time(char* id);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "mt_log.c"
#include <stdio.h>
#include <sys/time.h>

struct timeval mt_time_stamp;

void mt_time(char* id)
{
    if (id)
    {
	struct timeval ts;
	gettimeofday(&ts, NULL);
	mt_log_info("%s time is %lu us", id, (ts.tv_sec - mt_time_stamp.tv_sec) * 1000000 + ts.tv_usec - mt_time_stamp.tv_usec);
	mt_time_stamp = ts;
    }
    else
    {
	gettimeofday(&mt_time_stamp, NULL);
    }
}

#endif
