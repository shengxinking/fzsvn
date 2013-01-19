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

static ip_port_t	_g_svraddr;	/* server address */
static ip_port_t	_g_rsaddrs[MAX_REALSVR];/* real server address */
static int		_g_nrsaddr;	/* number of real server */
static int		_g_curconn = 50;
static int		_g_nwork = 8;
static int		_g_splice = 0;
static int		_g_nb_splice = 0;

static void _usage(void)
{
	printf("tcpproxy <options> <(realsvr address(ip:port)>\n");
	printf("\t-a\tproxy address\n");
	printf("\t-s\tuse splice\n");
	printf("\t-n\tuse nb_splice\n");
	printf("\t-w\twork thread number\n");
	printf("\t-c\tproxy max concurrency\n");
	printf("\t-h\tshow help usage\n");
}


static int _parse_cmd(int argc, char **argv)
{
	char c;
	char optstr[] = ":a:snw:c:h";
	int i;

	opterr = 0;
	while ( (c = getopt(argc, argv, optstr)) != -1) {
		switch (c) {

		case 'a':
			if (ip_port_from_str(&_g_svraddr, optarg)) {
				printf("invalid proxy address: %s\n", 
				       optarg);
				return -1;
			}
			break;

		case 's':
			_g_splice = 1;
			break;

		case 'n':
			_g_nb_splice = 1;
			break;

		case 'w':
			_g_nwork = atoi(optarg);
			if (_g_nwork < 1 || _g_nwork > MAX_WORKNUM) {
				printf("invalid work number: %s\n", 
				       optarg);
				return -1;
			}
			break;

		case 'c':
			_g_curconn = atoi(optarg);
			if (_g_curconn < 1) {
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

	if (optind >= argc) {
		printf("no real server\n");
		return -1;
	}
	
	if (argc - optind > MAX_REALSVR) {
		printf("too many realsvrs, excceed %d\n", MAX_REALSVR);
		return -1;
	}

	for (i = optind; i < argc; i++) {
		if (ip_port_from_str(&_g_rsaddrs[i-optind], argv[i])) {
			printf("invalid real server: %s\n", argv[i]);
			return -1;
		}
		_g_nrsaddr++;
	}

	if (_g_svraddr.family == 0) {
		printf("no server address\n");
		return -1;
	}

	if (_g_splice && _g_nb_splice) {
		printf("the splice and nb_splice can set only one\n");
		return -1;
	}

	return 0;
}

int main(int argc, char **argv)
{
	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	proxy_main(&_g_svraddr, _g_rsaddrs, _g_nrsaddr,
		   _g_curconn, _g_nwork, _g_splice, _g_nb_splice);

	return 0;
}




