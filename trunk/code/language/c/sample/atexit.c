/*
 *
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

void f1(void)
{
	printf("you called f1\n");
}

void f2(void)
{
	printf("you called f2\n");
}

void f3(void)
{
	printf("you called f3\n");
}

int main(void)
{
	atexit(f1);
	atexit(f2);
	atexit(f3);
	atexit(f1);

//	_exit(0);
	exit(0);
	return 0;
}


