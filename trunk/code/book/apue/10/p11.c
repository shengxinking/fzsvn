/*
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

void sig_int(int signo);

int main(void)
{
    sigset_t		nmask, omask, pmask;

    if (signal(SIGINT, sig_int) == SIG_ERR) {
	perror("signal error");
	exit(1);
    }

    sigemptyset(&nmask);
    sigaddset(&nmask, SIGINT);

    if (sigprocmask(SIG_BLOCK, &nmask, &omask) < 0) {
	perror("sigprocmask error");
	exit(1);
    }

    sleep(5);

    if (sigpending(&pmask) < 0) {
	perror("sigpending error");
	exit(1);
    }

    if (sigismember(&pmask, SIGINT))
	printf("SIGINT is pending\n");

    if (sigprocmask(SIG_SETMASK, &omask, NULL) < 0) {
	perror("sigprocmask error");
	exit(1);
    }
    printf("SIGINT unblocked\n");
    
    sleep(5);
    
    exit(0);
}

void sig_int(int signo)
{
    printf("I caught SIGINT\n");

    if (signal(SIGINT, SIG_DFL) == SIG_ERR) {
	perror("signal error");
	exit(1);
    }

    return;
}

