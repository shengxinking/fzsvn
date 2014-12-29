/*
 *
 *
 *
 *
 */

#include <stdio.h>

float function1(int radius)
{
	float result = radius * radius * 3.1415926;
	return result;
}

int main(void)
{
	int i;
	float result;
	
	i = 10;
	result = function1(i);
	printf("Radius: %d, Area: %f\n", i, result);

	i = 2;
	result = function1(i);
	printf("Radius: %d, Area: %f\n", i, result);

	i = 120;
	result = function1(i);
	printf("Radius: %d, Area: %f\n", i, result);

	return 0;
}
	
