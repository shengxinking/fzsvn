/*
 *
 *
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>

int main(void)
{
	struct timeval begin, end;
	struct timespec stime;

	stime.tv_sec=5;
	stime.tv_nsec = 0;

	gettimeofday(&begin, NULL);
	nanosleep(&stime, NULL);
	gettimeofday(&end, NULL);

	printf("begin is %ld seconds %ld microseconds, end is %ld seconds %ld microseconds\n", 
		begin.tv_sec, begin.tv_usec, end.tv_sec, end.tv_usec);

	printf("spend %ld seconds %ld microseconds\n", end.tv_sec - begin.tv_sec, end.tv_usec - begin.tv_usec);

	return 0;
}

