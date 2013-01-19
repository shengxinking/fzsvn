/*
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

static void sig_chld(int);

int main(void)
{
    sigset_t		    set, oset;
    struct sigaction	    act;
    int			    status;
    pid_t		    cpid;
    
    sigemptyset(&set);
    
    act.sa_handler = sig_chld;
    act.sa_mask = set;
    act.sa_flags = SA_NOCLDWAIT;

    if (sigaction(SIGCHLD, &act, NULL) < 0) {
	printf("set SIGCHLD action error\n");
	perror(NULL);
	exit(1);
    }

    if ( (cpid = fork()) == 0) {
	printf("child done\n");
	exit(0);
    }

    if (wait(&status) != cpid) {
	printf("wait error\n");
	perror(NULL);
	exit(1);
    }

    exit(0);
}

void sig_chld(int signo)
{
    printf("get signal SIGCHLD\n");
    sleep(5);
    printf("sig_chld done\n");
}
    

