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
#include <sys/socket.h>

#include "netutil.h"
#include "netlink.h"

enum {
	_NL_NEIGH,
	_NL_ADDR,
	_NL_ROUTE,
};

enum {
	_NL_ADD,
	_NL_DEL,
	_NL_FLUSH,
	_NL_LIST,
};

static char	_g_src[128];
static char	_g_dst[128];
static int	_g_iif = 0;
static int	_g_oif = 0;
static char 	_g_gateway[128];
static char	_g_addr[128];
static char	_g_mac[12];
static char	_g_optstr[] = ":t:adlfA:M:S:I:D:O:G:Ph";
static int	_g_priority = 0;
static int	_g_type;
static int	_g_family = AF_INET;
static int	_g_act = _NL_LIST;


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
	printf("\t-f\tflush all objects\n");
	printf("\t-l\tlist all objects\n");
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

	if (strcmp(argv[1], "neigh") == 0) {
		_g_type = _NL_NEIGH;
	}
	else if (strcmp(argv[1], "addr") == 0) {
		_g_type = _NL_ADDR;
	}
	else if (strcmp(argv[1], "route") == 0) {
		_g_type = _NL_ROUTE;
	}
	else {
		return -1;
	}
	
	opterr = 0;
	while ( (opt = getopt((argc - 1), (argv + 1), _g_optstr)) != -1) {
		
		switch (opt) {
		
		case 't':
			if (strcmp("AF_INET", optarg) == 0)
				_g_family = AF_INET;
			else if (strcmp("AF_INET6", optarg) == 0)
				_g_family = AF_INET6;
			else {
				printf("unknow family %s\n", optarg);
				return -1;
			}
			break;
		
		case 'a':
			_g_act = _NL_ADD;
			break;

		case 'd':
			_g_act = _NL_DEL;
			break;

		case 'f':
			_g_act = _NL_FLUSH;
			break;

		case 'l':
			_g_act = _NL_LIST;
			break;
			
		case 'A':
			strncpy(_g_addr, optarg, 127);
			break;

		case 'M':
			strncpy(_g_mac, optarg, 127);
			break;

		case 'S':
			strncpy(_g_src, optarg, 127);
			break;

		case 'D':
			strncpy(_g_dst, optarg, 127);
			break;

		case 'I':
			_g_iif = atoi(optarg);
			if (_g_iif < 0) {
				printf("invalid interface %s\n", optarg);
				return -1;
			}
			break;

		case 'O':
			_g_oif = atoi(optarg);
			if (_g_oif < 0) {
				printf("invalid interface %s\n", optarg);
				return -1;
			}
			break;

		case 'G':
			strncpy(_g_gateway, optarg, 127);
			break;

		case 'P':
			_g_priority = atoi(optarg);
			if (_g_priority <= 0) {
				printf("invalid priority %s\n", optarg);
				return -1;
			}
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

	if ((argc - 1) != optind)
		return -1;

	return 0;
}

static int 
_neigh_func(void)
{
	return 0;
}

static int 
_addr_print(unsigned long *args)
{
	return 0;
}

static int 
_addr_func(void)
{
	ip_mask_t ipmask;
	int ret = -1;

	switch (_g_act) {
		
	case _NL_ADD:
		if (_g_iif < 1) {
			printf("no interface\n");
			return -1;
		}

		if (ip_mask_from_str(&ipmask, _g_addr)) {
			printf("invalid address/cidr: %s\n", _g_addr);
			return -1;
		}
		
		if (ipmask.family != _g_family) {
			printf("invalid address type\n");
			return -1;
		}
		
		printf("ip4 addr is %p(%u.%u.%u.%u), %p\n", 
		       &ipmask._addr, IP4_QUAD(ipmask._addr.v4.s_addr), 
		       &ipmask._addr.v4.s_addr);

		ret = nl_addr_add(_g_iif, _g_family, &(ipmask._addr), ipmask.cidr);

		break;
		
	case _NL_DEL:
		if (_g_iif < 1) {
			printf("no interface\n");
			return -1;
		}

		if (ip_mask_from_str(&ipmask, _g_addr)) {
			printf("invalid address/cidr: %s\n", _g_addr);
			return -1;
		}
		
		if (ipmask.family != _g_family) {
			printf("invalid address type\n");
			return -1;
		}

		//ret = nl_addr_delete(_g_iif, _g_family, &ipmask._addr, ipmask.cidr);
		break;

	case _NL_FLUSH:
		ret = nl_addr_flush(_g_iif, _g_family);
		break;

	case _NL_LIST:
		ret = nl_addr_list(_g_iif, _g_family, _addr_print, NULL);
		break;
	}

	return ret;
}

static int 
_route_func(void)
{
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
	int ret;

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_initiate()) {
		return -1;
	}

	printf("_g_type %d\n", _g_type);

	switch (_g_type) {

	case _NL_NEIGH:
		ret = _neigh_func();
		break;

	case _NL_ADDR:
		ret = _addr_func();
		break;

	case _NL_ROUTE:
		ret = _route_func();
		break;

	default:
		printf("invalid type\n");
		ret = -1;
	}

	_release();

	return ret;
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






