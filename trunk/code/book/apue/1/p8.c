/*
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>

#define		    MAXLINE	1024

static	void sig_int(int);

int main(void)
{
    char	    buf[MAXLINE];
    pid_t	    pid;
    int		    status;

    if (signal(SIGINT, sig_int) == SIG_ERR) {
	printf("error: can't install signal handler\n");
	perror("reason: ");
	exit(1);
    }

    printf("%%");
    while (fgets(buf, MAXLINE, stdin) != NULL) {
	buf[strlen(buf) - 1] = 0;

	if ( (pid = fork()) < 0) {
	    printf("error: can't fork a process\n");
	    perror("reason: ");
	    exit(1);
	}
	else if (pid == 0) {
	    execlp(buf, buf, (char*)0);
	    printf("error: can't execlp a program\n");
	    perror("reason: ");
	    exit(1);
	}

	if ( (pid = waitpid(pid, &status, 0)) < 0) {
	    printf("error: waitpid wait for: %d\n", pid);
	    perror("reason: ");
	    exit(1);
	}

	printf("%%");
    }

    exit(0);
}

void sig_int(int signo)
{
    printf("interrupt\n%%");
    fflush(stdout);
}

