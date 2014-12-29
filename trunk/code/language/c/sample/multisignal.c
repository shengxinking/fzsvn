/*
 *  test linux can queue the same signal
 *
 *  write by Forrest.zhang
 */

#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/wait.h>

static void sig_chld(int signo)
{
    pid_t       pid;
    int         status;
    while( (pid = waitpid(-1, &status, WNOHANG)) > 0) {
	printf("child %d terminator\n", pid);
    }
    
    return;
}

int main(void)
{
    int        i;
    pid_t      pid;

    signal(SIGCHLD, sig_chld);

    for (i = 0; i < 5; i++) {
	pid = fork();
	if (pid == 0) {
	    sleep(1);
	    printf("child %d running\n", getpid());
	    exit(0);
	}
    }

    sleep(1);

    return 0;
}
