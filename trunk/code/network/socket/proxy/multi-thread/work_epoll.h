/*
 *	@file	work.h
 *
 *	@brief	 
 *	
 *	@author	Forrest.zhang
 *
 *	@date	2007-07-30
 */

#ifndef FZ_WORK_H
#define FZ_WORK_H

#include <sys/types.h>
#include <pthread.h>
#include <sys/epoll.h>

/**
 *	struct workfd is the base unit of client which accept 
 *	thread assign to work thread. 
 */
typedef struct workfd {
	int		fd;	/* client fd after accept */
	u_int32_t	ip;	/* client IP address */
	u_int16_t	port;	/* client port address */
	u_int16_t	ssl;	/* is SSL or not */
} workfd_t;

/**
 *	The private data struct of work thread.
 */
typedef struct work_info {
	/* the input array recved acept thread assigned client fd */
	workfd_t	*fds;		/* the array store client's workfd */
	workfd_t	*fds2;		/* the second array store client's workfd */
	int		nfd;		/* the workfd number in @fds */
	int		maxfds;		/* max workfd in @fds */
	pthread_mutex_t	lock;		/* locked it */
	int		send_epfd;	/* send epoll fd */
	int		recv_epfd;	/* recv epoll fd */
	struct epoll_event *events;	/* epoll event */
	int		nevents;	/* number of epoll event */
	int		low;		/* the low free id, included */
	int		high;		/* the max free id, not include */
	int		start;		/* the start position of free id */
} work_info_t;


/**
 *	The main function of work thread. 
 *
 *	Return NULL always.
 */
extern void *
work_run(void *arg);


/**
 *	Assign client @fd to work thread @index, the @index
 *	is the index of thread in @g_proxy.works, it's not
 *	the thread id. the client's IP is @ip, port is @port.
 *
 *	Return 0 if success, -1 on error.
 */
extern int
work_add(int fd, u_int32_t ip, u_int16_t port, int index);



#endif /* end of FZ_WORK_H  */



