/*
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

static void sig_chld(int signo);

int main(void)
{
    pid_t		cpid;

    if (signal(SIGCHLD, sig_chld) == SIG_ERR) {
	printf("can't set signal handler\n");
	perror(NULL);
	exit(1);
    }

    if ( (cpid = fork()) < 0) {
	perror(NULL);
	exit(1);
    }
    else if (cpid == 0) {
	sleep(2);
	exit(0);
    }
    pause();
    exit(0);
}

static void sig_chld(int signo)
{
    pid_t	    pid;
    int		    status;

    printf("SIGCHLD received\n");
    if (signal(SIGCHLD, sig_chld) == SIG_ERR)
	perror("signal error");

    if ( (pid = wait(&status)) < 0)
	perror("wait error");

    printf("pid = %d\n", pid);
    return;
}
