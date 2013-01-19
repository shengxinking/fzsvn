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
	if (setsid() < 0) {
	    printf("setsid error\n");
	    perror(NULL);
	    exit(1);
	}
	sleep(1);
	exit(0);
    }

    pr_ids("child");
    sleep(1);
    setsid();
    pr_ids("child");
    
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
