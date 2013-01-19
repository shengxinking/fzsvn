/*
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>

static void sig_int(int);
static void pr_mask(const char*);

int main(void)
{
    sigset_t		zmask;

    sigemptyset(&zmask);

    if (signal(SIGINT, sig_int) == SIG_ERR)
	_exit(1);
    
    pr_mask("before interrupt");

    if (sigsuspend(&zmask) != -1) 
	exit(1);

    pr_mask("after interrupt");

    exit(0);
}

void sig_int(int signo)
{
    pr_mask("in sig_int");
    return;
}

void pr_mask(const char* str)
{
    sigset_t        sigset;
    int             errno_save;

    errno_save = errno;

    if (sigprocmask(0, NULL, &sigset) < 0) {
	perror("sigprocmask error");
	exit(1);
    }

    printf("%s: ", str);
    if (sigismember(&sigset, SIGINT))   printf("SIGINT");
    if (sigismember(&sigset, SIGQUIT))  printf("SIGQUIT");
    if (sigismember(&sigset, SIGUSR1))  printf("SIGUSR1");
    if (sigismember(&sigset, SIGALRM))  printf("SIGALRM");

    printf("\n");
    errno = errno_save;
}

