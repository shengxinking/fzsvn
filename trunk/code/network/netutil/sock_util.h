/**
 *	@file	socket_util.h
 *
 *	@brief	socket function using ip_addr structure.
 *		
 *	@author	Forrest.zhang
 *
 *	@date	2012-08-20
 */

#ifndef FZ_SOCK_UTIL_H
#define FZ_SOCK_UTIL_H

#include <netinet/in.h>

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
 *	Create TCP server socket accoring IP port address @ip. 	
 *
 * 	Return socket fd is success, -1 on error.
 */
extern int
sk_tcp_server(ip_port_t *ip);

/**
 * 	Create a TCP client socket and connect to IP port address
 * 	@ip.
 *
 * 	Return socket fd is success, -1 on error.
 */
extern int 
sk_tcp_client(ip_port_t *ip);

/**
 * 	Create a TCP client socket and connect to IP port address
 * 	@ip, it use NONBLOCK socket before connect. If connection
 *	is finished, set @wait = 0, or set @wait = 1 and need
 *	check connection is finished later.
 *
 * 	Return socket fd is success, -1 on error.
 */
extern int 
sk_tcp_client_nb(ip_port_t *ip, int *wait);

/**
 * 	Connect socket @fd to server IP port address @ip.
 *	
 *	Return 0 if success, 1 need check later, -1 on error.
 */
extern int 
sk_tcp_connect(int fd, ip_addr_t *ip, u_int16_t port);

/**
 * 	Check Socket @fd connection is success or failed.
 *
 *	Return 0 if success, -1 if error.
 */
extern int 
sk_is_connected(int fd);

/**
 *	Accept an client and save it's IP port address in @ip.
 *
 *	Return client socket fd if accept success, -1 on error.
 */
extern int 
sk_accept(int fd, ip_port_t *ip);

/**
 * 	Set socket @fd to NONBLOCK mode if @nbio != 0, clear
 * 	socket @fd to BLOCKED mode if @nbio is 0.
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
sk_set_nonblock(int fd, int nbio);

/**
 * 	Set TCP socket @fd keepalive if @keepalive != 0, clear
 * 	@fd keepalive if @keepalive is 0.
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
sk_set_keepalive(int fd);

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
 *	Resolve the domain name and save it into IP.
 *
 *	Return 0 if sucess, -1 on error.
 */
extern int 
sk_gethostbyname(ip_addr_t *ip, const char *domain);

#endif /* end of FZ_SOCK_UTIL_H  */


