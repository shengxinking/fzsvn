/**
 *	@file	sock_util.c
 *	@brief	socket function implement.
 *
 *	@author	Forrest.Zhang
 */


#include <sys/socket.h>
#include <netdb.h>
#include <sys/types.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <poll.h>
#include <stdio.h>
#include <netinet/tcp.h>
#include <assert.h>
#include <netdb.h>

#include "netutil.h"

/* likely()/unlikely() for performance */
#ifndef unlikely
#define unlikely(x)  __builtin_expect(!!(x), 0)
#endif

#ifndef	likely
#define likely(x)    __builtin_expect(!!(x), 1)
#endif

/**
 *	Define debug MACRO to print debug information
 */
#define _SOCK_DBG	1
#ifdef _SK_DBG
#define _SK_ERR(fmt, args...)	fprintf(stderr, "%s:%d: "fmt,\
					__FILE__, __LINE__, ##args)
#else
#define _SK_ERR(fmt, args...)
#endif


static int 
_sk_addr_from_ip_port(sk_addr_t *sk, ip_port_t *ip)
{
	assert(sk);
	assert(ip);

	if (unlikely(ip->family!=AF_INET&&ip->family!=AF_INET6)){
		_SK_ERR("error ip->type %d\n", ip->type);
		return -1;
	}
	
	memset(sk, 0, sizeof(*sk));
	sk->_addrsa.sa_family = ip->family;
	if (ip->family == AF_INET) {
		sk->_addrv4.sin_addr = ip->_addr4;
		sk->_addrv4.sin_port = ip->port;
	}
	else {
		sk->_addrv6.sin6_addr = ip->_addr6;
		sk->_addrv6.sin6_port = ip->port;
	}

	return 0;
}

int 
sk_tcp_server(ip_port_t *ip)
{
	sk_addr_t svraddr;
	int fd;	
	int opt;
	socklen_t len;

	if (unlikely(!ip))
		return -1;

	if (_sk_addr_from_ip_port(&svraddr, ip))
		return -1;
	
	fd = socket(ip->family, SOCK_STREAM, 0);
	if (fd < 0) {
		_SK_ERR("socket error: %s\n", strerror(errno));
		return -1;
	}

	/* reuse address */
	opt = 1;
	len = sizeof(opt);
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, len)) {
		_SK_ERR("setsockopt error: %s\n", strerror(errno));
		return -1;
	}

	if (bind(fd, (struct sockaddr*)&svraddr, sizeof(svraddr))){
		_SK_ERR("bind error: %s\n", strerror(errno));
		return -1;
	}

	if (listen(fd, 80000)) {
		_SK_ERR("listen error: %s\n", strerror(errno));
		return -1;
	}

	return fd;
}

int 
sk_connect(int fd, ip_port_t *ip)
{
	sk_addr_t svraddr;
	int ret;
	socklen_t len;

	if (unlikely(fd < 0 || !ip))
		return -1;

	if (_sk_addr_from_ip_port(&svraddr, ip))
		return -1;
	
	len = sizeof(svraddr);
	ret = connect(fd, (struct sockaddr *)&svraddr, len);
	if (ret < 0) {
		if (errno == EINPROGRESS)
			return 0;
		
		_SK_ERR("connect error: %s\n", 	strerror(errno));
		return -1;
	}

	return 1;
}

int 
sk_tcp_client_nb(ip_port_t *ip, int *wait)
{
	int fd;
	int ret;

	if (unlikely(!ip || !wait))
		return -1;

	fd = socket(ip->family, SOCK_STREAM, 0);
	if (fd < 0) {
		_SK_ERR("socket error: %s\n", strerror(errno));
		return -1;
	}

	sk_set_nonblock(fd, 1);

	ret = sk_connect(fd, ip);
	if (ret < 0) {
		close(fd);
		return -1;
	}

	if (ret == 1)
		*wait = 0;
	else
		*wait = 1;

	return fd;
}

int 
sk_tcp_client(ip_port_t *ip)
{
	int fd;	
	int ret;

	if (unlikely(!ip))
		return -1;

	fd = socket(ip->family, SOCK_STREAM, 0);
	if (fd < 0) {
		_SK_ERR("socket error: %s\n", strerror(errno));
		return -1;
	}

	ret = sk_connect(fd, ip);
	if (ret != 1) {
		close(fd);
		return -1;
	}

	return fd;
}

int 
sk_accept(int fd, ip_port_t *ip)
{
	int clifd;
	sk_addr_t addr;
	socklen_t len;

	if (unlikely(fd < 0))
		return -1;
	
	len = sizeof(addr);
	clifd = accept(fd, (struct sockaddr *)&addr, &len);
	if (clifd < 0)
		return -1;

	if (ip) {
		ip->family = addr._addrsa.sa_family;
		if (addr._addrsa.sa_family == AF_INET) {
			ip->_addr4 = addr._addrv4.sin_addr;
			ip->port = addr._addrv4.sin_port;
		}
		else {
			ip->_addr6 = addr._addrv6.sin6_addr;
			ip->port = addr._addrv6.sin6_port;
		}
	}

	return clifd;
}

