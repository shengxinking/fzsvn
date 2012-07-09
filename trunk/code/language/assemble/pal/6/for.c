/* for.c : test GCC assemble for for statement */

#include <stdio.h>

int main(void)
{
	int i = 0;
	int j;

	for (i = 0; i < 1000; i++) {
		j = i * 5;
		printf("The answer is %d\n", j);
	}

	return 0;
}

