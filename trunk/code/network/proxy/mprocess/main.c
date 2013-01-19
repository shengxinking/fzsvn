/*
 *	@file	main.c
 *
 *	@brief	Using multi-process to run proxy.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>

#include "childproc.h"
#include "sock_util.h"

static ip_port_t	_g_svraddr;	/* proxy address */
static ip_port_t	_g_rsaddrs[MAX_RS]; /* real server address */
static int		_g_httpfd;
static int		_g_nrsaddr;	/* number of real server address */
static int		_g_capacity = 10;	/* the proxy capacity */
static int		_g_splice = 0;
static int 		_g_nb_splice = 0;
static int		_g_stop = 0;
static cproc_t		_g_childs[MAX_CP];
static int		_g_nchild = 8;

/**
 *	Show help information
 *
 *	No return.
 */
static void 
_usage(void)
{
	printf("proxy <options> <real server address>\n");
	printf("\t-a\tproxy address\n");
	printf("\t-c\tproxy concurrency capacity\n");
	printf("\t-s\tuse system splice\n");
	printf("\t-n\tuse nb_splice\n");
	printf("\t-w\tthe child process number\n");
	printf("\t-h\tshow help message\n");
}


/**
 *	Parse command line argument.
 *
 *	Return 0 if parse success, -1 on error.
 */
static int 
_parse_cmd(int argc, char **argv)
{
	char optstr[] = ":a:snc:w:h";
	char opt;
	int i = 0;

	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		switch (opt) {
			
		case 'a':
			if (ip_port_from_str(&_g_svraddr, optarg)) {
				printf("invalid proxy address: %s\n", optarg);
				return -1;
			}
			
			break;
			
		case 'c':
			_g_capacity = atoi(optarg);
			if (_g_capacity < 1) {
				printf("invalid proxy capacity: %s\n", optarg);
				return -1;
			}
			break;

		case 'w':
			_g_nchild = atoi(optarg);
			if (_g_nchild < 1) {
				printf("invalid child number: %s\n", optarg);
				return -1;
			}
			break;

		case 's':
			_g_splice = 1;
			break;

		case 'n':
			_g_nb_splice = 1;
			break;

		case 'h':
			_usage();
			exit(0);

		case ':':
			printf("option %c missing argument\n", optopt);
			return -1;

		case '?':
			printf("invalid option %c\n", optopt);
			return -1;
		}
	}

	if (optind >= argc) {		
		return -1;
	}

	for (i = optind; i < argc; i++) {
		if (ip_port_from_str(&_g_rsaddrs[i - optind], argv[i])) {
			printf("invalid real server address\n");
			return -1;
		}
		_g_nrsaddr++;
		if (_g_nrsaddr >= MAX_RS) {
			printf("too many real server\n");
			return -1;
		}
	}

	if (!_g_svraddr.family || !_g_nrsaddr) {
		return -1;
	}

	return 0;
}

/**
 *	Signal SIGINT handler, the stop signal.
 */
static void 
_sig_stop(int signo)
{
	if (signo != SIGINT)
		return;

	_g_stop = 1;
}

/**
 *	Initiate some global resources.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_initiate(void)
{	
	/* set stop signal */
	signal(SIGINT, _sig_stop);
	signal(SIGCHLD, SIG_IGN);

	_g_httpfd = sk_tcp_server(&_g_svraddr);
	if (_g_httpfd < 0)
		return -1;

	sk_set_nonblock(_g_httpfd, 1);

	return 0;
}

/**
 *	Release global resources alloc by _initiate().
 *
 *	No return.
 */
static void 
_release(void)
{
	if (_g_httpfd > 0) {
		close(_g_httpfd);
	}

	return;
}

static int 
_make_child(void)
{
	int i;
	proxy_arg_t arg;

	arg.httpfd = _g_httpfd;
	arg.svraddr = _g_svraddr;
	memcpy(arg.rsaddrs, _g_rsaddrs, sizeof(ip_port_t) * _g_nrsaddr);
	arg.nrsaddr = _g_nrsaddr;
	arg.use_splice = _g_splice ? 1 : 0;
	arg.use_nb_splice = _g_nb_splice ? 1 : 0;
	arg.maxsess = _g_capacity;

	for (i = 0; i < _g_nchild; i++) {
		arg.index = i;
		cproc_create(&_g_childs[i], &arg);
	}

	/* close listen fd */
	close(_g_httpfd);
	_g_httpfd = -1;

	return 0;
}

static int 
_stop_child(void)
{
	int i;

	for (i = 0; i < _g_nchild; i++) {
		if (_g_childs[i].pid > 0) 
			cproc_destroy(&_g_childs[i]);
	}

	return 0;
}

static void 
_do_loop(void)
{
	_make_child();


	while (!_g_stop) {
		sleep(1);
	}

	_stop_child();
}

/**
 *	The main entry of program.
 */
int main(int argc, char **argv)
{
	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_initiate()) {
		_release();
		return -1;
	}

	_do_loop();
	
	_release();
	    
	return 0;
}