int 
sk_is_connected(int fd)
{
	int err;
	socklen_t len;

        len = sizeof(err);
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len)) {
                _SK_ERR("getsockopt(SO_ERROR) error: %s\n",
                          strerror(errno));
                return -1;
        }
        if (err) {
                return -1;
        }

        return 1;
}

int 
sk_set_nonblock(int fd, int nbio)
{
	int oldflags;

	if (fd < 0)
		return -1;

        /* get old flags */
	oldflags = fcntl(fd, F_GETFL, 0);
	if (oldflags < 0) {
		return -1;
	}

	if (nbio)
		oldflags |=  O_NONBLOCK;
	else
		oldflags &= ~O_NONBLOCK;
	if (fcntl(fd, F_SETFL, oldflags) < 0) {
		_SK_ERR("fcntl error: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}

int 
sk_set_keepalive(int fd)
{
	int opt = 1;
	socklen_t len;

	if (fd < 0) {
		_SK_ERR("invalid parameter\n")
		return -1;
	}

	/* enable keepalive */
	len = sizeof(opt);
        if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &opt, len)) {
		_SK_ERR("setsockopt error: %s\n", strerror(errno));
		return -1;
	}
	
	/* set keepalive try times */
	opt = 3;
	if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &opt, len)) {
		_SK_ERR("setsockopt error: %s\n", strerror(errno));
		return -1;
	}

	/* set keepalive packet idle time */
	opt = 60;
	if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &opt, len)) {
		_SK_ERR("setsockopt error: %s\n", strerror(errno));
		return -1;
	}

	/* set keepalive pakcet interval */
	opt = 20;
	if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &opt, len)){
		_SK_ERR("setsockopt error: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}

int 
sk_gethostbyname(const char *domain, int family, ip_addr_t *ip)
{
	struct addrinfo ai;
	struct addrinfo *res = NULL, *pai = NULL;
	struct sockaddr_in *in4;
	struct sockaddr_in6 *in6;
	int ret;

	if (unlikely(!domain || !ip))
		return -1;

	/* set filter */
	memset(&ai, 0, sizeof(ai));
	ai.ai_flags = (AI_V4MAPPED | AI_ADDRCONFIG);
	ai.ai_family = family;
	ret = getaddrinfo(domain, NULL, &ai, &res);
	if (ret) {
		return -1;
	}

	pai = res;
	while (pai) {
		if (pai->ai_family == AF_INET) {
			in4 = (struct sockaddr_in *)pai->ai_addr;
			ip->family = AF_INET;
			memcpy(&ip->_addr4, &in4->sin_addr, 4);
			break;
		}
		else if (pai->ai_family == AF_INET6) {
			in6 = (struct sockaddr_in6 *)pai->ai_addr;
			ip->family = AF_INET6;			
			memcpy(&ip->_addr6, &in6->sin6_addr, 16);
			break;
		}

		pai = pai->ai_next;
	}

	freeaddrinfo(res);

	if (pai)
		return 0;
	else
		return -1;
}

int 
sk_recv(int fd, void *buf, size_t len, int *closed)
{
	int m;
	
	if (unlikely(fd < 0 || !buf || len < 1 || !closed)) {
		_SK_ERR("invalid parameter\n");
		return -1;
	}

	*closed = 0;

	m = recv(fd, buf, len, MSG_DONTWAIT);
	if (m < 0) {
		/* interrupt or need try again */
		if (errno == EINTR || errno == EAGAIN)
			return 0;
		
		_SK_ERR("recv error: %s\n", strerror(errno));
		return -1;
	}

	/* recv closed */
	else if (m == 0)
		*closed = 1;

	return m;
}

int 
sk_send_all(int fd, const void *buf, size_t len)
{
	struct pollfd pfd;
	int n = 0, m = 0, ret;
	int remain = len;

	if (unlikely(fd < 0 || !buf || len < 1)) {
		_SK_ERR("invalid param\n");
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
			if (errno != EINTR || errno != EAGAIN) {
				_SK_ERR("send error: %s\n", 
					strerror(errno));
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
sk_send(int fd, const void *buf, size_t len)
{
	int n = 0;

	if (unlikely(fd < 0 || !buf || len < 1)) {
		_SK_ERR("invalid param\n");
		return -1;
	}

	n = send(fd, buf, len, MSG_DONTWAIT);
	if (n < 0) {
		if (errno != EINTR || errno != EAGAIN) {
			_SK_ERR("send error: %s\n", 
				strerror(errno));
			return -1;
		}
		n = 0;
	}

	return n;
}




