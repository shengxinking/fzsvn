/*
 *
 *
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>

int main(void)
{
	unsigned char c[1024] = { 0 };
	int i;
	int j;

	printf("please input a unicode character:\n");

	scanf("%s", c);
	
	printf("the unicode value is: ");
	for (i = 0; i < 4; i++) {
		j = c[i];
		printf("%x ", j);
	}
	printf("\n");

	return 0;
}
