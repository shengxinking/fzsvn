/**
 *	@file	main.c
 *	@brief	the main function of tcpproxy, it read command line parameter
 *		then run a proxy. 
 * 
 *	@author	FZ
 */


#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "debug.h"
#include "proxy.h"

static u_int32_t _proxy_ip;
static u_int32_t _proxy_rip;
static u_int16_t _proxy_port;
static u_int16_t _proxy_sport;
static u_int16_t _proxy_rport;
static u_int16_t _proxy_rsport;
static int	 _capacity = 50;
static int	 _work_num = 1;

static void _usage(void)
{
	printf("tcpproxy <options> <(realsvr address(ip:port)>\n");
	printf("\t-a\tproxy address\n");
	printf("\t-p\tproxy HTTP port\n");
	printf("\t-s\tproxy HTTPS port");
	printf("\t-n\twork thread number\n");
	printf("\t-c\tproxy capacity, the max concurrency connections\n");
	printf("\t-h\tshow help usage\n");
}


static int _parse_cmd(int argc, char **argv)
{
	char c;
	char optstr[] = ":a:p:s:n:c:h";
	int port;

	opterr = 0;
	while ( (c = getopt(argc, argv, optstr)) != -1) {
		switch (c) {

		case 'a':
			_proxy_ip = inet_addr(optarg);
			if (_proxy_ip == INADDR_NONE) {
				printf("invalid proxy ip address: %s\n", 
				       optarg);
				return -1;
			}
			break;

		case 'p':
			port = atoi(optarg);
			if (port < 1 || port > 65535) {
				printf("invalid proxy port(1-65535): %s\n", 
				       optarg);
				return -1;
			}
			_proxy_port = htons((unsigned short)port);
			break;

		case 's':
			port = atoi(optarg);
			if (port < 1 || port > 65535) {
				printf("invalid proxy port(1-65535): %s\n", 
				       optarg);
				return -1;
			}
			_proxy_sport = htons((unsigned short)port);
			break;

		case 'n':
			_work_num = atoi(optarg);
			if (_work_num < 1 || _work_num > 65535) {
				printf("invalid work thread number(1-20): %s\n", 
				       optarg);
				return -1;
			}
			break;

		case 'c':
			_capacity = atoi(optarg);
			if (_capacity < 1) {
				printf("invalid capacity: %s\n", optarg);
				return -1;
			}
			break;

		case 'h':
			_usage();
			exit(0);

		case ':':
			printf("%c need argument\n", optopt);
			return -1;

		case '?':
			printf("unkowned option: %c\n", optopt);
			return -1;
		}
	}

	if (optind >= argc)
		return -1;

	if ((argc - optind) != 2)
		return -1;

	_proxy_rip = inet_addr(argv[optind]);
	if (_proxy_ip == INADDR_NONE) {
		printf("invalid proxy ip address: %s\n", optarg);
		return -1;
	}

	_proxy_rport = htons(atoi(argv[optind + 1]));

	return 0;
}

int main(int argc, char **argv)
{

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	DBG("proxy: addr %u.%u.%u.%u:%u, real %u.%u.%u.%u:%u\n", 
	    NIPQUAD(_proxy_ip), ntohs(_proxy_port),
	    NIPQUAD(_proxy_rip), ntohs(_proxy_rport));
	
	proxy_main(_proxy_ip, _proxy_port, _proxy_sport, 
		   _proxy_rip, _proxy_rport, _proxy_rsport, 
		   _capacity, _work_num);

	return 0;
}




