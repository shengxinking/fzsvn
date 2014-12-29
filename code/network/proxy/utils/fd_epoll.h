/**
 *	@file	fd_epoll.h
 *
 *	@brief	Socket fd epoll APIs for proxy.
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_FD_EPOLL_H
#define FZ_FD_EPOLL_H

#include <sys/epoll.h>

#include "gcc_common.h"

typedef enum fd_state {
	FD_CLOSED,
	FD_READY,
} fd_state_e;

#define	FD_IN		(EPOLLIN | EPOLLRDHUP)
#define	FD_OUT		(EPOLLOUT)

/*	the event callback function */
typedef int		(*fd_func)(int fd, int events, void *arg);

typedef struct fd_item {
	int		events;		/* current event flags need set to epoll */
	int		pevents;	/* prevent event flags in epoll */
	int		revents;	/* epoll return event */
	fd_state_e	state;		/* socket fd status */
	void		*arg;		/* for cb() argument @arg1 */
	int		is_updated;	/* this fd need updated into epoll */
	fd_func		iocb;		/* I/O callback function for event */
} fd_item_t;

typedef struct fd_epoll {
	int		epfd;		/* the epoll fd */
	int		maxfd;		/* max fd can used in epoll */
	struct epoll_event *events;	/* the epoll events */
	int		maxwait;	/* max wait fd for epoll_wait */
	int		waittime;	/* the epoll_wait timeout */
	fd_item_t	*maps;		/* fd array */
	int		nmap;		/* current fd map num */
	int		*updates;	/* update fd array */
	int		nupdate;	/* number of fd need update events */
} fd_epoll_t;


/**
 *	Init fd epoll structure @fe, the max socket is @maxfd
 *	epoll timeout is @timeout.
 *
 *	Return 0 if success, -1 on error.
 */
extern fd_epoll_t * 
fd_epoll_alloc(int maxfd, int wait_time);

/**
 *	Free resources in fd epoll structure @fe.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
fd_epoll_free(fd_epoll_t *fe);

/**
 *	Return the fd_item object in map which index is @fd.
 *
 *	Return object if success, NULL on error.
 */
static inline fd_item_t * 
fd_epoll_map(fd_epoll_t *fe, int fd)
{
	if (unlikely(!fe|| !fe->maps))
		return NULL;

	if (unlikely(fd < 0 || fd > fe->maxfd))
		return NULL;

	return &fe->maps[fd];
}

/**
 *	Alloc a update fd in fd epoll @fe, the fd state is @state, 
 *	new epoll event is @ev,  callback function is @cb.
 *
 *	Need call fd_epoll_flush_update() to apply changes into epoll
 *	socket.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
fd_epoll_add_event(fd_epoll_t *fe, int fd, int events, fd_func cb);

/**
 *	Flush the fd update into epoll socket by epoll_ctl().
 *
 *	the fd_epoll_add() not commit the event into kernel 
 *	until call this function.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
fd_epoll_flush_events(fd_epoll_t *fe);

/**
 *	The poll function of fd epoll @fe, if fd have event occured, 
 *	call the event callback function in @fd_item_t.
 *
 *	Return the event number if success, -1 on error.
 */
extern int 
fd_epoll_poll(fd_epoll_t *fe);


#endif /* end of FZ_FD_EPOLL_H  */

