/*
 *
 *
 */

#include <stdlib.h>
#include <unistd.h>

int f1(void)
{
    if (access("/etc/issue", F_OK))
	printf("/etc/issue not exist\n");
    else
	printf("/etc/issue exist\n");

    return 0;
}




