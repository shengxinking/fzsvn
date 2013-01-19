/*
 *
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>

extern pr_exit(int status);

int main(void)
{
    pid_t   cpid;
    int	    status;

    if ( (cpid = fork()) < 0)
	_exit(127);
    else if (cpid == 0)
	_exit(8);

    if (wait(&status) != cpid)
	_exit(18);

    pr_exit(status);

    if ( (cpid = fork()) < 0)
	_exit(127);
    else if (cpid == 0)
	abort();

    if (wait(&status) != cpid)
	_exit(18);

    pr_exit(status);

    if ( (cpid = fork()) < 0)
	_exit(127);
    else if (cpid == 0)
	status /= 0;

    if (wait(&status) != cpid)
	_exit(18);

    pr_exit(status);

    _exit(0);
}
