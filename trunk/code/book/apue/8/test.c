/*
 *
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
/*
int main(void)
{
    pid_t	    cpid;

    if ( (cpid = fork()) < 0)
	_exit(127);
    else if (cpid == 0)
	_exit(0);

    sleep (15);
    waitpid(cpid, NULL, 0);

    _exit(0);
}
*/

int main(void)
{
    printf("current: uid = %d, euid = %d\n", getuid(), geteuid());

    if (setreuid(geteuid(), getuid()) < 0)
	printf("can't setreuid\n");

    printf("current: uid = %d, euid = %d\n", getuid(), geteuid());
    return 0;
}
