/*
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int func(void)
{
    pid_t	    cpid;

    if ( (cpid = vfork()) < 0)
	exit(1);
    else if (cpid == 0) {
	printf("%d: in func1 return 0\n", getpid());
	return 0;
    }

    printf("%d: in func1 return 1\n", getpid());
    return 1;
}


int main(void)
{
    int		status;

    status = func();

    printf("%d: in main the return value of func is %d\n", getpid(), status);

    exit(0);
}
