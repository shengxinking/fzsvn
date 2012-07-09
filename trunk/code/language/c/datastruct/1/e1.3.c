/*
 *
 */

#include <stdio.h>

void DoesSomething(int *first, int *second)
{
	*first = *second - *first;
	*second = *second - *first;
	*first = *second + *first;
}

void swap(int *a, int *b)
{
	int     tmp;
	
	if (a && b) {
		tmp = *a;
		*a = *b;
		*b = tmp;
	}
}

int main(void)
{
	int a = 5, b = 4, c = 8, d = 18;

	printf("a = %d, b = %d\n", a, b);
	DoesSomething(&a, &b);
	printf("a = %d, b = %d\n", a, b);

	printf("c = %d, d = %d\n", c, d);
	swap(&c, &d);
	printf("c = %d, d = %d\n", c, d);

	return 0;
}
