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

#include "proxy.h"
#include "childproc.h"
#include "sock_util.h"

static proxy_arg_t	_g_arg;
static int		_g_stop = 0;	/* stop policy */
static cproc_t		_g_childs[MAX_CHILD];/* child processes */
static int		_g_nchild = 4;	/* number of child processes */

/**
 *	Show help information
 *
 *	No return.
 */
static void 
_usage(void)
{
	printf("proxyd <options>\n");
	printf("\t-p\t\t\tpolicy address, format is <vs>,<rs>[,<rs>...]\n");
	printf("\t\t\t\t<vs> is listen address, format <IP:port>\n");
	printf("\t\t\t\t<rs> is real server address, format <IP:port>\n");
	printf("\t-m\t\t\tproxy max concurrency\n");
	printf("\t-w\t\t\tthe child process number\n");
	printf("\t-b <rr|odd|even>\tbind cpu algorithm\n");
	printf("\t-r <full|low|high>\tbind cpu range, full,low, high\n");
	printf("\t-d <0-4>\t\tthe debug level<0-4>, 0 is disable\n");
	printf("\t-s\t\t\tuse system splice\n");
	printf("\t-n\t\t\tuse nb_splice\n");
	printf("\t-h\t\t\tshow help message\n");
}

static int 
_parse_addr(char *addr)
{
	policy_t *pl;
	char *ptr;
	char *begin;
	int i;

	if (!addr)
		return -1;

	if (_g_arg.npolicy >= MAX_POLICY) {
		printf("too many policies, exceed %d\n", MAX_POLICY);
		return -1;
	}

	pl = &_g_arg.policies[_g_arg.npolicy];

	begin = addr;
	ptr = strchr(begin, ',');
	if (!ptr) {
		printf("miss real server address\n");
		return -1;
	}

	/* get listen address */
	*ptr = 0;
	if (ip_port_from_str(&pl->httpaddr, begin)) {
		printf("invalid listen address %s\n", addr);
		return -1;
	}

	/* check exist real server address */
	begin = ptr + 1;
	if (*begin == 0) {
		printf("not real server address\n");
		return -1;
	}

	i = 0;
	while (1) {
		ptr = strchr(begin, ',');

		/* last real server address */
		if (ptr == NULL) {
			if (ip_port_from_str(&pl->rsaddrs[i], begin)) {
				printf("invalid real server address %s", begin);
				return -1;
			}
			i++;
			break;
		}

		*ptr = 0;
		if (ip_port_from_str(&pl->rsaddrs[i], begin)) {
			printf("invalid real server address %s", begin);
			return -1;
		}
		i++;
		begin = ptr + 1;
		if (*begin == 0)
			break;
	}

	if (i == 0) {
		printf("no real server address\n");
		return -1;
	}

	pl->nrsaddr = i;
	pl->index = _g_arg.npolicy;
	_g_arg.npolicy++;

	return 0;
}

/**
 *	Parse command line argument.
 *
 *	Return 0 if parse success, -1 on error.
 */
static int 
_parse_cmd(int argc, char **argv)
{
	char optstr[] = ":p:m:w:b:r:d:sbh";
	char opt;
	char *addr;

	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		switch (opt) {
			
		case 'p':
			addr = strdup(optarg);
			if (_parse_addr(addr)) {
				free(addr);
				printf("invalid policy\n");
				return -1;
			}
			free(addr);

			break;
			
		case 'm':
			_g_arg.max = atoi(optarg);
			if (_g_arg.max < 1) {
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

		case 'b':
			if (strcmp(optarg, "rr") == 0)
				_g_arg.bind_cpu_algo = CPROC_BIND_RR;
			else if (strcmp(optarg, "odd") == 0)
				_g_arg.bind_cpu_algo = CPROC_BIND_ODD;
			else if (strcmp(optarg, "even") == 0)
				_g_arg.bind_cpu_algo = CPROC_BIND_EVEN;
			else {
				printf("invalid bind CPU algorithm %s\n", optarg);
				return -1;
			}
			_g_arg.bind_cpu = 1;
			break;

		case 'r':
			if (strcmp(optarg, "full") == 0)
				_g_arg.bind_cpu_ht = CPROC_HT_FULL;
			else if (strcmp(optarg, "low") == 0)
				_g_arg.bind_cpu_ht = CPROC_HT_LOW;
			else if (strcmp(optarg, "high") == 0)
				_g_arg.bind_cpu_ht = CPROC_HT_HIGH;
			else {
				printf("invalid bind CPU ht %s\n", optarg);
				return -1;
			}
			_g_arg.bind_cpu = 1;
			break;

		case 'd':
			_g_arg.dbglvl = atoi(optarg);
			if (_g_arg.dbglvl < 0 || _g_arg.dbglvl > 4) {
				printf("invalid debug level: %s\n", optarg);
				return -1;
			}
			break;

		case 's':
			_g_arg.use_splice = 1;
			break;

		case 'n':
			_g_arg.use_nb_splice = 1;
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

	if (optind != argc) {
		return -1;
	}

	if (_g_arg.npolicy < 1) {
		return -1;
	}

	if (!_g_arg.max)
		_g_arg.max = 100;

	_g_arg.max = _g_arg.max / _g_nchild + 1;

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

	printf("proxyd recv stop signal\n");

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

	if (_g_arg.max == 0)
		_g_arg.max = 10;

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
	return;
}

static int 
_make_child(void)
{
	int i;
	proxy_arg_t arg;

	memcpy(&arg, &_g_arg, sizeof(proxy_arg_t));

	for (i = 0; i < _g_nchild; i++) {
		arg.index = i;
		cproc_create(&_g_childs[i], &arg);
	}

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

