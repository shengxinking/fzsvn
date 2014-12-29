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
#include <stdio.h>
#include <getopt.h>
#include <sys/poll.h>

#include "ip_addr.h"
#include "sock_util.h"

#ifndef	BUFLEN
#define	BUFLEN		1024
#endif

#define	MAX_TCPFD	128

#define	FLOW(level, fmt, args...)			\
	({						\
	if (level < _g_dbglvl)				\
		printf("<%d>: "fmt, level, ##args);	\
	})


struct tcp_fd;
typedef	int		(*event_cb)(struct tcp_fd *tfd);

typedef struct tcp_fd {
	int		fd;
	int		events;
	event_cb	cb;
	char		inbuf[BUFLEN];
	int		inlen;
	char		outbuf[BUFLEN];
	int		outlen;
} tcp_fd_t;


static ip_port_t	_g_svraddr;
static tcp_fd_t		_g_tcpfds[MAX_TCPFD];
static int		_g_nonblock;
static int		_g_transparent;
static int 		_g_dbglvl;

static int		_do_tcp_accept(tcp_fd_t *tfd);
static int		_do_tcp_recv(tcp_fd_t *tfd);
static int		_do_tcp_send(tcp_fd_t *tfd);


/**
 *	Show usage of program.
 *
 *	No return value.
 */
static void 
_usage(void)
{
	printf("ssl_server <options>\n\n");
	printf("\t-a <IP:port>\tserver address\n");
	printf("\t-n\t\tusing nonblock mode\n");
	printf("\t-t\t\tusing transparent mode\n");
	printf("\t-d <0-4>\t\tdebug level <0-4>\n");
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
	char optstr[] = ":a:ntd:h";
	char c;

	opterr = 0;
	while ( (c = getopt(argc, argv, optstr)) != -1) {
	
		switch (c) {
			
		case 'a':
			if (ip_port_from_str(&_g_svraddr, optarg)) {
				printf("invalid server address %s\n", optarg);
				return -1;
			}
			break;

		case 'n':
			_g_nonblock = 1;
			break;

		case 't':
			_g_transparent = 1;
			break;

		case 'd':
			_g_dbglvl = atoi(optarg);
			if (_g_dbglvl < 0 || _g_dbglvl > 4) {
				printf("invalid debug level %s\n", optarg);
				return -1;
			}
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
	
	if (_g_svraddr.family == 0)
		return -1;

	return 0;
}

static tcp_fd_t * 
_tcp_fd_alloc(void)
{
	int i;
	tcp_fd_t *tfd;

	for (i = 0; i < MAX_TCPFD; i++) {
		tfd = &_g_tcpfds[i];
		if (tfd->fd < 1)
			return tfd;
	}

	return NULL;
}

static tcp_fd_t * 
_tcp_fd_find(int fd)
{
	int i;
	tcp_fd_t *tfd;

	for (i = 0; i < MAX_TCPFD; i++) {
		tfd = &_g_tcpfds[i];
		if (tfd->fd == fd)
			return tfd;
	}

	return NULL;
}

static int 
_tcp_fd_free(tcp_fd_t *tfd)
{
	if (tfd->fd > 0) {
		close(tfd->fd);
		FLOW(1, "client %d closed\n", tfd->fd);
		tfd->fd = -1;
	}

	tfd->cb = NULL;
	tfd->events = 0;
	memset(tfd->inbuf, 0, sizeof(tfd->inbuf));
	tfd->inlen = 0;
	memset(tfd->outbuf, 0, sizeof(tfd->outbuf));
	tfd->outlen = 0;

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
	int fd;
	tcp_fd_t *tfd;

	fd = sk_tcp_server(&_g_svraddr, 0, _g_transparent);
	if (fd < 0)
		return -1;

	tfd = _tcp_fd_alloc();
	if (!tfd)
		return -1;

	tfd->fd = fd;
	tfd->events = POLLIN;
	tfd->cb = _do_tcp_accept;
	
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
	int i;
	tcp_fd_t *tfd;

	for (i = 0; i < MAX_TCPFD; i++) {
		tfd = &_g_tcpfds[i];
		_tcp_fd_free(tfd);
	}
}

/**
 *	Accept client ssl connection, socket fd into @_g_conns.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_do_tcp_accept(tcp_fd_t *tfd)
{
	int fd;
	tcp_fd_t *clitfd;
	ip_port_t cliaddr;
	ip_port_t svraddr;
	char ipstr1[IP_STR_LEN];
	char ipstr2[IP_STR_LEN];

	fd = sk_tcp_accept(tfd->fd, &cliaddr, &svraddr);
	if (fd < 0) {
		printf("_accept error: %s\n", strerror(errno));
		return -1;
	}

	FLOW(1, "client %d accepted(%s->%s)\n", fd, 
	     ip_port_to_str(&cliaddr, ipstr1, IP_STR_LEN),
	     ip_port_to_str(&svraddr, ipstr2, IP_STR_LEN));

	if (_g_nonblock)
		sk_set_nonblock(fd, 1);

	clitfd = _tcp_fd_alloc();
	if (!clitfd) {
		FLOW(1, "client %d can't get free slot\n", fd);
		close(fd);
		return -1;
	}

	clitfd->fd = fd;
	clitfd->events = POLLIN;
	clitfd->cb = _do_tcp_recv;
	
	return 0;
}

static int 
_do_tcp_recv(tcp_fd_t *tfd)
{
	int n;
	int len;
	char *ptr;
	int closed;

	ptr = tfd->inbuf + tfd->inlen;
	len = sizeof(tfd->inbuf) - tfd->inlen - 1;
	n = sk_recv(tfd->fd, ptr, len, &closed);
	if (n < 0) {
		FLOW(1, "client %d recv data failed\n", tfd->fd);
		_tcp_fd_free(tfd);
		return -1;
	}

	if (n == 0) {
		if (closed) {
			FLOW(1, "client %d recv closed\n", tfd->fd);
			_tcp_fd_free(tfd);
		}
		return 0;
	}
	
	FLOW(1, "client %d recv %d bytes: %s\n", tfd->fd, n, ptr);

	tfd->inlen += n;

	/* recv finished, need send recved data to peer */	
	memcpy(tfd->outbuf, tfd->inbuf, tfd->inlen);
	tfd->outlen = tfd->inlen;
	tfd->inlen = 0;

	tfd->events = POLLOUT;
	tfd->cb = _do_tcp_send;

	return 0;
}

static int 
_do_tcp_send(tcp_fd_t *tfd)
{
	int n;
	int len;
	char *ptr;
	
	ptr = tfd->outbuf;
	len = tfd->outlen;
	n = sk_send(tfd->fd, ptr, len);
	if (n < 0) {
		FLOW(1, "client %d send %d bytes failed\n", tfd->fd, len);
		_tcp_fd_free(tfd);
		return -1;
	}
	else if (n != len) {
		FLOW(1, "client %d send %d bytes blocked(%d)\n", 
		     tfd->fd, len, n);
		ptr = tfd->outbuf + n;
		memmove(tfd->outbuf, ptr, len - n);
		tfd->outlen = len - n;
		return 0;
	}

	FLOW(1, "client %d send %d bytes: %s\n", tfd->fd, len, ptr);
	tfd->events = POLLIN;
	tfd->cb = _do_tcp_recv;

	return 0;
}

static int 
_fill_poll_fd(struct pollfd *pfds, int npfd)
{
	int i;
	int n;
	tcp_fd_t *tfd;
	
	n = 0;
	for (i = 0; i < MAX_TCPFD; i++) {
		tfd = &_g_tcpfds[i];
		if (tfd->fd < 1)
			continue;
		pfds[n].fd = tfd->fd;
		pfds[n].events = tfd->events;
		pfds[n].revents = 0;		
		n++;
	}

	return n;
}

/**
 *	Main loop function, it accept SSL connections, recv SSl data.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_do_loop(void)
{
	int i;
	int n;
	int npfd;
	tcp_fd_t *tfd;
	struct pollfd pfds[MAX_TCPFD];

	while (1) {
		
		npfd = _fill_poll_fd(pfds, MAX_TCPFD);
		
		n = poll(pfds, npfd, 1);
		if (n < 0) {
			printf("poll failed: %s\n", strerror(errno));
			break;
		}
		if (n == 0) 
			continue;

		//FLOW(1, "poll get %d events\n", n);
		
		for (i = 0; i < npfd; i++) {
			tfd = _tcp_fd_find(pfds[i].fd);
			if (!tfd) {
				printf("not found fd %d\n", pfds[i].fd);
				continue;
			}

			if (pfds[i].revents & POLLERR) {
				FLOW(1, "client %d event  error\n", pfds[i].fd);
				_tcp_fd_free(tfd);
				continue;
			}

			if (pfds[i].revents & tfd->events) {
				tfd->cb(tfd);
			}
		}
	}
	
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

	_do_loop();

	_release();

	return 0;
}



