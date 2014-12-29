/*
 *
 */

#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

static char *env_init[] = {"USE=jbug", "PATH=/tmp", NULL};

int main(void)
{
    pid_t	cpid;

    if ( (cpid = fork()) < 0)
	_exit(127);
    else if (cpid == 0) {
	if (execle("p9", "jbug", "my arg1", "my arg2", NULL, env_init) < 0)
	    printf("execle error\n");
	_exit(127);
    }

    if (waitpid(cpid, NULL, 0) < 0)
	_exit(127);

    if ( (cpid = fork()) < 0)
	_exit(127);
    else if (cpid == 0) {
	if (execlp("/usr/bin/echo", "echo", "-e", "hello jbug.", NULL) < 0)
	    printf("execlp error\n");
	_exit(127);
    }

    exit(0);
}
