/**
 *	@file	ip_addr_test.c	
 *
 *	@brief	test program for ip_addr APIs.
 *
 *	@author	Forrest.zhang
 *	
 *	@date
 */


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/types.h>

#include "ip_addr.h"

enum {
	TEST_IP_ADDR,
	TEST_IP_MASK,
	TEST_IP_PORT,
};

static int	_g_test = TEST_IP_ADDR;


/**
 *	Show help message	
 *	
 *	No return.
 */
static void 
_usage(void)
{
	printf("ip_addr_test <options>\n");
	printf("\t-a\ttest ip_addr functions\n");
	printf("\t-m\ttest ip_mask functions\n");
	printf("\t-p\ttest ip_port functions\n");
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
	char optstr[] = ":amph";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {
			
		case'a':
			_g_test = TEST_IP_ADDR;
			break;

		case 'm':
			_g_test = TEST_IP_MASK;
			break;
			
		case 'p':
			_g_test = TEST_IP_PORT;
			break;

		case 'h':
			return -1;

		case ':':
			printf("Option %c missing argument\n", 
			       optopt);
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

static void 
_test_ip_addr(void)
{

}


static void 
_test_ip_mask(void)
{

}

static void 
_test_ip_port(void)
{
	ip_port_t ip;
	char buf[1024] = {0};
	size_t len = sizeof(buf);;
	
	IP_PORT_SET_V4(&ip, 0x11111111, 8080);

	if (ip_port_from_str(&ip, "172.22.14.60:8080")) {
		printf("ip_port_from_str failed\n");
	}
	else {
		printf("%s\n", ip_port_to_str(&ip, buf, len));
	}

	if (ip_port_from_str(&ip, "[::ffff:172.22.14.61]:8090"))
		printf("ip_port_from_str failed\n");
	else
		printf("%s\n", ip_port_to_str(&ip, buf, len));
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

	switch (_g_test) {

	case TEST_IP_ADDR:
		_test_ip_addr();
		break;

	case TEST_IP_MASK:
		_test_ip_mask();
		break;

	case TEST_IP_PORT:
		_test_ip_port();
		break;
		
	default:
		break;
	}

	_release();

	return 0;
}



