/**
 *	@file	epoll_event.h
 *
 *	@brief	epoll_event APIs.
 *	
 *	@author	Forrest.zhang
 *
 *	@date	2012-08-28
 */

#ifndef FZ_EPOLL_EVENT_H
#define FZ_EPOLL_EVENT_H

struct ep_event;

typedef void (*ep_event_func)(struct ep_event *ev);

typedef struct ep_event {
	int		fd;		/* the socket putted in epoll */
	u_int32_t	is_read:1;	/* is read event */
	u_int32_t	is_write:1;	/* is write event */
	void		*data;		/* the private data, like session */
	ep_event_func	read_func;	/* the read event handler */
	ep_event_func	write_func;	/* the write event handler */
	ep_event_func	error_func;	/* error event handler */
} ep_event_t;

typedef struct ep_stat {
	u_int64_t	naccept;	/* number of accept event */
	u_int64_t	nconn;		/* number of connect events */
	u_int64_t	nread;		/* number of read events */
	u_int64_t	nwrite;		/* number of write events */
	u_int64_t	nerror;		/* number of error events */
} ep_stat_t;

typedef struct ep_ctx {
	int		epfd;		/* epoll fd */
	int		nfd;		/* number of socket in epoll */
	struct epoll_event *events;	/* the events */
	int		nevent;		/* max events number */

	ep_stat_t	stat;		/* epoll statistic data */
} ep_ctx_t;

/**
 *	Create a epoll context for epoll event.
 *
 *	Return the ep_ctx_t object.
 */
extern ep_ctx_t * 
ep_event_create(int max);

/**
 *	Destory epoll context 
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
ep_event_destroy(ep_ctx_t *ctx);

/**
 *	Add the listen socket into epoll event.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
ep_event_add_accept(ep_ctx_t *ctx, ep_event_t *ep);

/**
 *	Add the connect socket into epoll event.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
ep_event_add_connect(ep_ctx_t *ctx, ep_event_t *ev);

/**
 *	Add read event socket into epoll event
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
ep_event_add_read(ep_ctx_t *ctx, ep_event_t *ev);

/**
 *	Add write event socket into epoll event
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
ep_event_add_write(ep_ctx_t *ctx, ep_event_t *ev);

/**
 *	Delete event from epoll event.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
ep_event_del_read(ep_ctx_t *ctx, ep_event_t *ev);

/**
 *	Delete write event from epoll event.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
ep_event_del_write(ep_ctx_t *ctx, ep_event_t *ev);

/**
 *	Delete event from epoll event.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
ep_event_del(ep_ctx_t *ctx, ep_event_t *ev);

/**
 *	Do epoll wait, it'll check the epoll event and
 *	call @handler function in ep_event_t. the wait
 *	timeout is @timeout.
 *
 *	Return the number of event if success, -1 on error.
 */
extern int
ep_event_loop(ep_ctx_t *ctx, int timeout);

#endif /* end of FZ_  */


