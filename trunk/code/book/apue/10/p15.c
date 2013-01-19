/*
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>

static void sig_int(int);
static void pr_mask(const char*);

int main(void)
{
    sigset_t	    nmask, omask, zmask;

    if (signal(SIGINT, sig_int) == SIG_ERR) {
	perror("signal(SIGINT) error");
	exit(1);
    }

    sigemptyset(&zmask);

    sigemptyset(&nmask);
    sigaddset(&nmask, SIGINT);

    if (sigprocmask(SIG_BLOCK, &nmask, &omask) < 0) {
	perror("sigprocmask(SIG_BLOCK) error");
	exit(1);
    }

    pr_mask("in critical region");
    
    if (sigsuspend(&zmask) != -1) {
	perror("sigsuspend error");
	exit(1);
    }

    pr_mask("after return from sigsuspend");
    
    if (sigprocmask(SIG_SETMASK, &omask, NULL) < 0) {
	perror("sigprocmask(SIG_SETMASK) error");
	exit(1);
    }

    exit(0);

}


void sig_int(int signo)
{
    pr_mask("\n in sig_int");
    return;
}

void pr_mask(const char* str)
{
    sigset_t	    sigset;
    int		    errno_save;

    errno_save = errno;

    if (sigprocmask(0, NULL, &sigset) < 0) {
	perror("sigprocmask error");
	exit(1);
    }
    
    printf("%s: ", str);
    if (sigismember(&sigset, SIGINT))	printf("SIGINT");
    if (sigismember(&sigset, SIGQUIT))	printf("SIGQUIT");
    if (sigismember(&sigset, SIGUSR1))	printf("SIGUSR1");
    if (sigismember(&sigset, SIGALRM))	printf("SIGALRM");

    printf("\n");
    errno = errno_save;
}
    
