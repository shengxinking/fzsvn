/*
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    printf("fd 0 is %s", isatty(0) ? "tty" : "not a tty");
    if (isatty(0))
	printf(", tty name is %s\n", ttyname(0));
    else
	putchar('\n');
    printf("fd 1 is %s", isatty(1) ? "tty" : "not a tty");
    if (isatty(1))
	printf(", tty name is %s\n", ttyname(1));
    else
	putchar('\n');
    printf("fd 2 is %s", isatty(2) ? "tty" : "not a tty");
    if (isatty(2))
	printf(", tty name is %s\n", ttyname(2));
    else
	putchar('\n');

    exit(0);
}

