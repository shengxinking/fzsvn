/**
 *	@file	tcpcli_tw.c
 *
 *	@brief	TCP client timewait test program.
 *
 *	@author	Forrest.zhang
 *	
 *	@date	2010-01-20
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
#include <sys/poll.h>

#include "ip_addr.h"
#include "sock_util.h"

#ifndef	BUFLEN
#define	BUFLEN		1024
#endif

#define FLOW(level, fmt, args...)			\
	({						\
		if (level < _g_dbglvl)			\
		printf("<%d>: "fmt, level, ##args);	\
	 })

static ip_port_t	_g_svraddr;	/* server address */
static char		_g_msg[BUFLEN];	/* message sendto server */
static int		_g_msglen;	/* message length */
static int		_g_nonblock;	/* nonblock mode */
static int		_g_dbglvl;	/* debug level 0-4 */

/**
 *	Show help message	
 *	
 *	No return.
 */
static void 
_usage(void)
{
	printf("tcpcli_tw <options>\n");
	printf("\t-a <IP:port>\tthe server address\n");
	printf("\t-m <string>\tthe message send to server\n");
	printf("\t-n\t\tusing nonblock mode\n");
	printf("\t-d <0-4>\tdebug level\n");
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
	char optstr[] = ":a:m:nd:h";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {
			
		case 'a':
			if (ip_port_from_str(&_g_svraddr, optarg)) {
				printf("invalid server address\n");
				return -1;
			}
			break;

		case 'm':
			strcpy(_g_msg, optarg);
			_g_msglen = strlen(_g_msg);
			break;

		case 'n':
			_g_nonblock = 1;
			break;

		case 'd':
			_g_dbglvl = atoi(optarg);
			if (_g_dbglvl < 0 || _g_dbglvl > 4) {
				printf("invalid debug level %s\n", optarg);
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
_do_tcp_connect(void)
{
	int fd;
	int ret;
	int wait;
	struct pollfd pfd;
	char ipstr[IP_STR_LEN];

	/* connect to server */
	fd = sk_tcp_client_nb(&_g_svraddr, NULL, 0, &wait);
	if (fd < 0) {
		FLOW(1, "client connect to server(%s) failed\n", 
		     ip_port_to_str(&_g_svraddr, ipstr, IP_STR_LEN));
		return -1;
	}

	while (wait) {
		pfd.fd = fd;
		pfd.events = POLLOUT;
		pfd.revents = 0;

		ret = poll(&pfd, 1, 1);
		if (ret < 0)
			return -1;

		if (ret == 0)
			continue;

		if (pfd.revents & POLLERR) {
			FLOW(1, "client %d connect event error\n", fd);
			close(fd);
			return -1;
		}

		if (sk_is_connected(fd)) {
			FLOW(1, "client %d connect failed\n", fd);
			close(fd);
			return -1;
		}

		wait = 0;
	}

	FLOW(1, "client %d connect success\n", fd);

	return fd;
}

static int 
_do_tcp_send(int fd)
{
	int n;
	int len;
	char *ptr;
	struct pollfd pfd;

	if (_g_msglen < 1)
		return 0;

	ptr = _g_msg;
	len = _g_msglen;

	while (len > 0) {
		pfd.fd = fd;
		pfd.events = POLLOUT;
		pfd.revents = 0;

		n = poll(&pfd, 1, 1);
		if (n < 0)
			return -1;

		if (n == 0)
			continue;

		if (pfd.revents & POLLERR) {
			FLOW(1, "client %d send event error\n", fd);
			return -1;
		}

		n = sk_send(fd, ptr, len);
		if (n < 0) {
			FLOW(1, "client %d send %d bytes failed\n", fd, len);
			return -1;
		}

		if (n == len) {
			FLOW(1, "client %d send %d bytes: %s\n", 
			     fd, len, ptr);
			return 0;
		}

		FLOW(1, "client %d send %d bytes blocked %d\n", fd, len, n);

		ptr += n;
		len -= n;
	}

	return 0;
}

static int 
_do_tcp_recv(int fd)
{
	int n;
	int len = 0;
	int buflen;
	char buf[BUFLEN];
	struct pollfd pfd;
	int closed;

	if (_g_msglen < 1)
		return 0;

	buflen = sizeof(buf);

	while (len < _g_msglen) {
		pfd.fd = fd;
		pfd.events = POLLIN;
		pfd.revents = 0;

		n = poll(&pfd, 1, 1);
		if (n < 0)
			return -1;

		if (n == 0)
			continue;

		if (pfd.events & POLLERR) {
			FLOW(1, "client %d recv failed\n", fd);
			return -1;
		}

		memset(buf, 0, buflen);
		n = sk_recv(fd, buf, buflen, &closed);
		if (n < 0) {
			FLOW(1, "client %d recv failed\n", fd);
			return -1;
		}

		FLOW(1, "client %d recv %d bytes: %s\n", fd, n, buf);
		len += n;

		if (closed) {
			FLOW(1, "client %d recv closed\n", fd);
			return 0;
		}
	}

	return 0;
}

static int 
_do_client_nb(void)
{
	int fd;

	/* connect to server */
	fd = _do_tcp_connect();
	if (fd < 0)
		return -1;

	/* send data */
	if (_do_tcp_send(fd)) {
		close(fd);
		return -1;
	}

	/* recv data */
	if (_do_tcp_recv(fd)) {
		close(fd);
		return -1;
	}

	/* close connect */
	FLOW(1, "client %d closed\n", fd);
	close(fd);

	return 0;
}

static int 
_do_client(void)
{
	int n;
	int fd;
	int wait;
	int buflen;
	char buf[BUFLEN];
	char ipstr[IP_STR_LEN];

	fd = sk_tcp_client(&_g_svraddr, NULL, 0);
	if (fd < 0) {
		FLOW(1, "connect to %s failed\n", 
		     ip_port_to_str(&_g_svraddr, ipstr, IP_STR_LEN));
		return -1;
	}

	FLOW(1, "client %d connect %s success\n", fd, 
	     ip_port_to_str(&_g_svraddr, ipstr, IP_STR_LEN));

	if (_g_msglen > 0) {
		n = sk_send(fd, _g_msg, _g_msglen);
		if (n != _g_msglen) {
			FLOW(1, "client %d send %d bytes failed\n", 
			     fd, _g_msglen);
			close(fd);
			return -1;
		}
		FLOW(1, "client %d send %d bytes: %s\n", fd, n, _g_msg);

		buflen = sizeof(buf);
		memset(buf, 0, buflen);
		//_do_tcp_recv(fd);
	}

	close(fd);

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

	if (_g_nonblock)
		_do_client_nb();
	else
		_do_client();

	_release();

	return 0;
}



