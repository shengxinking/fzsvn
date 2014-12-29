/*
 *
 *
 */

#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>

#define CLIENT_MAX	1024 * 10
#define IPLEN		32

static unsigned short g_svr_port = 8080;
static char g_svr_ip[IPLEN] = { 0 };
static int g_conns[CLIENT_MAX];
static int g_conn_num = 0;

static void _usage(void)
{
	printf("tcpsvr_select [options]\n\n");
	printf("-a\tserver address\n");
	printf("-p\tserver port\n");
	printf("-h\tshow help message\n");
}

static int _parse_cmd(int argc, char **argv)
{
	char c;
	int port;
	char optstr[] = ":a:p:h";

	while ( (c = getopt(argc, argv, optstr)) != -1) {
		switch (c) {
	
		case 'a':
			strncpy(g_svr_ip, optarg, IPLEN - 1);
			break;
		
		case 'p':
			port = atoi(optarg);
			if (port <= 0 || port > 65535) {
				printf("-p <1 - 65535>");
				return -1;
			}
			g_svr_port = port;
			break;

		case 'h':
			return -1;

		case ':':
			printf("%c need argument\n", optopt);
			return -1;

		case '?':
			printf("unkowned option %c\n", optopt);
			return -1;

		}
	}

	if (optind < argc)
		return -1;

	return 0;
}

static int _init(void)
{
	return 0;
}

static void _cleanup(void)
{
}

static int _create_svr_socket(void)
{
	struct sockaddr_in svraddr;
	int fd;	
	int sockopt;
	long fdflags = 0;

	memset(&svraddr, 0, sizeof(svraddr));
	svraddr.sin_family = AF_INET;
	svraddr.sin_port = htons(g_svr_port);
	svraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		printf("socket error: %s\n", strerror(errno));
		return -1;
	}

	/* reuse address */
	sockopt = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt))) {
		printf("setsockopt SO_REUSEADDR error: %s\n", strerror(errno));
		return -1;
	}

	/* listen socket non-block */
	fdflags = fcntl(fd, F_GETFL, fdflags);
	fdflags |= O_NONBLOCK;
	fcntl(fd, F_SETFL, fdflags);

	if (bind(fd, (struct sockaddr*)&svraddr, sizeof(svraddr))) {
		printf("bind error: %s\n", strerror(errno));
		return -1;
	}

	if (listen(fd, 1024)) {
		printf("listen error: %s\n", strerror(errno));
		return -1;
	}

	return fd;
}

static int _do_accept(int fd)
{
	int clifd;
	int i;
	struct sockaddr_in cliaddr;
	socklen_t clilen = sizeof(cliaddr);

	while (1) {
		clifd = accept(fd, (struct sockaddr*)&cliaddr, &clilen);
		if (clifd < 0) {
			if (errno == EINTR)
				continue;
			if (errno == EAGAIN)
				break;
			printf("accept error: %s\n", strerror(errno));
			return -1;
		}

		if (g_conn_num >= CLIENT_MAX) {
			printf("too many client connect to SSL server\n");
			close(clifd);
			return -1;
		}

		printf("a client connect to me\n");

		for (i = 0; i < CLIENT_MAX; i++) {
			if (g_conns[i] < 0) {
				g_conns[i] = clifd;
				
				g_conn_num ++;
				break;
			}
		}
	}
	
	return 0;
}

#ifndef BUFLEN
#define BUFLEN	1024
#endif
static int _do_read(int *fd)
{
	char buf[BUFLEN] = {0};
	int n;
	
	if (*fd < 0)
		return -1;

	n = read(*fd, buf, BUFLEN - 1);
	if (n > 0)
		printf("read %d bytes from client\n", n);
	else {
		printf("client closed\n\n");
		close(*fd);
		*fd = -1;
	}
		
	return 0;
}

static int _do_loop(int fd)
{
	fd_set rset;
	int maxfd1;
	int n, i;
	struct timeval tv;

	if (fd < 0)
		return -1;

	while (1) {

		tv.tv_sec = 5;
		tv.tv_usec = 0;
		
		maxfd1 = 0;
		
		/* set FDSET */
		FD_ZERO(&rset);
		for (i = 0; i < CLIENT_MAX; i++) {
			if (g_conns[i] > 0) {
				FD_SET(g_conns[i], &rset);
				if (g_conns[i] >= maxfd1)
					maxfd1 = g_conns[i] + 1;
			}
		}
		
		FD_SET(fd, &rset);
		if (fd >= maxfd1)
			maxfd1 = fd + 1;

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
		if (FD_ISSET(fd, &rset)) {
			_do_accept(fd);
		}

		/* have client data */
		for (i = 0; i < CLIENT_MAX; i++) {

			if ( g_conns[i] >= 0 && FD_ISSET(g_conns[i], &rset)) {
				_do_read(&g_conns[i]);
			}
		}

	}
	
	return 0;
}

int main(int argc, char **argv)
{
	int fd;

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	_init();

	fd = _create_svr_socket();
	if (fd < 0)
		return -1;
	
	_do_loop(fd);

	_cleanup();

	return 0;
}

