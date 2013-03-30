/**
 *	@file	netopt_test.c
 *
 *	@brief	netopt test program
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

#include "netopt.h"

static char _g_optstr[] = ":sph";
static int _g_type;

enum {
	_TEST_SYS,
	_TEST_PROC,
};

/**
 *	Show help message	
 *	
 *	No return.
 */
static void 
_usage(void)
{
	printf("netopt_test <options>\n");
	printf("\t-s\ttest nets_xxx APIs\n");
	printf("\t-p\ttest netp_xxx APIs\n");
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
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, _g_optstr)) != -1) {
		
		switch (opt) {

		case 's':
			_g_type = _TEST_SYS;
			break;

		case 'p':
			_g_type = _TEST_PROC;
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

static int 
_nets_test(void)
{
	const char *ifname = "eth0";

	printf("eth0 vendor id: 0x%x\n", nets_get_vendor_id(ifname));
	printf("eth0 device id: 0x%x\n", nets_get_device_id(ifname));

	return 0;
}

static int 
_netp_test(void)
{
	char buf[32] = {0};
	const char *ifname = "default";
	int ret = 1;

	printf("ip_forward: %d\n", netp_get_ip4_forward());
	if (netp_set_ip4_forward(0)) {
		printf("set ip_forward failed\n");
		return -1;
	}
	printf("ip_forward: %d\n", netp_get_ip4_forward());

	ret = netp_get_ip4_promote_secondaries(ifname); 
	if (ret < 0){
		printf("netp_get_ip4_promote_secondaries failed");
		return -1;
	}
	printf("promote secondaries: %d\n", ret);

	memset(buf, 0, sizeof(buf));
	if (netp_get_ip4_local_port_range(buf, sizeof(buf))) {
		printf("netp_get_ip4_local_port_range failed\n");
		return -1;
	}
	printf("local_port_range: %s\n", buf);

	if (netp_set_ip4_local_port_range("32768	61000")) {
		printf("write local port range failed\n");
		return -1;
	}

	memset(buf, 0, sizeof(buf));
	if (netp_get_tcp4_mem(buf, sizeof(buf))) {
		printf("netp_get_tcp4_mem failed\n");
		return -1;
	}
	printf("tcp_mem: %s\n", buf);
	
	memset(buf, 0, sizeof(buf));
	if (netp_get_tcp4_rmem(buf, sizeof(buf))) {
		printf("netp_get_tcp4_rmem failed\n");
		return -1;
	}
	printf("tcp_rmem: %s\n", buf);

	memset(buf, 0, sizeof(buf));
	if (netp_get_tcp4_wmem(buf, sizeof(buf))) {
		printf("netp_get_tcp4_wmem failed\n");
		return -1;
	}
	printf("tcp_wmem: %s\n", buf);

	printf("tw_reuse: %d\n", netp_get_tcp4_tw_reuse());
	printf("tw_recyle: %d\n", netp_get_tcp4_tw_recycle());
	printf("tw_max_buckets: %d\n", netp_get_tcp4_tw_max_buckets());
	printf("fin_timeout: %d\n", netp_get_tcp4_fin_timeout());
	printf("timestamps: %d\n", netp_get_tcp4_timestamps());
	printf("syn_backlog: %d\n", netp_get_tcp4_max_syn_backlog());	

	printf("rmem_default: %d\n", netp_get_rmem_default());
	printf("rmem_max: %d\n", netp_get_rmem_max());
	printf("wmem_default: %d\n", netp_get_wmem_default());
	printf("wmem_max: %d\n", netp_get_wmem_max());
	printf("somaxconn: %d\n", netp_get_somaxconn());

	printf("accept_dad: %d\n", netp_get_ip6_accept_dad(ifname));

	return 0;
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
	
	switch (_g_type) {

	case _TEST_SYS:
		_nets_test();
		break;

	case _TEST_PROC:
		_netp_test();
		break;

	default:
		break;
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






