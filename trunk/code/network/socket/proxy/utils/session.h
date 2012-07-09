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

#include "packet.h"

#define SESSION_MAGIC	0x12345678

/**
 *	the parse information 
 */
typedef struct parse_info {
	void		*clihttp;	/* client http parse info */
	void		*svrhttp;	/* server http parse info */
} parse_info_t;


/**
 *	The packet cache.
 */
typedef struct sesscache {
	pktq_t		input;		/* the input queue */
	pktq_t		output;		/* the output queue */
	packet_t	*pkt;		/* the cache queue */
	packet_t	*blkpkt;	/* blocked packet */
	u_int32_t	pos;		/* the pos of next recv byte */
} sesscache_t;


/**
 *	Session structure.
 */
typedef struct session {
        int		id;		/* session id */
	int		index;		/* the index of work thread */
	u_int32_t	magic;		/* the session magic number */
	u_int32_t	sip, dip;	/* source/dest IP */
	u_int16_t	sport, dport;	/* source/dest port */
	int		clifd, svrfd;	/* client/server socket */
	SSL		*clissl;	/* client SSL */
	SSL		*svrssl;	/* server SSL */
	sesscache_t	clicache;	/* client packet cache */
	sesscache_t	svrcache;	/* the server packet cache */
	int		nalloced;	/* number of alloced packet in session */

	u_int32_t	is_clierror:1;	/* client side error */
	u_int32_t	is_cliclose:1;	/* client side closed */
	u_int32_t	is_clisslwait:1;/* client side need wait SSL connection */
	u_int32_t	is_cliblock:1;	/* client side is blocked to send data */
	
	u_int32_t	is_svrerror:1;	/* server side error */
	u_int32_t	is_svrclose:1;	/* server side closed */
	u_int32_t	is_svrwait:1;	/* server side need wait TCP connection */
	u_int32_t	is_svrsslwait:1;/* server side need wait SSL connection */
	u_int32_t	is_svrblock:1;	/* server side is blocked to send data */

	parse_info_t 	pinfo;		/* the private data for parse */
} session_t;


/**
 * 	The session tup, using in inline/offline hash find.
 */
typedef struct session_tup {
	int		id;		/* the session id for inline */
	u_int32_t	sip, dip;	/* source/dest ip for offline */
	u_int16_t	sport, dport;	/* source/dest port for offline */
	int		clifd;		/* the client fd */
	int		svrfd;		/* the server fd */
	u_int16_t	index;		/* the index, for offline */
} session_tup_t;


/**
 *	Session table
 */
typedef struct session_table {
	session_t	**table;	/* the session table */
	int		capacity;	/* the max table size */
	int		nfreed;		/* number of freed entry */
	int		freeid;		/* the current freed index */
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
session_table_alloc(int capacity);


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
 *	Get a free id for a new session, the return id range is 
 *	(@low <= @id < @high).
 *
 *	Return the id if success, -1 on error.
 */
extern int 
session_table_add(session_table_t *st, session_t *s);


/**
 *	Free the id @id in session map @m
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
session_print(session_t *s);


#endif /* end of FZ_SESSION_H  */

