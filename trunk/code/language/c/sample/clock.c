/*
 *
 *
 */

#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <stdio.h>

int main(void)
{
	clock_t begin, end;

	begin = clock();
	sleep(1);
	end = clock();

	printf("begin is %ld clocks, end is %ld clocks, spent %ld clocks\n", begin, end, (end - begin) );
	printf("begin is %ld seconds, end is %ld seconds, spend %ld seconds\n", 
		begin/CLOCKS_PER_SEC, end / CLOCKS_PER_SEC, (end - begin) / CLOCKS_PER_SEC);

	return 0;
}

