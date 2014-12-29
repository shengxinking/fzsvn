/**
 *	@file	sock_util.c
 *	@brief	socket function implement.
 *
 *	@author	Forrest.Zhang
 */

#define	_GNU_SOURCE

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

#include "sock_util.h"

/* likely()/unlikely() for performance */
#if !defined(likely)
#if __GNUC__ < 3
#define __builtin_expect(x,y) (x)
#define likely(x) (x)
#define unlikely(x) (x)
#elif __GNUC__ < 4
/* gcc 3.x does the best job at this */
#define likely(x) (__builtin_expect((x) != 0, 1))
#define unlikely(x) (__builtin_expect((x) != 0, 0))
#else
/* GCC 4.x is stupid, it performs the comparison then compares it to 1,
 * so we cheat in a dirty way to prevent it from doing this. This will
 * only work with ints and booleans though.
 */
#define likely(x) (x)
#define unlikely(x) (__builtin_expect((unsigned long)(x), 0))
#endif
#endif

/**
 *	Define debug MACRO to print debug information
 */
#define _SK_DBG	1
#ifdef	_SK_DBG
#define _SK_ERR(fmt, args...)	fprintf(stderr, "%s:%d: "fmt,\
					__FILE__, __LINE__, ##args)
#else
#define _SK_ERR(fmt, args...)
#endif

/* for some old system no REUSEPORT macro */
#ifndef	SO_REUSEPORT
#define	SO_REUSEPORT	15
#endif


/**
 *	Convert ip_port_t @ip to sk_addr_t address.
 *
 * 	Return 0 if success, -1 on error.
 */
static int 
_sk_addr_from_ip_port(sk_addr_t *sk, ip_port_t *ip)
{
	assert(sk);
	assert(ip);

	if (unlikely(ip->family!=AF_INET&&ip->family!=AF_INET6)){
		_SK_ERR("error ip->type %d\n", ip->family);
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
sk_tcp_client(ip_port_t *dip, ip_port_t *sip, int transparent)
{
	int fd;	
	int ret;

	if (unlikely(!dip))
		return -1;

	fd = socket(dip->family, SOCK_STREAM, 0);
	if (fd < 0) {
		_SK_ERR("socket error: %s\n", strerror(errno));
		return -1;
	}

	ret = sk_tcp_connect(fd, dip, sip, transparent);
	if (ret != 1) {
		close(fd);
		return -1;
	}

	return fd;
}

int 
sk_tcp_client_nb(ip_port_t *dip, ip_port_t *sip, int transparent, int *wait)
{
	int fd;
	int ret;

	if (unlikely(!dip || !wait))
		return -1;

	fd = socket(dip->family, SOCK_STREAM, 0);
	if (fd < 0) {
		_SK_ERR("socket error: %s\n", strerror(errno));
		return -1;
	}

	sk_set_nonblock(fd, 1);

	ret = sk_tcp_connect(fd, dip, sip, transparent);
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
sk_tcp_server(ip_port_t *ip, int reuse_port, int transparent)
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
		_SK_ERR("setsockopt(SO_REUSEADDR) error: %s\n", 
			strerror(errno));
		close(fd);
		return -1;
	}

	/* reuse port */
	if (reuse_port) {
		opt = 1;
		len = sizeof(opt);
		if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt, len)) {
			_SK_ERR("setsockopt(SO_REUSEPORT) error: %s\n",
					strerror(errno));
			close(fd);
			return -1;
		}
	}

	if (transparent) {
		opt = 1;
		len = sizeof(opt);
		if (setsockopt(fd, IPPROTO_IP, IP_TRANSPARENT, &opt, len)) {
			_SK_ERR("setsockopt(IP_TRANSPARENT) error: %s\n", 
				strerror(errno));
			return -1;
		}
	}

#if 0
	opt = 1;
	len = sizeof(opt);
	if (setsockopt(fd, IPPROTO_TCP, TCP_DEFER_ACCEPT, &opt, len)) {
		_SK_ERR("setsockopt(TCP_DEFER_ACCEPT) error: %s\n",
			strerror(errno));
		close(fd);
		return -1;
	}
#endif

	if (bind(fd, (struct sockaddr*)&svraddr, sizeof(svraddr))){
		_SK_ERR("bind error: %s\n", strerror(errno));
		close(fd);
		return -1;
	}

	if (listen(fd, 25000)) {
		_SK_ERR("listen error: %s\n", strerror(errno));
		close(fd);
		return -1;
	}

	return fd;
}

int 
sk_tcp_connect(int fd, ip_port_t *dip, ip_port_t *sip, int transparent)
{
	int ret;
	long opt;
	socklen_t len;
	sk_addr_t cliaddr;
	sk_addr_t svraddr;

	if (unlikely(fd < 0 || !dip))
		return -1;

	if (_sk_addr_from_ip_port(&svraddr, dip))
		return -1;
	
	/* need set transparent mode and bind local address */
	if (transparent) {
		if (!sip) {
			_SK_ERR("need provide sip for TP mode\n");
			return -1;
		}

		if (_sk_addr_from_ip_port(&cliaddr, sip)) {
			_SK_ERR("invalid source IP in TP mode\n");
			return -1;
		}

		opt = 1;
		len = sizeof(opt);
		if (setsockopt(fd, IPPROTO_IP, IP_TRANSPARENT, &opt, len)) {
			_SK_ERR("setsockopt(IP_TRANSPARENT) error: %s\n", 
				strerror(errno));
			return -1;
		}

		len = sizeof(cliaddr);
		if (bind(fd, (struct sockaddr *)&cliaddr, len)) {
			_SK_ERR("bind error: %s\n", strerror(errno));
			return -1;
		}
	}

	len = sizeof(svraddr);
	ret = connect(fd, (struct sockaddr *)&svraddr, len);

	/* need save local address if not TP mode */
	if (!transparent && sip) {
		if (sk_tcp_get_localaddr(fd, sip))
			return -1;
	}

	if (ret < 0) {
		if (errno == EINPROGRESS)
			return 0;
		
		_SK_ERR("connect error: %s\n", 	strerror(errno));
		return -1;
	}

	return 1;
}

