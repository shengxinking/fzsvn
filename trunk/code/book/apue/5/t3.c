/*
 *
 */

#include <stdio.h>

int main(void)
{
    int		n;

    if ( (n = printf("")) < 0) {
	printf("printf error\n");
	perror("");
	return -1;
    }
    else if (n == 0) {
	perror("");
	printf("printf %d bytes\n", n);
    }
    else
	printf("%d bytes is printf\n", n);

    return 0;
}
