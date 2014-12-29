/**
 *	@file	rt_signal
 *
 *	@brief	The realtime signal test program.
 *
 *	@author	Forrest.zhang
 *	
 *	@date	2010-08-30
 */


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/types.h>
#include <signal.h>

/**
 *	Show help message	
 *	
 *	No return.
 */
static void 
_usage(void)
{
	printf("rt_signal <option>\n");
	printf("\t-h\tthe help message\n");
}


/**
 *	Parse command line argument.	
 *
 * 	Return 0 if parse success, -1 on error.
 */
static int 
_parse_cmd(int argc, char **argv)
{
	char opt;
	char optstr[] = ":h";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {
			
		case 'h':
			return -1;

		case ':':
			printf("Option %c missing argument\n", optopt);
			return -1;

		case '?':
			printf("Unknowed option %c\n", optopt);
			return -1;
		}
	}

	if (argc != optind)
		return -1;

	return 0;
}

static void 
_sig_realtime(int signo, siginfo_t *sinfo, void *context)
{
	printf("recv signal %d\n", signo);
	printf("signal info(%p)\n", sinfo);
	if (sinfo) {
		printf("\tsi_signo: %d\n", sinfo->si_signo);
		printf("\tsi_errno: %d\n", sinfo->si_errno);
		printf("\tsi_code: %d\n", sinfo->si_code);
		printf("\tsi_int: %d\n", sinfo->si_int);
	}

	printf("context(%p)\n", context);
}


/**
 *	Init some global resource used in program.	
 *
 * 	Return 0 if success, -1 on error.
 */
static int 
_initiate(void)
{
	int i;
	struct sigaction sa;

	for (i = 34; i < 64; i++) {
		memset(&sa, 0, sizeof(struct sigaction));
		sa.sa_sigaction = _sig_realtime;
		sa.sa_flags |= SA_SIGINFO;
		if (sigaction(i, &sa, NULL)) {
			printf("sigaction(%d) error: %s\n", 
			       i, strerror(errno));
		}
	}

	return 0;
}


/**
 *	Release global resource alloced by _initiate().	
 *
 * 	No Return.
 */
static void 
_release(void)
{
	
}


/**
 *	The main entry of program.	
 *
 * 	Return 0 if success, other value on error.
 */
int 
main(int argc, char **argv)
{
	int i;
	sigval_t sv;

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_initiate()) {
		return -1;
	}

	for (i = 34; i < 64; i++) {
		sv.sival_int = i - 34;
		sigqueue(getpid(), i, sv);		
	}

	_release();

	return 0;
}



