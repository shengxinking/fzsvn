/**
 *	@file	time_util.h
 *
 *	@brief	time APIs.
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_TIME_UTIL_H
#define FZ_TIME_UTIL_H

#include <sys/time.h>

/**
 *	Return microsecond of two timeval structure @tv1,@tv2.
 */
static inline 
int timeval_sub(struct timeval *tv1, struct timeval *tv2)
{
	int msec;

	msec = (tv1->tv_sec - tv2->tv_sec) * 1000000;
	msec += tv1->tv_usec - tv2->tv_usec;

	return msec;
}


#endif /* end of FZ_TIME_UTIL_H */


