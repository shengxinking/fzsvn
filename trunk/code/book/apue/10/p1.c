/*
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

static void sig_int(int);

int main(void)
{
    sigset_t		    set, oset;
    struct sigaction	    act;
    
    sigemptyset(&set);
    
    act.sa_handler = sig_int;
    act.sa_mask = set;
    act.sa_flags = SA_NODEFER;

    if (sigaction(SIGINT, &act, NULL) < 0) {
	printf("set SIGINT action error\n");
	perror(NULL);
	exit(1);
    }

    while (1)
	pause();

    exit(0);
}

void sig_int(int signo)
{
    printf("get signal SIGINT\n");
    sleep(10);
    printf("sig_int done\n");
}
    

