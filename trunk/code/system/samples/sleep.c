/**
 *	@file	
 *
 *	@brief
 *	
 *	@author	Forrest.zhang	
 *
 *	@date
 */

#include <unistd.h>
#include <string.h>
#include <sys/select.h>

int 
main(void)
{
	int n;
	struct timeval tv;

	while (1) {
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		n = select(0, NULL, NULL, NULL, &tv);
		if (n < 0) {
			printf("select failed\n");
		}
		else if (n == 0) {
			printf("select timeout\n");
		}
		else {
			printf("have event in select\n");
		}
	}

	return 0;
}




