/*
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

static void sig_int(int);
static void sig_chld(int);

int main(void)
{
    int status;

    if (signal(SIGINT, sig_int) == SIG_ERR)
	_exit(1);

    if (signal(SIGCHLD, sig_chld) == SIG_ERR)
	_exit(1);

    if ( (status = system("/bin/ed")) < 0)
	_exit(1);

    exit(0);
}

void sig_int(int signo)
{
    printf("caught SIGINT\n");
    return;
}

void sig_chld(int signo)
{
    printf("caught SIGCHLD\n");
}


