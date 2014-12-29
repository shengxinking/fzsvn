/**
 *	@file	ipaddr.h
 *
 *	@brief	IP address for v4/v6, include some APIs to handle
 *		it.
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_SOCK_UTIL_H
#define FZ_SOCK_UTIL_H

#include "ip_addr.h"

/**
 * 	Common socket address.
 */
typedef struct sk_addr {
	union {
		struct sockaddr		sa;	/* common */
		struct sockaddr_in	v4;	/* IPv4 */
		struct sockaddr_in6	v6;	/* IPv6 */
	} _addr;
} sk_addr_t;

#define	_addrsa		_addr.sa
#define	_addrv4		_addr.v4
#define _addrv6		_addr.v6

/**
 * 	Create a TCP client socket and connect to IP port address @dip. 
 * 	If @sip is not NULL, it'll bind @sip before connect.
 * 	if @transparent is set, it'll set IP_TRANSPARENT.
 *
 * 	Return socket fd is success, -1 on error.
 */
extern int 
sk_tcp_client(ip_port_t *dip, ip_port_t *sip, int transparent);

/**
 * 	Create a TCP client socket and connect to IP port address @dip.
 * 	It use NONBLOCK socket before connect. 
 * 	If @sip is not NULL, it'll bind @sip before connect.
 * 	If @transparent != 0, it'll set IP_TRANSPARENT before 
 * 	bind()/connect().
 * 	If connection is finished, set @wait = 0, 
 * 	or set @wait = 1 and need check connection is finished later.
 *
 * 	Return socket fd is success, -1 on error.
 */
extern int 
sk_tcp_client_nb(ip_port_t *dip, ip_port_t *sip, int transparent, int *wait);

/**
 *	Create TCP server socket accoring IP port address @ip. 	
 * 	If @reuse_port is set, it'll reuse_port
 *	If @transparent is set, it'll set IP_TRANSPARENT before 
 *	bind().
 *
 * 	Return socket fd is success, -1 on error.
 */
extern int
sk_tcp_server(ip_port_t *ip, int reuse_port, int transparent);

/**
 * 	Connect socket @fd to server IP port address @ip.
 *	
 *	Return 0 if success, 1 need check later, -1 on error.
 */
extern int 
sk_tcp_connect(int fd, ip_port_t *dip, ip_port_t *sip, int transparent);

/**
 *	Accept an TCP client and save client IP/port address in @sip,
 *	save server IP/port address in @dip.
 *
 *	Return blocked client socket fd if success, -1 on error.
 */
extern int 
sk_tcp_accept(int fd, ip_port_t *sip, ip_port_t *dip);

/**
 *	Accept an TCP client and save client IP/port address in @sip,
 *	save server IP/port address in @dip.
 *
 *	Return nonblock client socket fd if success, -1 on error.
 */
int 
sk_tcp_accept_nb(int fd, ip_port_t *sip, ip_port_t *dip);

/**
 * 	Check Socket @fd connection is success or failed.
 *
 *	Return 0 if success, -1 if error.
 */
extern int 
sk_is_connected(int fd);

/**
 *	Get local address of connected socket.
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
sk_tcp_get_localaddr(int fd, ip_port_t *ip);

/**
 * 	Set socket @fd to NONBLOCK mode if @nbio != 0, clear
 * 	socket @fd to BLOCKED mode if @nbio is 0.
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
sk_set_nonblock(int fd, int nbio);

/**
 *	Set socket @fd to NODELAY mode if @nodelay != 0, clear
 *	socket @fd NODELY mode if @nodelay == 0.
 *	it affect the Nagle algorithm in kernel.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
sk_set_nodelay(int fd, int nodelay);

/**
 *	Set socket @fd to QUICKACK mode if @quickack != 0,
 *	clear socket @fd QUICKACK mode if @quickack == 0.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
sk_set_quickack(int fd, int quickack);

/**
 * 	Set TCP socket @fd keepalive if @keepalive != 0, clear
 * 	@fd keepalive if @keepalive is 0.
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
sk_set_keepalive(int fd);

/**
 *	Set mark @mark on socket @fd.
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
sk_set_mark(int fd, int mark);

/**
 *	Resolve the domain name and save it into IP. 
 *	If family is AF_INET, only return IPv4 address.
 *	If family is AF_INET6, only return IPv6 address.
 *	If family is 0, return a valid address if exist.
 *
 *	Return 0 if sucess, -1 on error.
 */
extern int 
sk_gethostbyname(const char *domain, int family, ip_addr_t *ip);

/**
 * 	Recv data from socket @fd, the data saved in @buf, it
 * 	the @buf size is @len.
 *
 * 	If peer close connect, the @closed is set 1.
 *
 * 	Return recevied bytes number if success, -1 on error.
 */
extern int 
sk_recv(int fd, void *buf, size_t len, int *closed);

/**
 * 	Send data in @buf to socket @fd, the @buf length is
 * 	@len.
 *
 * 	Return send bytes number is success, -1 on error.
 */
extern int 
sk_send(int fd, const void *buf, size_t len);

/**
 * 	Send data in @buf to socket @fd, the @buf length is
 * 	@len.
 *
 * 	Return send bytes number is success, -1 on error.
 */
extern int 
sk_send_all(int fd, const void *buf, size_t len);

/**
 *	Create a client unix socket and connect to server @path.
 *	socket type @type need to SOCK_STREAM | SOCK_DGRAM
 *
 * 	Return >= 0 if success, -1 on error.
 */
extern int 
sk_unix_client(const char *path, int type);

/**
 *	Create a server unix socket and return it.
 *	socket type @type need to SOCK_STREAM | SOCK_DGRAM
 *
 * 	Return >= 0 if success, -1 on error.
 */
extern int
sk_unix_server(const char *path, int type);

/**
 *	Create a packet socket and return it.
 *
 * 	Return >= 0 if success, -1 on error.
 */
extern int 
sk_packet(int type, int protocol);

/**
 *	mmap packet socket @fd to ring buffer which size 
 *	is @len, and return mmaped ring buffer.
 *	
 *	Return pointer if success, NULL if failed.
 */
extern void * 
sk_packet_mmap(int fd, size_t len);

/**
 *	Create a raw socket and return it.
 *
 * 	Return >= 0 if success, -1 on error.
 */
extern int 
sk_raw(int protocol);


#endif /* end of FZ_SOCK_UTIL_H  */