int 
sk_tcp_accept(int fd, ip_port_t *sip, ip_port_t *dip)
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

	if (sip) {
		sip->family = addr._addrsa.sa_family;
		if (addr._addrsa.sa_family == AF_INET) {
			sip->_addr4 = addr._addrv4.sin_addr;
			sip->port = addr._addrv4.sin_port;
		}
		else {
			sip->_addr6 = addr._addrv6.sin6_addr;
			sip->port = addr._addrv6.sin6_port;
		}
	}


	if (dip && sk_tcp_get_localaddr(clifd, dip)) {
		close(clifd);
		return -1;
	}

	return clifd;
}

int 
sk_tcp_accept_nb(int fd, ip_port_t *sip, ip_port_t *dip)
{
	int clifd;
	sk_addr_t addr;
	socklen_t len;

	if (unlikely(fd < 0))
		return -1;
	
	len = sizeof(addr);
	clifd = accept4(fd, (struct sockaddr *)&addr, &len, SOCK_NONBLOCK);
	if (clifd < 0)
		return -1;

	if (sip) {
		sip->family = addr._addrsa.sa_family;
		if (addr._addrsa.sa_family == AF_INET) {
			sip->_addr4 = addr._addrv4.sin_addr;
			sip->port = addr._addrv4.sin_port;
		}
		else {
			sip->_addr6 = addr._addrv6.sin6_addr;
			sip->port = addr._addrv6.sin6_port;
		}
	}

	/* get local address */
	if (dip && sk_tcp_get_localaddr(clifd, dip)) {
		close(clifd);
		return -1;
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

        return 0;
}

int 
sk_tcp_get_localaddr(int fd, ip_port_t *ip)
{
	sk_addr_t addr;
	socklen_t len;

	if (unlikely(fd < 0 || !ip))
		_SK_ERR("invalid argument\n");

	len = sizeof(addr);
	if (getsockname(fd, (struct sockaddr *)&addr, &len)) {
		_SK_ERR("getsockname failed\n");
		return -1;
	}

	if (addr._addrsa.sa_family == AF_INET) {
		ip->_addr4 = addr._addrv4.sin_addr;
		ip->port = addr._addrv4.sin_port;
	}
	else if (addr._addrsa.sa_family == AF_INET6) {
		ip->_addr6 = addr._addrv6.sin6_addr;
		ip->port = addr._addrv6.sin6_port;
	}
	else {
		_SK_ERR("fd %d is not established\n", fd);
		return -1;
	}
	ip->family = addr._addrsa.sa_family;

	return 0;
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
sk_set_nodelay(int fd, int nodelay)
{
	if (fd < 0)
		return -1;

	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay)))
		return -1;

	return 0;
}

int 
sk_set_quickack(int fd, int quickack)
{
	if (fd < 0)
		return -1;

	if (setsockopt(fd, IPPROTO_TCP, TCP_QUICKACK, &quickack, sizeof(quickack)))
		return -1;

	return 0;
}

int 
sk_set_keepalive(int fd)
{
	int opt = 1;
	socklen_t len;

	if (fd < 0) {
		_SK_ERR("invalid parameter\n");
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
sk_set_mark(int fd, int mark)
{
	if (fd < 0)
		return -1;

	if (setsockopt(fd, SOL_SOCKET, SO_MARK, &mark, sizeof(mark)))
		return -1;

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

	m = recv(fd, buf, len, MSG_DONTWAIT|MSG_NOSIGNAL);
	if (unlikely(m < 0)) {
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
sk_send(int fd, const void *buf, size_t len)
{
	int n = 0;

	if (unlikely(fd < 0 || !buf || len < 1)) {
		_SK_ERR("invalid param\n");
		return -1;
	}

	n = send(fd, buf, len, MSG_DONTWAIT);
	if (unlikely(n < 0)) {
		if (unlikely(errno != EINTR || errno != EAGAIN)) 
		{
			_SK_ERR("send error: %s\n", 
				strerror(errno));
			return -1;
		}
		n = 0;
	}

	return n;
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
			if (unlikely(errno != EINTR || errno != EAGAIN)) {
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
sk_unix_server(const char *unixpath, int type)
{

	return 0;
}

int 
sk_unix_client(const char *unixpath, int type)
{
	return 0;
}

int 
sk_unix_connect(int fd, const char *unixpath)
{
	return 0;
}

int 
sk_unix_accept(int fd)
{
	return 0;
}

int 
sk_packet(int type, int protocol)
{
	int fd;

	if (type != SOCK_RAW || type != SOCK_DGRAM)
		return -1;

	if (protocol == 0)
		return -1;

	fd =socket(AF_PACKET, type, htons(protocol));
	if (fd < 0) {
		return -1;
	}

	return fd;
}

void *  
sk_packet_mmap(int fd, size_t len)
{

	return 0;
}

int 
sk_raw(int protocol)
{
	int fd;

	fd = socket(AF_INET, SOCK_RAW, protocol);
	if (fd < 0)
	{
		return -1;
	}

	return fd;
}



