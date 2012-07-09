/**
 *	@file	sock.h
 *
 *	@brief	The socket APIs.
 *
 *	@author	Forrest.Zhang
 */

#ifndef FZ_SOCK_H
#define FZ_SOCK_H

#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>


/**
 *	Create a server socket, it bind address @ip:@port, the @port is 
 *	network byte-sequence. The SO_REUSEADDR is set when socket 
 *	create succeeded.
 *
 *	Return socket fd if >= 0, -1 on error.
 */
extern int 
sock_tcpsvr(u_int32_t ip, u_int16_t port);


/**
 *	Create a socket and connect to address @ip:@port.
 *
 *	Return socket fd if >= 0, -1 on error.
 */
extern int 
sock_tcpcli(u_int32_t ip, u_int16_t port);


/**
 * 	Create a socket and connect to address @ip:@port,
 *	The socket using nonblock mode.
 *	
 *	If @wait == 1, it need wait to connect finished.
 * 	
 * 	Return socket fd if >= 0; -1 on error.
 */
extern int 
sock_tcpcli_nb(u_int32_t ip, u_int16_t port, int *wait);


/**
 * 	Check a socket @fd is connect success or failed, 
 *	the @fd is nonblock mode.
 *
 * 	Return 1 if connect success, 0 if failed.
 */
extern int 
sock_tcpcli_success(int fd);


/**
 *	Set the socket fd to block/nonblock mode acording @nbio, if @nbio
 *	is zero, set block mode, or else set non-block mode
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
sock_set_nbio(int fd, int nbio);


/**
 *	Connect to server @ip:@port using socket @fd. 
 *
 *	Return 1 if success, 0 need check the connect status, -1 on error.
 */
extern int 
sock_connect(int fd, u_int32_t ip, u_int16_t port);


/**
 * 	Check socket @fd connect succeed or failed.
 *
 *	Return 1 if connect succeed, 0 if need wait, 
 *	-1 if failed.
 */
int 
sock_is_connected(int fd);


/**
 *	Accept client connection using socket @fd.
 *
 *	Return client fd if success, -1 on error.
 */
extern int 
sock_accept(int fd, u_int32_t *dip, u_int16_t *dport);


/**
 *	Send all data to socket @fd.
 *
 *	Return >=0 if send success, -1 on error.
 */
extern int 
sock_send(int fd, const char *buf, size_t len);


/**
 *	Recv data from socket @fd and store them in @buf, the @buf 
 *	length is @len. 
 *	If the socket is closed, the @close set 1, or else the @close set 0.
 *
 *	Return >= 0 if success, -1 on error. 
 */
extern int  
sock_recv(int fd, char *buf, size_t len, int *closed);


#endif /* end of FZ_SOCK_H */

