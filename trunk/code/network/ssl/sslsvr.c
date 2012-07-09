/**
 *	@file	sslsvr.c
 *	@brief	SSL server program, it's a simple SSL server just to 
 *		print the Client's data sending to it.
 *	@author	Forrest.zhang
 */

#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/crypto.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include "sslsock.h"

#define CLIENT_MAX	10

#ifndef BUFLEN
#define	BUFLEN		63
#endif

static u_int32_t	_g_ip = 0;
static u_int16_t	_g_port = 8443;
static sslsock_ctx_t	*_g_sslctx;
static sslsock_t	*_g_socks[CLIENT_MAX];
static int		_g_nsock = 0;
static char		_g_password[BUFLEN + 1];
static char		_g_cert[BUFLEN + 1];
static char		_g_key[BUFLEN + 1];


/**
 *	Show usage of program.
 *
 *	No return value.
 */
static void 
_usage(void)
{
	printf("ssl_server <options>\n\n");
	printf("\t-i\tserver ip address\n");
	printf("\t-p\tserver port\n");
	printf("\t-c\tserver certificate\n");
	printf("\t-k\tserver private key\n");
	printf("\t-w\tserver certificate password\n");
	printf("\t-h\tshow help message\n");
}

/**
 *	Parse the command line arguments, and stored them into local variables
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_parse_cmd(int argc, char **argv)
{
	char optstr[] = ":i:p:c:k:w:t:h";
	char c;
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
				printf("port <1 - 65535>\n");
				return -1;
			}
			_g_port = htons(port);
			break;

		case 'c':
			strncpy(_g_cert, optarg, BUFLEN);
			break;

		case 'k':
			strncpy(_g_key, optarg, BUFLEN);
			break;

		case 'w':
			strncpy(_g_password, optarg, BUFLEN);
			break;

		case 'h':
			_usage();
			exit(0);

		case ':':
			printf("option %c need parameter\n", optopt);
			return -1;
		
		case '?':
			printf("unknowed option %c\n", optopt);
			return -1;
		}
	}

	if (optind < argc)
		return -1;
	
	if (strlen(_g_cert) < 1)
		return -1;

	return 0;
}


/**
 *	Initiate global resource used in program.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_initiate(void)
{
	_g_sslctx = ss_ctx_alloc(SS_SERVER, SS_VERIFY_NONE);
	if (!_g_sslctx)
		return -1;
		
	return 0;
}


/**
 *	Free global resources used in program.
 *
 *	No return.
 */
static void 
_release(void)
{
	if (_g_sslctx)
		ss_ctx_free(_g_sslctx);
	_g_sslctx = NULL;
}


/**
 *	Accept client ssl connection, and store the socket fd into @_g_conns.
 *
 *	Return 0 if success, -1 on error.
 */
static int _do_accept(sslsock_t *ss)
{
	int i;
	sslsock_t *sk;

	while (1) {
		sk = ss_accept(ss, 0);
		if (!sk) {
			printf("ss_accept error: %s\n", strerror(errno));
			return -1;
		}

		if (_g_nsock >= CLIENT_MAX) {
			printf("too many client connect to SSL server\n");
			ss_free(sk);
			return -1;
		}

		printf("a client connect to me\n");

		for (i = 0; i < CLIENT_MAX; i++) {
			if (_g_socks[i] == NULL) {
				_g_socks[i] = sk;
				_g_nsock ++;
				break;
			}
		}

		if (i == CLIENT_MAX) {
			printf("can't find a free entry\n");
			ss_free(sk);
		}
	}
	
	return 0;
}

/**
 *
 *
 */
#ifndef BUFLEN
#define BUFLEN	1024
#endif
static int _do_process(sslsock_t *sk, int i)
{
	char buf[BUFLEN] = {0};
	int n;
	int closed = 0;
	
	if (!sk)
		return -1;

	n = ss_recv(sk, buf, sizeof(buf), &closed);
	if (n > 0) {
		printf("read from client: %s\n", buf);
	}
	else if (n == 0) {
		printf("client closed\n\n");
		ss_free(sk);
		_g_socks[i] = NULL;
		_g_nsock--;
	}
		
	return 0;
}

/**
 *	The SSL server main loop, it accept SSL connections, recv SSl data.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_do_server(void)
{
	fd_set rset;
	int maxfd1;
	int n, i;
	struct timeval tv;
	sslsock_t *ss;

	ss = ss_server(_g_sslctx, _g_ip, _g_port);

	while (1) {
		tv.tv_sec = 5;
		tv.tv_usec = 0;
		
		maxfd1 = 0;
		
		/* set FDSET */
		FD_ZERO(&rset);
		for (i = 0; i < CLIENT_MAX; i++) {
			if (_g_socks[i] && _g_socks[i]->fd > 0) {
				FD_SET(_g_socks[i]->fd, &rset);
				if (_g_socks[i]->fd >= maxfd1)
					maxfd1 = _g_socks[i]->fd + 1;
			}
		}
		
		FD_SET(ss->fd, &rset);
		if (ss->fd >= maxfd1)
			maxfd1 = ss->fd + 1;

		n = select(maxfd1, &rset, NULL, NULL, &tv);
		if (n < 0) {
			if (errno == EINTR)
				continue;
			printf("select error: %s\n", strerror(errno));
			break;
		}

		if (n == 0)
			continue;

		/* have client connect */
		if (FD_ISSET(ss->fd, &rset)) {
			_do_accept(ss);
		}

		/* have client data */
		for (i = 0; i < CLIENT_MAX; i++) {
			if (_g_socks[i] && FD_ISSET(_g_socks[i]->fd, &rset)) {
				_do_process(_g_socks[i], i);
			}
		}

	}
	
	ss_free(ss);

	return 0;
}

/**
 *	The main function of SSL server.
 *
 *	Return 0 if success, -1 on error.
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

	_do_server();

	_release();

	return 0;
}



