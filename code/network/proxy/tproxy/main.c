/**
 *	@file	main.c
 *	@brief	the main function of tcpproxy, it read command line parameter
 *		then run a proxy. 
 * 
 *	@author	FZ
 */


#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/param.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "proxy.h"
#include "proxy_debug.h"
#include "proxy_config.h"

static char	_s_cfgfile[PATH_MAX];	/* config file */
static int	_s_vrycfg;		/* print proxy config and exit */
static proxy_t	*_s_proxy = NULL;	/* proxy config */

volatile int	g_stop;			/* stop program */
int		g_timestamp;		/* timestamp in debug output */
int		g_dbglvl;		/* debug level: 0 disable, 7 max */
int		g_flowlvl;		/* flow level: 0 disable, 7 max */
int		g_httplvl;		/* http level: 0 disable, 7 max */
pthread_t	g_maintid;		/* main thread id */

/**
 *	show usage function.
 *
 *	No return.
 */
static void 
_usage(void)
{
	printf("tproxy <options>\n");
	printf("\t-f\tthe proxy config file\n");
	printf("\t-v\tprint proxy config in config file and exit\n");
	printf("\t-h\tshow help usage\n");
}

/**
 *	Parse comand line argument.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_parse_cmd(int argc, char **argv)
{
	char c;
	char optstr[] = ":f:vh";

	opterr = 0;
	while ( (c = getopt(argc, argv, optstr)) != -1) {
		switch (c) {

		case 'f':
			strncpy(_s_cfgfile, optarg, sizeof(_s_cfgfile) - 1);
			break;

		case 'v':
			_s_vrycfg = 1;
			break;

		case 'h':
			_usage();
			exit(0);

		case ':':
			printf("%c need argument\n", optopt);
			return -1;

		case '?':
			printf("unkowned option: %c\n", optopt);
			return -1;
		}
	}

	if (!_s_cfgfile[0]) {
		printf("not provide config file\n");
		return -1;
	}

	if (optind != argc) {
		return -1;
	}
	
	return 0;
}

/**
 *	The stop signal of program, it interrupt main thread, 
 *
 *	No return.
 */
static void 
_sig_int(int signo)
{
	if (signo != SIGINT)
		return;

	if (pthread_self() == g_maintid) {
		DBG(1, "\n\nrecved stop signal SIGINT\n");
		g_stop = 1;
	}
	else {
		if (g_stop == 0)
			pthread_kill(g_maintid, SIGINT);
	}
}

/**
 *	Initiate global resource.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_initiate(void)
{
	struct sigaction act;

	g_maintid = pthread_self();

	/* using SIGINT as stop signal */ 
	act.sa_handler = _sig_int;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_restorer = NULL;
	if (sigaction(SIGINT, &act, NULL))
		ERR_RET(-1, "sigaction error: %s\n", ERRSTR);

	/* ignore SIGPIPE signal */
	act.sa_handler = SIG_IGN;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_restorer = NULL;
	if (sigaction(SIGPIPE, &act, NULL)) 
		ERR_RET(-1, "sigaction error: %s\n", ERRSTR);

	_s_proxy = proxy_alloc();
	if (!_s_proxy)
		ERR_RET(-1, "alloc proxy failed\n");

	return 0;
}

/**
 *	Release global resource.
 *
 *	No return.
 */
static void 
_release(void)
{
	if (_s_proxy) {
		proxy_free(_s_proxy);
		_s_proxy = NULL;
	}
}

/**
 *	main function of program.
 *
 *	Return 0 if success, -1 on error. 
 */
int 
main(int argc, char **argv)
{
	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_initiate())
		return -1;

	if (cfg_load_file(_s_proxy, _s_cfgfile)) {
		_release();
		ERR_RET(-1, "parse config file %s failed\n", _s_cfgfile);
	}

	if (_s_vrycfg)	
		proxy_print(_s_proxy);		
	else 
		proxy_main(_s_proxy);

	_release();

	return 0;
}




