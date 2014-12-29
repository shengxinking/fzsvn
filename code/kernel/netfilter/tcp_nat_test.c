/**
 *	@file	tcp_nat_test.c
 *
 *	@brief	tcp_nat module test program.
 *
 *	@author	Forrest.zhang
 *	
 *	@date	2012-09-20
 */


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/types.h>

#include "tcp_nat.h"

static u_int32_t	_g_ip;
static u_int16_t	_g_port;
static int		_g_cmd;

/**
 *	Show help message	
 *	
 *	No return.
 */
static void 
_usage(void)
{
	printf("tcp_nat_test <options>\n");
	printf("\t-a\t add ip\n");
	printf("\t-d\t delete ip\n");
	printf("\t-c\t clear all ip\n");
	printf("\t-g\t get ip\n");
	printf("\t-s\t set port\n");
	printf("\t-h\t show help message\n");
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
	char optstr[] = ":a:d:c:g:s:h";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {
			
		case 'a':
			_g_cmd = TCP_NAT_ADD_IP;
			_g_ip = inet_addr(optarg);
			break;
			
		case 'd':
			_g_cmd = TCP_NAT_DEL_IP;
			_g_ip = inet_addr(optarg);
			break;
			
		case 'g':
			_g_cmd = TCP_NAT_GET_IP;
			break;
		case 'c':
			_g_cmd = TCP_NAT_CLEAR_IP;
			break;
		case 's':
			_g_cmd = TCP_NAT_SET_PORT;
			_g_port = inet_addr(optarg);
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
	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_initiate()) {
		return -1;
	}

	_release();

	return 0;
}



