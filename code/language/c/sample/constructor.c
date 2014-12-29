/*
 *  test constructor and destructor attribute that gcc defined
 *
 */

#include <stdio.h>
#include <stdlib.h>

void ini(void) __attribute__ ((constructor));

void ini(void)
{
	printf("before main %d\n", 1024);
//	exit(1);
}

void dest(void) __attribute__ ((destructor));

void dest(void)
{
	printf("after main %d\n", 1025);
	exit(1);
}

int main(void)
{
	printf("in main\n");
	
	exit(0);
}



