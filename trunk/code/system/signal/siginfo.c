/*
 *
 *
 *
 */

#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>


static void  
_sig_safe(int signo, siginfo_t *si, void *arg)
{
	printf("recv signal %d\n", signo);
}

int 
main(void)
{
	struct sigaction act;
	int i;

	memset(&act, 0, sizeof(act));
	act.sa_sigaction = _sig_safe;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;
	act.sa_restorer = NULL;
	for (i = 35; i < 60; i++) {
		sigaction(i, &act, NULL);
	}

	sleep(100);

	return 0;
}



