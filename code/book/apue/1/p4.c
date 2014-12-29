/*
 *  write by jbug
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    printf("pid: %d\nppid: %d\nuid: %d\ngid: %d\n", getpid(),
	    getppid(), getuid(), getgid());

    exit(0);
}
