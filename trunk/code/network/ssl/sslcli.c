/*
 *	ssl_client.c:	a simple SSL client
 *
 *	author:		forrest.zhang
 */
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "sslsock.h"

#ifndef	BUFLEN
#define BUFLEN		1024
#endif

static u_int16_t	_g_port = 8443;
static u_int32_t	_g_ip = 0; 
static sslsock_ctx_t	*_g_sslctx = NULL;


static void 
_usage(void)
{
	printf("sslcli <options>\n");
	printf("\t-i\tserver IP address\n");
	printf("\t-p\tserver port\n");
	printf("\t-h\tshow help info\n");
}


static int 
_parse_cmd(int argc, char **argv)
{
	char c;
	const char optstr[] = ":i:p:h";
	int port;	

	opterr = 0;
	while ( (c = getopt(argc, argv, optstr)) != -1) {
		
		switch (c) {

		case 'i':
			_g_ip = inet_addr(optarg);
			break;

		case 'p':
			port = atoi(optarg);
			if (port < 1 || port > 65535) {
				printf("-p <1-65535>\n");
				return -1;
			}
			_g_port = port;
			break;

		case 'h':
			_usage();
			exit(0);
		
		case ':':
			printf("option %c miss argument\n", optopt);
			return -1;
		
		case '?':
			printf("unknow option %c\n", optopt);
			return -1;
		}

	}

	if (optind != argc)
		return -1;

	if (_g_ip == 0 || _g_port == 0)
		return -1;

	return 0;
}

static int 
_initiate(void)
{
	_g_sslctx = ss_ctx_alloc(SS_CLIENT, SS_VERIFY_NONE);
	if (!_g_sslctx)
		return -1;

	return 0;
}


static void
_release(void)
{
	if (_g_sslctx) {
		ss_ctx_free(_g_sslctx);
	}

	_g_sslctx = NULL;
}

static int 
_do_client(void)
{
	char buf[BUFLEN] = "hello, world";
	sslsock_t *ss;
	int closed = 0;
	
	ss = ss_client(_g_sslctx, _g_ip, _g_port);
	if (!ss) 
		return -1;

	ss_send(ss, buf, strlen(buf));
	printf("send to server: %s\n", buf);

	memset(buf, 0, sizeof(buf));
	ss_recv(ss, buf, sizeof(buf), &closed);
	printf("recv from server: %s\n", buf);

	if (closed == 1) {
		printf("server closed\n");
	}

	ss_free(ss);

	return 0;
}	


int 
main(int argc, char **argv)
{

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}	

	if (_initiate()) {
		_release();
		return -1;
	}

	_do_client();

	_release();
	
	return 0;
}











