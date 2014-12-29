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

static ip_port_t	_g_svraddr;	/* proxy address */
static ip_port_t	_g_rsaddrs[MAX_RS]; /* real server address */
static int		_g_nrsaddr;	/* number of real server address */
static int		_g_capacity = 10;	/* the proxy capacity */
static int		_g_splice = 0;
static int 		_g_nb_splice = 0;

/**
 *	Show help information
 *
 *	No return.
 */
static void _usage(void)
{
	printf("proxy <options>\n");
	printf("\t-a\tproxy address\n");
	printf("\t-c\tproxy concurrency capacity\n");
	printf("\t-s\tuse system splice\n");
	printf("\t-n\tuse nb_splice\n");
	printf("\t-h\tshow help message\n");
}


/**
 *	Parse command line argument.
 *
 *	Return 0 if parse success, -1 on error.
 */
static int _parse_cmd(int argc, char **argv)
{
	char optstr[] = ":a:snc:h";
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

	proxy_main(&_g_svraddr, _g_rsaddrs, _g_nrsaddr, 
		   _g_capacity, _g_splice, _g_nb_splice);
	
	_release();
	    
	return 0;
}

