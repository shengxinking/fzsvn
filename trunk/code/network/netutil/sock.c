/**
 *	@file	sock.c
 *
 *	@brief	the socket wrap APIs.
 *
 *	@author	Forrest.zhang.
 *
 */
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/poll.h>
#include <stdio.h>
#include <sys/time.h>
#include <assert.h>

#include "sock.h"

/**
 *	convert IP address(uint32_t) to dot-decimal string
 *	like xxx.xxx.xxx.xxx 
 */
#ifndef NIPQUAD
#define NIPQUAD(addr) \
        ((unsigned char *)&addr)[0], \
        ((unsigned char *)&addr)[1], \
        ((unsigned char *)&addr)[2], \
        ((unsigned char *)&addr)[3]
#define NIPQUAD_FMT "%u.%u.%u.%u"
#endif


/**
 *	Define debug MACRO to print debug information
 */
#define _SOCK_DBG	1
#ifdef _SOCK_DBG
#define _SOCK_ERR(fmt, args...)	fprintf(stderr, "sock:%s:%d: "fmt,\
					__FILE__, __LINE__, ##args)
#else
#define _SOCK_ERR(fmt, args...)
#endif

int 
sock_set_nbio(int fd, int nbio)
{
	int oldflags;

	if (fd < 0)
		return -1;

	/* get old flags */
	oldflags = fcntl(fd, F_GETFL, 0);
	if (oldflags < 0) {
		_SOCK_ERR("fd %d fcntl(F_GETFL) failed: %s\n", 
			  fd, strerror(errno));
		return -1;
	}

	if (nbio)
	       	oldflags |=  O_NONBLOCK;
	else
		oldflags &= ~O_NONBLOCK;

	if (fcntl(fd, F_SETFL, oldflags) < 0) {
		_SOCK_ERR("fd %d fcntl(F_SETFL) error: %s\n", 
			  fd, strerror(errno));
		return -1;
	}

	return 0;
}


int 
sock_tcpsvr(u_int32_t ip, u_int16_t port)
{
	int fd;	
	int sopt;
	struct sockaddr_in svraddr;

	/* create socket */
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		_SOCK_ERR("socket error: %s\n", strerror(errno));
		return -1;
	}

	/* reuse address */
	sopt = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &sopt, sizeof(sopt))) {
		_SOCK_ERR("fd %d setsockopt(SO_REUSEADDR) error: %s\n", 
			  fd, strerror(errno));
		close(fd);
		return -1;
	}
	
	/* bind */
	memset(&svraddr, 0, sizeof(svraddr));
	svraddr.sin_family = AF_INET;
	svraddr.sin_port = port;
	svraddr.sin_addr.s_addr = ip;
	if (bind(fd, (struct sockaddr*)&svraddr, sizeof(svraddr))) {
		_SOCK_ERR("fd %d bind error: %s\n", fd, strerror(errno));
		close(fd);
		return -1;
	}

	/* listen */
	if (listen(fd, 1024)) {
		_SOCK_ERR("fd %d listen error: %s\n", fd, strerror(errno));
		return -1;
	}

	return fd;
}


int 
sock_tcpcli(u_int32_t ip, u_int16_t port)
{
	int fd;	
	int ret = 0;

	/* create socket */
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		_SOCK_ERR("socket error: %s\n", 
			  strerror(errno));
		return -1;
	}

	ret = sock_connect(fd, ip, port);
	if (ret) {
		close(fd);
		return -1;
	}

	return fd;
}


int 
sock_tcpcli_nb(u_int32_t ip, u_int16_t port, int *wait)
{
	int fd;	
	int ret = 0;

	if (!wait)
		return -1;

	/* create socket */
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		_SOCK_ERR("socket error: %s\n", 
			  strerror(errno));
		return -1;
	}

	if (sock_set_nbio(fd, 1))
		return -1;

	ret = sock_connect(fd, ip, port);
	if (ret < 0) {
		close(fd);
		return -1;
	}
	if (ret == 0)
		*wait = 1;
	else
		*wait = 0;

	return fd;
}



