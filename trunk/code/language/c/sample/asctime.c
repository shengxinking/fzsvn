/*
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>

int main(void)
{
	time_t now;
	struct tm local, utc;

	now = time(NULL);

	printf("local time is %s\n", asctime(localtime(&now)));
	printf("UTC time is %s\n", asctime(gmtime(&now)));

	return 0;
}

