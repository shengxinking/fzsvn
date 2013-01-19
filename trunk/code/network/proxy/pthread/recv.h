/**
 *	@file	recv.h
 *
 *	@brief	process client connection and recv data from client.
 *
 *	@author	Forrest.zhang
 *	
 *	@date	2010-07-07
 */

#ifndef FZ_RECV_H
#define FZ_RECV_H

#define RECV_TIMEOUT	100
#define RECV_PKTSIZE	1500

#include "proxy.h"

typedef struct _recv {
	int		epfd;	/* the epoll socket */
	fd_array_t	infds;	/* the new client fd */
	fd_array_t	delfds;	/* the delete session */
} recv_t;


/**
 *	Recv thread main function.
 *
 *	No return.
 */
extern void *
recv_run(void *arg);

/**
 *	Add a new client fd %fd and it's session id @id into
 *	recv thread process queue.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
recv_add_fd(u_int32_t id, int fd);

/**
 *	Delete a new socket fd @fd and it's session id @id from
 *	Recv thread.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
recv_del_fd(u_int32_t id, int fd);


#endif /* end of FZ_RECV_H */

