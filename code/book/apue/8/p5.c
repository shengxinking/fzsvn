/*
 *
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    pid_t	    cpid;

    if ( (cpid = fork()) < 0) 
	exit(7);
    else if (cpid == 0) {
	if ( (cpid = fork()) < 0)
	    exit(7);
	else if (cpid > 0)
	    _exit(0);

//	sleep(2);

	printf("second child, parent pid = %d\n", getppid());
	exit(0);
    }

    if (waitpid(cpid, NULL, 0) != cpid) {
	printf("waitpid error for %d\n", cpid);
	perror(NULL);
	exit(1);
    }

    exit(0);
}

