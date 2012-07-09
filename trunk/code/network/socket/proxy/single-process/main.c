/*
 *	@file	main.c
 *
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include "proxy.h"

static u_int32_t _g_proxy_ip;		/* the Proxy IP */
static u_int32_t _g_svr_ip;		/* the real server IP */
static u_int16_t _g_proxy_port;		/* the Proxy port */
static u_int16_t _g_svr_port;		/* the real server port */
static int _g_proxy_capacity = 10;	/* the proxy capacity, how many concurrency connections */

/**
 *	Show help information
 *
 *	No return.
 */
static void _usage(void)
{
	printf("proxy <options>\n");
	printf("\t-a\tproxy address\n");
	printf("\t-p\tproxy port\n");
	printf("\t-r\tserver address\n");
	printf("\t-d\tserver port\n");
	printf("\t-c\tproxy concurrency capacity\n");
	printf("\t-h\tshow help message\n");
}


/**
 *	Parse command line argument.
 *
 *	Return 0 if parse success, -1 on error.
 */
static int _parse_cmd(int argc, char **argv)
{
	char optstr[] = ":a:p:r:d:c:h";
	char opt;
	int port = 0;

	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		switch (opt) {
			
		case 'a':
			if (inet_aton(optarg, (struct in_addr *)&_g_proxy_ip) < 0) {
				printf("invalid proxy address: %s\n", optarg);
				return -1;
			}
			
			break;
			
		case 'p':
			port = atoi(optarg);
			if (port < 1 || port > 65535) {
				printf("invalid proxy port<1-65535>: %s\n", optarg);
				return -1;
			}
			_g_proxy_port = port;
			break;
			
		case 'r':
			if (inet_aton(optarg, (struct in_addr *)&_g_svr_ip) < 0) {
				printf("invalid real server address: %s\n", optarg);
				return -1;
			}
			
			break;
			
		case 'd':
			port = atoi(optarg);
			if (port < 1 || port > 65535) {
				printf("invalid server port<1-65535>: %s\n", optarg);
				return -1;
			}
			_g_svr_port = port;
			break;

		case 'c':
			_g_proxy_capacity = atoi(optarg);
			if (_g_proxy_capacity < 1) {
				printf("invalid proxy capacity<positive integer>: %s\n", optarg);
				return -1;
			}
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

	if (!_g_proxy_ip || !_g_svr_ip) {
		return -1;
	}

	return 0;
}

/**
 *	Initiate some global resources.
 *
 *	Return 0 if success, -1 on error.
 */
static int _initiate(void)
{
	return 0;
}

/**
 *	Release global resources alloc by _initiate().
 *
 *	No return.
 */
static void _release(void)
{
	return;
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

	proxy_main(_g_proxy_ip, _g_proxy_port, _g_svr_ip, 
		   _g_svr_port, _g_proxy_capacity);

	_release();
	    
	return 0;
}

