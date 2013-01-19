/*
 *	@file	session.h
 *
 *	@brief	TCP session table for whole proxy
 *
 *
 */

#ifndef FZ_SESSPOOL_H
#define FZ_SESSPOOL_H

#include <sys/types.h>
#include <pthread.h>

/**
 *	session flags
 */
#define SESSION_BIT_CLICLOSE	0
#define SESSION_BIT_SVRCLOSE	1
#define SESSION_BIT_CLEAR	2
#define SESSION_CLICLOSE	(1 << SESSION_BIT_CLICLOSE)
#define SESSION_SVRCLOSE	(1 << SESSION_BIT_SVRCLOSE)
#define SESSION_CLEAR		(1 << SESSION_BIT_CLEAR)


/**
 *	Define struct packet for recv data.
 */
typedef struct packet {
	struct packet	*next;
	u_int16_t	capacity;
	u_int16_t	size;
	u_int16_t	pos;
	u_int16_t	start;
	u_int8_t	data[0];
} packet_t;

/**
 *	Define packet queue for store packet in session. 
 */
typedef struct pktqueue {
	packet_t	*head;
	packet_t	*tail;
	u_int32_t	size;
} pktqueue_t;


/**
 *	Session is a tcp session, in proxy it have two connection, 
 *	one connect to client, and the other is connect to server.
 */
typedef struct session {

	struct session	*next, *prev;

	int		id;
	int		flags;

	/* two file descriptor for socket */
	int		clifd;
	int		svrfd;
	
	/* client and server infomation */
	u_int32_t	sip;
	u_int32_t	dip;
	u_int16_t	sport;
	u_int16_t	dport;

	/**
	 * In multi-thread environment, to reduce lock time, each
	 * session need process by only one thread, this avoid lock
	 * session pool long time. so we need remember thread id in
	 * session structure. It's no use in single thread environment.
	 */
	pthread_t	owner;

	/**
	 *	Define Client data cache in session. Sometimes we need 
	 *	cache some client data in session before send them to Server.
	 */
	pktqueue_t	cli_input;
	pktqueue_t	cli_output;
	packet_t	*cli_current;

	/**
	 *	Define Server data cache in session. Sometimes we need
	 *	cache some server data in session before send the to Client.
	 */
	pktqueue_t	svr_input;
	pktqueue_t	svr_output;
	packet_t	*svr_current;
} session_t;



#define SESSPOOL_HSIZE		4

/**
 *	session_table flags
 */
#define SESSPOOL_BIT_LOCK	0
#define	SESSPOOL_LOCK		(1 << SESSPOOL_BIT_LOCK)
#define SESSPOOL_IS_LOCK(flags)	(flags & SESSPOOL_LOCK)
#define SESSPOOL_SET_LOCK(flags)	(flags |= SESSPOOL_LOCK);
#define SESSPOOL_CLR_LOCK(flags)	(flags &= ~SESSPOOL_LOCK);


/**
 *	Session table is used to store all sessions. You can
 *	add a new session in pool, delete a session from pool,
 *	and find a session in pool. if
 */
typedef struct sesspool {
	u_int32_t	size;
	u_int32_t	nused;

	void		*pool;
	void		*hash;
	void		*map;
	int		freeid;

	u_int32_t	flags;

	/* for thread-safe */
	pthread_mutex_t	lock;

} sesspool_t;

/**
 *	Alloc a session pool, pool's size is @size. If the @lock is not 
 *	zero, this pool is protected by pthread lock, so it's thread 
 *	safe.
 *
 *	Return session pool if success, NULL on error.
 */
extern sesspool_t *
sesspool_alloc(int size, int lock);

/**
 *	Free a session pool #@sp which is alloced by function @sesspool_alloc.
 *
 *	No return.
 */
extern void 
sesspool_free(sesspool_t *st);

/**
 *	Find a session in session pool @sp according socket fd @fd.
 *
 *	Return the session if find, NULL on no found.
 */
extern session_t *
sesspool_find(sesspool_t *sp, int fd);

/**
 *	Add a new session into session pool @sp, the @fd is the client socket,
 *	it's unique in whole process.
 *
 *	Return the new session if success, NULL on error.
 */
extern session_t *
sesspool_add(sesspool_t *sp, int clifd, int svrfd);

/**
 *	Delete a session from session pool @sp acording socket fd @fd.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
sesspool_del(sesspool_t *sp, int id);

/**
 *	Make clifd %d and svr fd %d as a pair in one session
 *
 *	Return 0 if success, -1 on error.
 */
int
sesspool_map(sesspool_t *sp, int clifd, int svrfd);

/**
 *	Print session pool @sp.
 *
 *	No return.
 */
extern void
sesspool_print(sesspool_t *sp);



/**
 *	Put a packet @pkt to tail of  packet queue @q
 *
 *	Return 0 if success, -1 on error.
 */
extern int
pktqueue_in(pktqueue_t *q, packet_t *pkt);

/**
 *	Get a packet from head of packet queue @q, and
 *	the packet'll remove from queue.
 *
 *	Return the packet if success, NULL on error.
 */
extern packet_t *
pktqueue_out(pktqueue_t *q);

#endif /* end of FZ_SESSION_H */
