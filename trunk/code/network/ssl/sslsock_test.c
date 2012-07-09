/**
 *	@file	sslsock_test.c
 *
 *	@brief	Test SSL socket API in sslsock.h
 *	
 *	@date	2009-08-10.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "sslsock.h"


#ifndef BUFLEN
#define	BUFLEN		63
#endif

static char		_g_certfile[BUFLEN + 1];/* certificate file */
static char		_g_keyfile[BUFLEN + 1];	/* private key file */
static char		_g_password[BUFLEN + 1];/* the password keyfile */
static u_int32_t	_g_sip;			/* server IP */
static u_int16_t	_g_sport;		/* server port */
static int		_g_mode;		/* the run mode */
static int		_g_stop = 0;		/* stop running */
static sslsock_ctx_t	*_g_sslctx;		/* the sslsock ctx */


/**
 *	Show help message	
 *	
 *	No return.
 */
static void 
_usage(void)
{
	printf("sslsock_test <options>\n");
	printf("\t-m\trun mode: 0 client, 1 server\n");
	printf("\t-c\tcerificate file\n");
	printf("\t-k\tprivate key file\n");
	printf("\t-w\tpassword for private key file\n");
	printf("\t-i\tthe service IP address\n");
	printf("\t-p\tthe server port\n");
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
	char optstr[] = ":m:c:k:w:i:p:h";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {

		case 'm':
			_g_mode = atoi(optarg);
			if (_g_mode < 0 || _g_mode > 1) {
				printf("non-support mode: %s\n", optarg);
				return -1;
			}
			
			_g_mode = (_g_mode == 0) ? SS_CLIENT : SS_SERVER;

			break;
	
		case 'c':
			strncpy(_g_certfile, optarg, BUFLEN);

			break;

		case 'k':
			strncpy(_g_keyfile, optarg, BUFLEN);

			break;

		case 'w':
			strncpy(_g_password, optarg, BUFLEN);

			break;		

		case 'i':
			_g_sip = inet_addr(optarg);

			break;

		case 'p':
			_g_sport = htons(atoi(optarg));
			if (_g_sport < 1) {
				printf("unsupport port: %s\n", optarg);
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

	if (argc != optind)
		return -1;

	if (_g_sport == 0)
		return -1;

	if (_g_sip == 0 && _g_mode == 0)
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
	_g_sslctx = ss_ctx_alloc(_g_mode, SS_VERIFY_NONE);
	if (!_g_sslctx) {
		printf("Create sslsock_ctx_t object failed\n");
		return -1;
	}

	if (ss_ctx_load_cert(_g_sslctx, _g_certfile, 
			     _g_keyfile, _g_password))
	{
		return -1;
	}


	if (ss_ctx_set_cipher(_g_sslctx, "HIGH:MEDIA:LOW:!DH")) {
		printf("set cipher failed\n");
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
	if (_g_sslctx) {
		ss_ctx_free(_g_sslctx);
	}
	_g_sslctx = NULL;
}

static int 
_process_server(void)
{
	sslsock_t *ss, *cliss;
	char buf[1024];
	int len;
	int closed = 0;
	
	ss = ss_server(_g_sslctx, _g_sip, _g_sport);
	if (!ss) {
		printf("Create server sslsock_t object failed\n");
		return -1;
	}

	while (!_g_stop) {

		cliss = ss_accept(ss, 0);
		if (!cliss) {
			continue;
		}

		ss_set_nbio(ss, 1);

		sleep(10000);

		if (ss_handshake(cliss) != 1) {
			printf("handshake failed\n");
		}

		memset(buf, 1024, 0);
		len = ss_recv(cliss, buf, 1023,  &closed);
		if (len < 0) {
			printf("Recv client data failed\n");
			ss_free(cliss);
			break;
		}

		printf("Client: %s\n", buf);

		memset(buf, 0, 1024);
		strcpy(buf, "Hello world from server!\n");
		len = strlen(buf) + 1;
		if (ss_send(cliss, buf, len) < 0) {
			ss_free(cliss);
			printf("Send client data failed\n");
			break;
		}

		printf("server: %s\n", buf);

		ss_free(cliss);
	}

	ss_free(ss);

	return 0;
}


static int 
_process_client(void)
{
	sslsock_t *ss;
	char buf[1024] = "hello world from client!\n";
	int len;
	int closed = 0;

	ss = ss_client(_g_sslctx, _g_sip, _g_sport);
	if (!ss) {
		printf("sslsock_client failed\n");
		return -1;
	}

	if (ss_handshake(ss) != 1) {
		printf("handshake failed\n");
	}

	len = strlen(buf) + 1;
	if (ss_send(ss, buf, len) < 0) {
		printf("sslsock_send failed\n");
	}
	
	printf("Client: %s\n", buf);

	memset(buf, 0, 1024);
	len = ss_recv(ss, buf, 1023, &closed);
	if (len < 0) {
		printf("sslsock_recv failed\n");
		return -1;
	}

	printf("Server: %s\n", buf);

	ss_free(ss);

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

	if (_g_mode == SS_CLIENT)
		_process_client();
	else 
		_process_server();


	_release();

	return 0;
}



