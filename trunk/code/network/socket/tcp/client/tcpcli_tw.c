/**
 *	@file	tcpcli_tw.c
 *
 *	@brief	TCP client timewait test program.
 *
 *	@author	Forrest.zhang
 *	
 *	@date	2010-01-20
 */


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "sock.h"

static u_int32_t	_g_svrip = 0;
static u_int16_t	_g_svrport = 0;


/**
 *	Show help message	
 *	
 *	No return.
 */
static void 
_usage(void)
{
	printf("tcpcli_tw <options>\n");
	printf("\t-i\tthe server's IP address\n");
	printf("\t-p\thte server's port\n");
	printf("\t-h\tshow help message\n");
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
	char optstr[] = ":i:p:h";
	int port = 0;
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {
			
		case 'i':
			_g_svrip = inet_addr(optarg);
			break;

		case 'p':
			port = atoi(optarg);
			if (port < 1 || port > 65535) {
				printf("The tcp port out of range(1-65535)\n");
				return -1;
			}
			_g_svrport = htons((u_int16_t)port);
			break;

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

/**
 *	Init some global resource used in program.	
 *
 * 	Return 0 if success, -1 on error.
 */
static int 
_initiate(void)
{
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
	int fd;

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_initiate()) {
		return -1;
	}

	fd = sock_tcpcli(_g_svrip, _g_svrport);

//	sleep(5);
	close(fd);

	_release();

	return 0;
}



