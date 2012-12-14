/**
 *	@file	session.h
 *
 *	@brief	The Session Table implement.
 *	
 *	@author	Forrest.zhang
 *
 *	@date	2010-02-23
 */

#ifndef FZ_SESSION_H
#define FZ_SESSION_H

#include <openssl/ssl.h>

#include "epoll_event.h"
#include "packet.h"
#include "ip_addr.h"


/**
 *	The session packet queue.
 */
typedef struct session_queue {
	pktq_t		inq;		/* the input queue */
	pktq_t		outq;		/* the output queue */
	packet_t	*pkt;		/* the current queue */
	packet_t	*blkpkt;	/* blocked packet */
	u_int32_t	pos;		/* pos of next recv byte */
	u_int32_t	nbyte;		/* number of bytes in queue */
} session_queue_t;

/**
 *	Session structure.
 */
typedef struct session {
        int		id;		/* session id */
	u_int8_t	index;		/* index of work thread */
	u_int32_t	magic;		/* session magic number */

	ip_port_t	cliaddr;	/* the client address */
	int		clifd;		/* client socket */
	SSL		*clissl;	/* client SSL */
	session_queue_t	cliq;		/* client packet queue */
	ep_event_t	cliev;		/* client read event */

	ip_port_t	svraddr;	/* the server address */
	int		svrfd;		/* server socket */
	SSL		*svrssl;	/* server SSL */
	session_queue_t	svrq;		/* server packet queue */
	ep_event_t	svrev;		/* server read event */
	
	int		read_pipe;	/* read pipe for splice */
	int		write_pipe;	/* write pipe for splice */

	int		nalloced;	/* alloced packet number */

	/* session flags */
	u_int32_t	is_clierror:1;	/* client error */
	u_int32_t	is_cliclose:1;	/* client closed */
	u_int32_t	is_clisslwait:1;/* client need wait SSL */
	u_int32_t	is_cliblock:1;	/* client side is blocked*/
	
	u_int32_t	is_svrerror:1;	/* server error */
	u_int32_t	is_svrclose:1;	/* server closed */
	u_int32_t	is_svrwait:1;	/* wait TCP connection */
	u_int32_t	is_svrsslwait:1;/* server wait SSL */
	u_int32_t	is_svrblock:1;	/* server is blocked */
} session_t;

/**
 * 	The session tup, using in offline hash find.
 */
typedef struct session_tup {
	int		id;		/* session id for inline */
	ip_port_t	svraddr;	/* server address */
	ip_port_t	cliaddr;	/* client address */
	int		clifd;		/* the client fd */
	int		svrfd;		/* the server fd */
	u_int16_t	index;		/* the index for offline */
} session_tup_t;

/**
 *	Session table
 */
typedef struct session_table {
	session_t	**table;	/* the session table */
	size_t		max;		/* the max table size */
	size_t		nfreed;		/* number of freed entry */
	int		freeid;		/* the freed index */
	pthread_mutex_t	lock;		/* lock for table */
} session_table_t;

/**
 * 	Compair 2 session_tup_t object is same or not.
 *	Using in hash table. 
 *	if clifd > 0, it's inline mode and compare id,
 *	or else it's offline mode and compre sip:sport->dip:dport.
 *	
 *	Return 0 if equal, !=0 if not equal;
 */
extern int 
session_tup_compare(session_tup_t *tup1, session_tup_t *tup2);

/**
 *	Alloc a session table and return it.
 *
 *	Return object ptr if success, NULL on error.
 */
extern session_table_t * 
session_table_alloc(size_t max);

/**
 *	Free session map @m.
 *
 *	No return.
 */
extern void 
session_table_free(session_table_t *st);

/**
 *	Lock session map @m.
 *
 *	No return.
 */
extern void 
session_table_lock(session_table_t *st);

/**
 *	Unlock session map @m.
 *
 *	No return.
 */
extern void 
session_table_unlock(session_table_t *st);

/**
 *	Add new session into session table 
 *
 *	Return the 0 if success, -1 on error.
 */
extern int 
session_table_add(session_table_t *st, session_t *s);

/**
 *	Delete session which id=@id from session table @st.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
session_table_del(session_table_t *st, int id);

/**
 * 	Find a session from session table @st acording
 *	session's ID.
 *
 *	Return pointer if success, NULL on error.
 */
extern session_t *  
session_table_find(session_table_t *st, int id);

/**
 *	Print the whole session table
 *
 *	No return.
 */
extern void 
session_table_print(session_table_t *st);

/**
 *	Print the session information.
 *
 *	No return.
 */
extern void 
session_print(const session_t *s);


#endif /* end of FZ_SESSION_H  */

