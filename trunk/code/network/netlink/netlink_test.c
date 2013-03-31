/**
 *	@file	netlink_test.c
 *
 *	@brief	netlink test program
 *
 *	@author	Forrest.zhang
 *	
 *	@date	2013-03-31
 */


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/types.h>


static char	_g_src[128];
static char	_g_dst[128];
static int	_g_iif = 0;
static int	_g_oif = 0;
static char 	_g_gateway[128];
static char	_g_addr[128];
static char	_g_mac[12];
static char	_g_optstr[] = ":t:adlfA:M:S:I:D:O:G:Ph";

/**
 *	Show help message	
 *	
 *	No return.
 */
static void 
_usage(void)
{
	printf("netlink_test <neigh|addr|route> <options>\n");
	printf("\tneigh\tneighbor function\n");
	printf("\taddr\taddress function\n");
	printf("\troute\troute function\n");
	printf("\t-t\tfamily type: AF_INET | AF_INET6\n");
	printf("\t-a\tadd object\n");
	printf("\t-d\tdelete object\n");
	printf("\t-l\tlist all objects\n");
	printf("\t-f\tflush all objects\n");
	printf("\t-A\t(addr)address/cidr, (neigh)address\n");
	printf("\t-M\t(neigh)MAC address\n");
	printf("\t-S\t(route)source address/cidr\n");
	printf("\t-I\t(route)input interface index, (neigh|addr) interface index\n");
	printf("\t-D\t(route)destination address/cidr\n");
	printf("\t-O\t(route)output interface index\n");
	printf("\t-G\t(route)gateway address\n");
	printf("\t-P\t(route)priority\n");
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

	if (argc < 2)
		return -1;

	if (stcmp(argv[1], "neigh")) {
		_g_type = _NL_NEIGH;
	}
	else if (strcmp(argv[1], "addr")) {
		_g_type = _NL_ADDR;
	}
	else if (strcmp(argv[1], "route")) {
		_g_type = _NL_ROUTE;
	}
	else
		return -1;
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, _g_optstr)) != -1) {
		
		switch (opt) {

		case 't':
			if (strcmp()
			
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



/**
 *	@file	
 *
 *	@brief
 *	
 *	@author	Forrest.zhang	
 *
 *	@date
 */






