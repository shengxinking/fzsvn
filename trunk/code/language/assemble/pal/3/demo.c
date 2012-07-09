/*
 *
 *
 *
 */

#include <stdio.h>

void func1(void)
{
	int i, j;
	for (i = 0; i < 10000; i++)
		j += i;
}

void func2(void)
{
	int i, j;
	func1();
	for (i = 0; i < 200000; i++)
		j = i;
}

int main(void)
{

	int i, j;

	for (i = 0; i < 100; i++)
		func1();

	for (i = 0; i < 5000; i++)
		func2();

	return 0;
}
