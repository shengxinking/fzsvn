/**
 *	@file	connection.h
 *
 *	@brief	The TCP connection structure and APIs for proxy
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_CONNECTION_H
#define FZ_CONNECTION_H

#include "task.h"
#include "ip_addr.h"
#include "sock_util.h"
#include "ssl_util.h"
#include "cblist.h"
#include "worker.h"
#include "thread.h"
#include "proxy_debug.h"

/**
 *	connction flags 
 */
#define	CONN_F_ERROR	0x0001		/* recv/send error */
#define	CONN_F_HSK	0x0002		/* tcp handshake */
#define	CONN_F_SSLHSK	0x0004		/* ssl handshake */
#define	CONN_F_SHUTRD	0x0100		/* shutdown read */
#define	CONN_F_SHUTWR	0x0200		/* shutdown write */
#define	CONN_F_SSLSHUT	0x0400		/* ssl shutdown */
#define CONN_F_BLOCKED  0x1000          /* send data blocked */
#define	CONN_F_CLOSED	(CONN_F_SHUTRD | CONN_F_SHUTWR)

#define	CONN_IS_CLOSED(c)	(((c)->flags & CONN_F_CLOSED) == CONN_F_CLOSED)

struct session;

/**
 *	Connection structure.
 */
typedef struct connection {
	struct session	*s;		/* session it belong */
	int		fd;		/* socket fd */
	SSL		*ssl;		/* SSL object */
	int		dir;		/* direction: 0 client, 1 server */
	const char	*side;		/* side */	
	ip_port_t	peer;		/* peer address */
	ip_port_t	local;		/* local address */
	cblist_t	in;		/* input packet queue */
	cblist_t	out;		/* output packet queue */
	task_t		task;		/* task */
	int		flags;		/* flags */
} connection_t;

/**
 *	Init onnection @c using session @s. @dir 0 is client,
 *	1 is server, side is "client"/"server" for debug output.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
conn_init(connection_t *c, struct session *s, int dir, const char *side);

/**
 *	Free connection @c resource using session @s.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
conn_free(connection_t *c);

/**
 *	Recv data in connection @c, it'll just
 *
 */
static inline int 
conn_raw_recv(int fd, SSL *ssl, void *buf, int len, 
	      int *close, int *handshake)
{
	int ret;
	if (unlikely(!buf || len < 1 || !handshake || !close))
		ERR_RET(-1, "invalid argument\n");
	
	*close = 0;
	*handshake = 0;
	if (ssl)
		ret = ssl_recv(ssl, buf, len, close, handshake);
	else
		ret = sk_recv(fd, buf, len, close);

	return ret;
}

static inline int 
conn_raw_send(int fd, SSL *ssl, void *buf, int len)
{
	int n;
	
	if (unlikely(!buf || len < 1))
		ERR_RET(-1, "invalid argument\n");

	if (ssl)
		n = ssl_send(ssl, buf, len);
	else
		n = sk_send(fd, buf, len);

	return n;
}

/**
 *	Receive data from connection @c.
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
conn_recv_data(int fd, int events, void *arg);

/**
 *	Send data to connection @c.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
conn_send_data(int fd, int events, void *arg);

/**
 *	Check connection @c handshake success or not.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
conn_handshake(int fd, int events, void *arg);

/**
 *	Do shutdown on connection @c. Now used in SSL shutdown.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
conn_shutdown(int fd, int events, void *arg);

#endif /* end of FZ_CONNECTION_H */


