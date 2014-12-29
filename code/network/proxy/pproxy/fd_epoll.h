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
#include <sys/time.h>

#include "gcc_common.h"

typedef enum fd_state {
	FD_CLOSED,
	FD_READY,
	FD_SHUTRD,
	FD_SHURWR,
	FD_STOP,
} fd_state_e;

/*	the event callback function */
typedef int		(*fd_epoll_iocb)(int fd, int e, void *arg1, void *arg2);

typedef struct fd_item {
	int		pevents;	/* prevent event flags in epoll */
	int		events;		/* current event flags need set to epoll */
	int		revents;	/* epoll return event */
	fd_state_e	state;		/* socket fd status */
	void		*arg1;		/* first arg, for accept fd is policy_fd */
	void		*arg2;		/* second arg, for session fd is session_data */
	int		is_tasked;	/* this fd need run in task */
	int		is_updated;	/* this fd need updated into epoll */
	fd_epoll_iocb	iocb;		/* I/O callback function for event */
	struct timeval	timestamp;	/* the task timestamp when added */
} fd_item_t;

typedef struct fd_epoll {
	int		epfd;		/* the epoll fd */
	int		maxfd;		/* max fd can used in epoll */
	int		maxwait;	/* max wait fd for epoll_wait */
	int		nwait;		/* number of fd in epoll */
	struct epoll_event *events;	/* the epoll events */
	int		waittime;	/* the epoll_wait timeout */
	fd_item_t	*maps;		/* fd array */
	int		nmap;		/* current fd map num */
	int		*updates;	/* update fd array */
	int		nupdate;	/* number of fd need update events */
	int		*tasks;		/* the fd need process after poll */
	int		*tasks1;	/* the backed tasks, it switch with tasks for performance */
	int		ntask;		/* number of cached fd */
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
	if (unlikely(!fe))
		return NULL;

	if (unlikely(fd < 0 || fd > fe->maxfd))
		return NULL;

	return &fe->maps[fd];
}


static inline int 
fd_epoll_init_item(fd_item_t *fi) 
{
	if (unlikely(!fi))
		return -1;

	memset(fi, 0, sizeof(fd_item_t));

	return 0;
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
fd_epoll_alloc_update(fd_epoll_t *fe, int fd, int events, fd_epoll_iocb cb);

/**
 *	Flush the fd update into epoll socket by epoll_ctl().
 *
 *	the fd_epoll_update() not commit the event into kernel 
 *	until call this function.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
fd_epoll_flush_update(fd_epoll_t *fe);

/**
 *	Alloc a cache entry for socket fd @fd.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
fd_epoll_alloc_task(fd_epoll_t *fe, int fd, struct timeval *tm, fd_epoll_iocb cb);

/**
 *	Remove a cache entry for socket fd @fd.
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
fd_epoll_flush_task(fd_epoll_t *fe, struct timeval *tm);

/**
 *	The poll function of fd epoll @fe, if fd have event occured, 
 *	call the event callback function in @fd_item_t.
 *
 *	Return the event number if success, -1 on error.
 */
extern int 
fd_epoll_poll(fd_epoll_t *fe);


#endif /* end of FZ_FD_EPOLL_H  */

