/**
 *	@file	debug.h
 *
 *	@brief	The debug function is used to debug program file.
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_DEBUG_H
#define FZ_DEBUG_H


#define DBG_STR_LEN	1023

/**
 *	A log record.
 */
typedef struct dbg_msg {
	struct dbg_msg	*next;
	int		len;
	char		msg[DBG_STR_LEN + 1];
} dbg_msg_t;

#define DBG_MSG_LEN	(sizeof(dbg_msg_t))

/**
 *	The log database, it'll using queue structure.
 */
typedef struct dbg_db {
	int		capacity;
	int		nmsgs;
	void		*pool;
	dbg_msg_t	*head, *tail;
	dbg_msg_t	*freed;
} dbg_db_t;


/**
 *	Alloc a dbg_db_t object.
 *
 *	Return 0 if success, -1 on error.
 */
extern dbg_db_t *
dbg_alloc_db(int capacity);


/**
 *	Free a dbg_db_t object.
 *
 *	No return.
 */
extern void
dbg_free_db(dbg_db_t *db);


/**
 *	Add a log message into @db.
 *
 *	No return.
 */
extern void 
dbg_log_db(dbg_db_t *db, const char *fmt, ...);


/**
 *	Save log database @db into file @file according 
 *	log sequence, one line for one log.
 *
 *	No return.
 */
extern void 
dbg_save_db(dbg_db_t *db, const char *file);


/**
 *	Add a log into fd, the fd is file descript.
 *
 *	No return.
 */
extern void 
dbg_log_fd(int fd, const char *fmt, ...);

/**
 *	Recv log from fd and add it to db
 *
 *	No return.
 */
extern void
dbg_log_recv(dbg_db_t *db, int fd);


/**
 *	Print the stack of crashed program.
 *
 *	No return.
 */
extern void
dbg_stack(void);

#endif /* end of FZ_DEBUG_H  */

