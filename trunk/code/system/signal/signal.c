/*
 *
 *
 *
 */

#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>


#define	SIG_REBOOT		35
#define SIG_SHUTDOWN		36
#define	SIG_UPGRADE		37
#define	SIG_UPDATE_CLI		50
#define	SIG_UPDATE_FULLCLI	51
#define	SIG_UPDATE_AV		52

static void _sig_handler(int signo, siginfo_t *si, void *ctxt)
{
	printf("recv signo %d\n", signo);

	switch(signo) {
		
	case SIGINT:
		printf("SIGINT\n");
		break;

	case SIG_REBOOT:
		printf("SIG_REBOOT\n");
		break;

       case SIG_SHUTDOWN:
		printf("SIG_SHUTDOWN\n");
		break;

	case SIG_UPGRADE:
		printf("SIG_UPDATE\n");
		break;

	case SIG_UPDATE_CLI:
		printf("SIG_UPDATE_CLI\n");
		break;

	case SIG_UPDATE_FULLCLI:
		printf("SIG_UPDATE_FULLCLI\n");
		break;

	case SIG_UPDATE_AV:
		printf("SIG_UPDATE_AV\n");
		break;

	default:
		printf("unknown sig %d\n", signo);

	}

}

static void  
_sig_safe(int signo)
{
	printf("recv signal %d\n", signo);
}

int main(void)
{
	struct sigaction act;
	int i;

	for (i = 35; i < 64; i++)
		signal(i, _sig_safe);

#if 0
	memset(&act, 0, sizeof(act);
	act.sa_sigaction = _sig_handler;
	sigemptyset(&act.sa_mask);
//	act.sa_flags = SA_SIGINFO;
	act.sa_restorer = NULL;
	for (i = 35; i < 60; i++) {
		sigaction(i, &act, NULL);
	}
#endif

	sleep(100);

	return 0;
}



