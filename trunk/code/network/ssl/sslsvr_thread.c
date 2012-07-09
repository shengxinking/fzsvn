/**
 *	@file	sslsvr_thread.c
 *
 *	@brief	Create a thread to handle SSL connect.
 *
 *	@author	Forrest.zhang
 *	
 *	@date	2010-01-21
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
	printf("sslsvr_thread a multithread ssl server\n");
	printf("it'll echo client's data\n");
	printf("\nusage: sslsvr_thread <options>\n");
	printf("\n\t-i\tserver's IP, if not assigned, using all IP\n");
	printf("\t-p\tserver' port, if not set, using 8443\n");
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
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {
			
		case 'i':
			_g_svrip = inet_addr(optarg);
			break;

		case 'p':
			port = atoi(optarg);
			if (port < 1 || port > 65535) {
				printf("server port out of range(1-65535)\n");
				return -1;
			}
			_g_svrport = (u_int16)port;
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

	if (_g_svrport == 0) {
		_g_svrport = htons(8443);
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
	_g_svrfd = sock_tcpsvr(_g_svrip, _g_svrport, 10);
	if (_g_svrfd < 0) {
		printf("create socket failed\n");
		return -1;
	}
	
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
	if (_g_svrfd > 0)
		close(_g_svrfd);
}

static void *thread_main(void *arg)
{
	int clifd;

	clifd = *((int*)arg);

	printf("thread %lu is running, clifd %d\n");

	sslsock_accept(fd, ssl
}


static int 
_do_loop(void)
{
	sslsock_t *ss;
	sslsock_t *cliss;

	ss = sslsock_server(_g_svrip, _g_svrport);
	if (!ss) {
		return -1;
	}

	while (!_g_stop) {
		ss = sslsock_accept();
			
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

	_release();

	return 0;
}



