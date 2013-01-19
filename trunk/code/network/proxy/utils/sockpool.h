/**
 *	@file       sockpool.h
 *	@brief      a simple TCP socket pool to Server, it's used to keepalive
 *
 *	@date       2007-09-30
 *  
 */

#ifndef FZ_SOCKPOOL_H
#define FZ_SOCKPOOL_H

#include <sys/types.h>
#include <pthread.h>


/* hash table size */
#define SOCKPOOL_HSIZE		10001

/**
 *	server socket pool.
 */
typedef struct sockpool {
	u_int32_t	size;
	u_int32_t	success;
	u_int32_t	failed;
	u_int32_t	nused;
	void		*pool;
	void		*hash;
	u_int32_t	memsize;
	pthread_mutex_t lock;		/* for thread-safe */
} sockpool_t;

/**
 *	Create a socket pool, a client can get a socket(connect to server) 
 *	from this pool, and the client finished job, it return the socket, 
 *	and this socket can reused by other client. We also call it server-pool.
 *	If the locked is not zero, the returned sockpool is thread-safe.
 *
 *	Return socket pool if success, NULL on error.
 */
extern sockpool_t *
sockpool_alloc(size_t size, int locked);

/**
 *	Free socket pool @pool which is alloced using @sockpool_alloc.
 *
 *	No return.
 */
extern void 
sockpool_free(sockpool_t *pool);

/**
 *	Get a socket which is connected to address @ip:@port from socket 
 *	pool @pool.
 *
 *	Return the socket if >= 0, -1 on error.
 */
extern int 
sockpool_get(sockpool_t *pool, u_int32_t ip, u_int16_t port);

/**
 *	Put socket @fd to socket pool @pool, if @close is not zero, the 
 *	socket need close and not reuse. 
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
sockpool_put(sockpool_t *pool, int fd, int isclose);


/**
 *	Print socket pool @pool. It's a debug function.
 *
 *	No return.
 */
extern void 
sockpool_print(const sockpool_t *pool);


#endif /* end of FZ_SOCKPOOL_H */

