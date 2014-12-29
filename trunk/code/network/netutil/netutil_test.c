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
#include "sock_util.h"

enum {
	TEST_IP_ADDR,
	TEST_IP_MASK,
	TEST_IP_PORT,
	TEST_SK_UTIL,
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
	printf("netutil_test <options>\n");
	printf("\t-a\ttest ip_addr functions\n");
	printf("\t-m\ttest ip_mask functions\n");
	printf("\t-p\ttest ip_port functions\n");
	printf("\t-s\ttest sk_util functions\n");
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
	char optstr[] = ":ampsh";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {
			
		case 'a':
			_g_test = TEST_IP_ADDR;
			break;

		case 'm':
			_g_test = TEST_IP_MASK;
			break;
			
		case 'p':
			_g_test = TEST_IP_PORT;
			break;

		case 's':
			_g_test = TEST_SK_UTIL;
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
	char ip4_str[IP_STR_LEN] = "10.200.4.12";
	char ip6_str[IP_STR_LEN] = "10:200::4:12";
	char ipstr[IP_STR_LEN] = {0};
	ip_addr_t ip4, ip6;
	
	if (ip_addr_from_str(&ip4, ip4_str)) {
		printf("%s ip_addr_from_str failed\n", ip4_str);
		return;
	}

	ip_addr_apply_mask(&ip4, 16);

	printf("after apply mask, address is %s\n", 
	       ip_addr_to_str(&ip4, ipstr, IP_STR_LEN));

	if (ip_addr_from_str(&ip6, ip6_str)) {
		printf("%s ip_addr_from_str failed\n", ip6_str);
		return;
	}

	ip_addr_apply_mask(&ip6, 24);

	printf("after apply mask, address is %s\n", 
	       ip_addr_to_str(&ip6, ipstr, IP_STR_LEN));
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

static void 
_test_sk_util(void)
{
	char domain_v4[] = "pc111111111122222222223333333333444444444455555555556666666666.autotest.com";
	char domain_v6[] = "www.google.com.hk";
	ip_addr_t ip4, ip6;
	ip_port_t laddr;
	int fd;
	char ipstr[IP_STR_LEN] = {0};

	if (ip_port_from_str(&laddr, "[::1]:8080")) {
		printf("ip_port from str failed\n");
		return;
	}
	fd = sk_tcp_server(&laddr, 0, 0);
	if (fd < 0) {
		printf("create server socket failed\n");
		return;
	}

	if (sk_gethostbyname(domain_v6, AF_INET6, &ip6)) {
		printf("%s sk_gethostbyname failed\n", domain_v6);
	}
	else {
		printf("%s address(6) is %s\n", domain_v6, 
		       ip_addr_to_str(&ip6, ipstr, IP_STR_LEN));
	}

	if (sk_gethostbyname(domain_v4, AF_INET, &ip4)) {
		printf("%s sk_gethostbyname(4) failed\n", domain_v4);
	} else {
		printf("%s address(4) is %s\n", domain_v4, 
		       ip_addr_to_str(&ip4, ipstr, IP_STR_LEN));
	}

	return ;
	
	if (sk_gethostbyname(domain_v6, 0, &ip6)) {
		printf("%s sk_gethostbyname(0) failed\n", domain_v6);
	}
	else {
		printf("%s address(0) is %s\n", domain_v6, 
		       ip_addr_to_str(&ip6, ipstr, IP_STR_LEN));
	}

	if (sk_gethostbyname(domain_v4, 0, &ip4)) {
		printf("%s sk_gethostbyname(0) failed\n", domain_v4);
	} else {
		printf("%s address(0) is %s\n", domain_v4, 
		       ip_addr_to_str(&ip4, ipstr, IP_STR_LEN));
	}
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
		
	case TEST_SK_UTIL:
		_test_sk_util();
		break;
		
	default:
		break;
	}

	_release();

	return 0;
}



