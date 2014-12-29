/*
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

static void pr_ids(const char*);
static void sig_hup(int signo);

int main(void)
{
    char		c;
    pid_t		cpid;

    pr_ids("parent");
    
    if ( (cpid = fork()) < 0) {
	printf("fork error\n");
	perror(NULL);
	exit(1);
    }
    else if (cpid > 0) {
	sleep(5);
	exit(0);
    }

    pr_ids("child");
    signal(SIGHUP, sig_hup);
    kill(getpid(), SIGTSTP);
    pr_ids("child");
    if (read(STDIN_FILENO, &c, 1) != 1) {
	printf("read error from control terminal\n");
	perror(NULL);
	exit(1);
    }
    printf("read %c from stdin\n", c);
    
    exit(0);
}

void pr_ids(const char* name)
{
    printf("%s: pid = %d, ppid = %d, pgrp = %d\n", 
	    name, getpid(), getppid(), getpgrp());
    fflush(stdout);
}

void sig_hup(int signo)
{
    printf("SIGHUP received, pid = %d\n", getpid());
    return;
}
