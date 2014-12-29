/**
 *	@file	listener.h
 *
 *	@brief	TCP listener structure and APIs
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_LISTENER_H
#define FZ_LISTENER_H

#include <sys/param.h>

#include "cblist.h"
#include "ip_addr.h"
#include "ssl_util.h"
#include "certset.h"
#include "proxy_common.h"

/**
 *	Listener config.
 */
typedef struct listener_cfg {
	char		name[MAX_NAME];
	ip_port_t	address;	/* address */
	int		ssl;		/* SSL status */
	certset_t	*cert;		/* certificate set */
} listener_cfg_t;

/**
 *	Listener structure.
 */
typedef struct listener {
	listener_cfg_t	cfg;		/* config */

	int		refcnt;		/* reference count */
	cblist_t	list;		/* list into proxy's @ltnlist */
} listener_t;

/**
 *	Listener running data.
 */
typedef struct listener_data {
	ssl_ctx_t	*sslctx;	/* ssl context */
	int		refcnt;		/* reference count */
} listener_data_t;

/**
 *	Listener fd using in @worker thread.
 */
typedef struct listener_fd {
	int		fd;		/* listen fd */
	ip_port_t	address;	/* listen address */

	void		*worker;	/* worker data */
	void		*thread;	/* thread data */
	void		*policy;	/* policy data */
	
	cblist_t	ssnlist;	/* sessions for this listener */	
	int		nssn;

	cblist_t	list;
} listener_fd_t;


/**
 *	Alloc a new listener and return it.
 *
 *	Return pointer if success, -1 on error.
 */
extern listener_t * 
listener_alloc(void);

/**
 *	Clone a listener @ltn, increment @refcnt
 *
 *	Return @ltn if success, NULL on error.
 */
extern listener_t * 
listener_clone(listener_t *ltn);

/**
 *	Release a listener @ltn until @refcnt is zero.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
listener_free(listener_t *ltn);

/**
 *	Print listener @ltn config. output prefix string
 *	is @prefix.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
listener_print(const listener_t *ltn, const char *prefix);

/**
 *	Alloc a new listener running data for listener @ltn
 *	and return it.
 *
 *	Return pointer if success, NULL on error.
 */
extern listener_data_t *  
listener_alloc_data(listener_t *ltn);

/**
 *	Clone a listener_data @ltndata, incement @refcnt.
 *
 *	Return @ltndata if success, NULL on error.
 */
extern listener_data_t * 
listener_clone_data(listener_data_t *ltndata);

/**
 *	Free listener_data @ltndata until @refcnt is zero.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
listener_free_data(listener_data_t *ltndata);

/**
 *	Alloc a new listener_fd for listener @ltn and 
 *	return it, if @transparent is 1, set listener as
 *	transparent mode.
 *
 *	Return pointer if success, NULL on error.
 */
extern listener_fd_t * 
listener_alloc_fd(listener_t *ltn, int transparent);

/**
 *	Free a listener_fd alloced by @listener_alloc_fd().
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
listener_free_fd(listener_fd_t *ltnfd);

/**
 *	Accept a client connection in event call.
 *
 *	Return 0 if success, NULL on error.
 */ 
extern int 
listener_accept(int fd, int events, void *arg);

#endif /* end of FZ_LISTENER_H */

