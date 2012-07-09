/*
 * file         https_client.c
 * 
 * brief        this is a simple https client to send all HTTPS request
 *              and receive HTTPS reponse using openssl client
 *
 * author       Forrest.zhang
 *
 */

#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/bio.h>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>

#define IPLEN		24

static SSL_CTX *g_ssl_ctx;
static unsigned long g_times = 1;
static unsigned short g_svr_port = 1433;
static char g_svr_ip[IPLEN] = {0}; 

static void _usage(void)
{
	printf("ssl_client [options]\n");
	printf("\t-a\tserver address\n");
	printf("\t-p\tserver port\n");
	printf("\t-t\tconnection times\n");
	printf("\t-f\tPOST file name\n");
	printf("\t-h\tshow help info\n");
}

static int _parse_cmd(int argc, char **argv)
{
	char c;
	const char optstr[] = ":a:p:t:h";
	int port;
	int times;

	opterr = 0;
	while ( (c = getopt(argc, argv, optstr)) != -1) {
		
		switch (c) {

		case 'a':
			strncpy(g_svr_ip, optarg, IPLEN - 1);
			break;

		case 'p':
			port = atoi(optarg);
			if (port <= 0) {
				printf("-p <1-65535>\n");
				return -1;
			}
			g_svr_port = port;
			break;

		case 't':
			times = atoi(optarg);
			if (times < 1) {
				printf("-t <1 - NNNN>\n");
				return -1;
			}
			g_times = (unsigned long)times;
			break;

		case 'h':
			_usage();
			exit(0);
		
		case ':':
			printf("%c miss argument\n", optopt);
			return -1;
		
		case '?':
			printf("unknow option %c\n", optopt);
			return -1;
		}

	}

	if (optind != argc)
		return -1;

	if (g_svr_ip[0] == '\0')
		return -1;

	return 0;
}

static int _init(void)
{
	SSL_load_error_strings();
	SSL_library_init();

	g_ssl_ctx = SSL_CTX_new(SSLv23_client_method());
	if (!g_ssl_ctx)
		return -1;

	return 0;
}

static int _do_connect(int fd)
{
	struct sockaddr_in svraddr;
	
	memset(&svraddr, 0, sizeof(svraddr));
	svraddr.sin_family = AF_INET;
	svraddr.sin_port = htons(g_svr_port);
	if (inet_pton(AF_INET, g_svr_ip, &svraddr.sin_addr) <= 0) {
		printf("invalid ip address: %s\n", g_svr_ip);
		return -1;
	}

	if (connect(fd, (struct sockaddr *)&svraddr, sizeof(svraddr))) {
		printf("connect error: %s\n", strerror(errno));
		return -1;
	}
	
	return 0;
}


static int _do_https_request(void)
{

}

static int _do_https_response(void)
{

}

#ifndef BUFLEN
#define BUFLEN	1024
#endif
int main(int argc, char **argv)
{
	int fd;
	SSL *ssl;
	int ret;
	char buf[BUFLEN] = "hello, world";
	int m, n;

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}	

	_init();

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		printf("socket error: %s\n", strerror(errno));
		return -1;
	}

	if (_do_connect(fd)) {
		return -1;
	}

	ssl = SSL_new(g_ssl_ctx);
	if (!ssl) {
		printf("can't get SSL from SSL_ctx\n");
		return -1;
	}

	SSL_set_fd(ssl, fd);

	ret = SSL_connect(ssl);
	if (ret != 1) {
		printf("SSL_connect error\n");
		goto out;
	}

	n = SSL_write(ssl, buf, strlen(buf));
	printf("send %d bytes: %s\n", n, buf);
	memset(buf, 0, sizeof(buf));
	m = SSL_read(ssl, buf, n);
	printf("recv %d bytes: %s\n", m , buf);
	SSL_shutdown(ssl);

out:
	close(fd);
	SSL_free(ssl);
	SSL_CTX_free(g_ssl_ctx);
	
	return 0;
}





