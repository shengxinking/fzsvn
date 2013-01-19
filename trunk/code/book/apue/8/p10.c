/*
 *
 */

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

int main(void)
{
    pid_t	cpid;

    if ( (cpid = fork()) < 0) {
	printf("fork error\n");
	perror(NULL);
	exit(1);
    }
    else if(cpid == 0) {
	if (execl("testinterp", "test", "myarg1", "myarg2", NULL) < 0)
	    printf("error when execl\n");
	exit(1);
    }

    if (waitpid(cpid, NULL, 0) < 0) {
	printf("waitpid error\n");
	exit(1);
    }

    exit(0);
}