int 
sock_is_connected(int fd)
{
	struct pollfd pfd;
	int err = 0;
	socklen_t len;
	int ret;

	pfd.fd = fd;
	pfd.events = POLLOUT;
	pfd.revents = 0;

	ret = poll(&pfd, 1, 0);
	if (ret < 0) {
		if (errno != EINTR)
			return -1;
		return 0;
	}
	if (ret == 0)
		return 0;

	if (pfd.revents & POLLERR)
		return -1;

	len = sizeof(err);
	if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len)) {
		_SOCK_ERR("getsockopt(SO_ERROR) error: %s\n", 
			  strerror(errno));
		return -1;
	}
	if (err) {
		return -1;
	}

	return 1;
}


int 
sock_connect(int fd, u_int32_t ip, u_int16_t port)
{
	struct sockaddr_in svraddr;
	socklen_t len;
	int ret = -1;

	if (fd < 0)
		return -1;

	/* connect to server */
	memset(&svraddr, 0, sizeof(svraddr));
	svraddr.sin_family = AF_INET;
	svraddr.sin_port = port;
	svraddr.sin_addr.s_addr = ip;
	len = sizeof(svraddr);
	ret = connect(fd, (struct sockaddr *)&svraddr, len);
	if (ret < 0) {
		/* the connect failed */
		if (errno != EINPROGRESS) {
			_SOCK_ERR("connect to %u.%u.%u.%u:%u failed: %s\n", 
				  NIPQUAD(ip), port, strerror(errno));
			return -1;
		}
		/* connect need check later */
		return 0;
	}
	
	return 1;
}


int 
sock_accept(int fd, u_int32_t *dip, u_int16_t *dport)
{
	struct sockaddr_in cliaddr;
	socklen_t len;
	int clifd = -1;

	if (fd < 0) {
		_SOCK_ERR("invlid argument\n");
		return -1;
	}

	len = sizeof(cliaddr);
	memset(&cliaddr, 0, len);
	clifd = accept(fd, (struct sockaddr *)&cliaddr, &len);
	if (clifd < 0) {
		if (errno != EINTR && errno != EAGAIN) {
			return -1;
		}
		return -1;
	}

	if (dip)
		*dip = cliaddr.sin_addr.s_addr;
	if (dport)
		*dport = cliaddr.sin_port;

	return clifd;
}


int 
sock_send(int fd, const char *buf, size_t len)
{
	struct pollfd pfd;
	int n = 0, m = 0, ret;
	int remain = len;

	if (fd < 0 || !buf || len < 1) {
		_SOCK_ERR("invalid param\n");
		return -1;
	}

	pfd.fd = fd;
	pfd.events = POLLOUT;
	pfd.revents = 0;
	while (remain > 0) {
		ret = poll(&pfd, 1, -1);
		if (ret < 0) {
			if (errno == EINTR)
			       return m;
			return -1;	
		}
		
		if (pfd.revents & POLLERR)
			return -1;

		/* send first time */
		n = send(fd, buf + m, remain, MSG_DONTWAIT);
		if (n < 0) {
			if (errno != EINTR && errno != EAGAIN) {
				_SOCK_ERR("fd %d send error: %s\n", fd, strerror(errno));
				return -1;
			}					
			n = 0;
		}

		remain -= n;
		m += n;
	}

	return m;
}


int 
sock_recv(int fd, char *buf, size_t len, int *closed)
{
	int m;

	if (fd < 0 || !buf || len < 1 || !closed) {
		_SOCK_ERR("invalid parameter\n");
		return -1;
	}

	*closed = 0;

	m = recv(fd, buf, len, MSG_DONTWAIT);
	if (m < 0) {
		if (errno != EINTR && errno != EAGAIN) {
			_SOCK_ERR("fd %d recv error: %s\n", fd, strerror(errno));
			return -1;
		}

		return 0;
	}

	/* recv closed */
	else if (m == 0) {
		*closed = 1;
	}

	return m;
}




