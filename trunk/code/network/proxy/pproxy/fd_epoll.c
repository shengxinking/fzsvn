/**
 *	@file	fd_epoll.c
 *
 *	@brief	the socket fd and epoll function implement.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2014-02-24.
 */

#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "gcc_common.h"
#include "fd_epoll.h"
#include "session.h"
#include "time_util.h"

/**
 *      Define some MACRO for print error message
 */
#define _FE_DEBUG

#ifdef _FE_DEBUG
#define _FE_ERR(f, a...)        fprintf(stderr, "%s:%d: "f,     \
                                        __FILE__, __LINE__, ##a)
#define	_FE_ESTR		strerror(errno)
#else
#define _FE_ERR(f, a...)
#endif

fd_epoll_t * 
fd_epoll_alloc(int maxfd, int waittime)
{
	fd_epoll_t *fe;

	if (maxfd < 1 || waittime < 0) {
		_FE_ERR("invalid argument\n");
		return NULL;
	}
	
	fe = calloc(1, sizeof(fd_epoll_t));
	if (!fe) {
		_FE_ERR("alloc memory for fd_epoll failed: %s\n", _FE_ESTR);
		return NULL;
	}
	
	fe->epfd = epoll_create(maxfd);
	if (fe->epfd < 0) {
		_FE_ERR("epoll_create %d failed: %s\n", maxfd, _FE_ESTR);
		fd_epoll_free(fe);
		return NULL;
	}

	fe->events = calloc(maxfd, sizeof(struct epoll_event));
	if (!fe->events) {
		_FE_ERR("calloc memory for map failed: %s\n", _FE_ESTR);
		fd_epoll_free(fe);
		return NULL;
	}

	fe->maps = calloc(maxfd, sizeof(fd_item_t));
	if (!fe->maps) {
		_FE_ERR("calloc memory for maps failed: %s\n", _FE_ESTR);
		fd_epoll_free(fe);
		return NULL;
	}

	fe->updates = calloc(maxfd, sizeof(int));
	if (!fe->updates) {
		_FE_ERR("calloc memory for updates failed: %s\n", _FE_ESTR);
		fd_epoll_free(fe);
		return NULL;
	}

	fe->tasks = calloc(maxfd, sizeof(int));
	if (!fe->tasks) {
		_FE_ERR("calloc memory for tasks failed: %s\n", _FE_ESTR);
		fd_epoll_free(fe);
		return NULL;
	}
	
	fe->tasks1 = calloc(maxfd, sizeof(int));
	if (!fe->tasks1) {
		_FE_ERR("calloc memory for tasks1 failed: %s\n", _FE_ESTR);
		fd_epoll_free(fe);
		return NULL;
	}
	fe->maxfd = maxfd;
	fe->maxwait = 100;
	fe->waittime = waittime;

	return fe;
}

int 
fd_epoll_free(fd_epoll_t *fe)
{
	if (!fe) {
		_FE_ERR("invalid argument\n");
		return -1;
	}

	if (fe->tasks1) {
		free(fe->tasks1);
	}

	if (fe->tasks) {
		free(fe->tasks);
	}

	if (fe->updates) {
		free(fe->updates);
	}

	if (fe->maps) {
		free(fe->maps);
	}

	if (fe->events) {
		free(fe->events);
	}

	if (fe->epfd > 0) {
		close(fe->epfd);
	}

	free(fe);

	return 0;
}

int 
fd_epoll_alloc_update(fd_epoll_t *fe, int fd, int e, fd_epoll_iocb cb)
{
	fd_item_t *fi;

	if (unlikely(!fe || fd < 0)) {
		_FE_ERR("invalid argument\n");
		return -1;
	}

	if (unlikely(fd >= fe->maxfd)) {
		_FE_ERR("the socket fd %d execeed maxfd %d\n", fd, fe->maxfd);
		return -1;
	}

        fi = &fe->maps[fd];
	if (unlikely(!fi)) {
		_FE_ERR("the socket fd %d is failed\n", fd);
		return -1;
	}

	fi->events = e;
	fi->iocb = cb;

	if (unlikely(fi->is_updated)) {
		_FE_ERR("the socket fd %d already in update\n", fd);
		return -1;
	}

	/* alloc new entry */
	fe->nupdate++;
	fe->updates[fe->nupdate - 1] = fd;
	fi->is_updated = 1;

	//printf("%d add update %d\n", fd, e);

	return 0;
}

int 
fd_epoll_flush_update(fd_epoll_t *fe)
{
	int i;
	int fd;
	int op;
	fd_item_t *fi;
	struct epoll_event e;

	if (unlikely(!fe)) {
		_FE_ERR("invalid argument\n");
		return -1;
	}

	if (fe->nupdate < 1)
		return 0;

	for (i = 0; i < fe->nupdate; i++) {
		fd = fe->updates[i];
		
		if (fd <= 0)
			continue;

		fi = &fe->maps[fd];

		if (unlikely(!fi->arg1)) continue;
		if (unlikely(!fi->is_updated)) continue;
		if (unlikely(fi->state != FD_READY)) continue;
		if (unlikely(fi->events == fi->pevents)) continue;

		if (fi->pevents == 0)
			op = EPOLL_CTL_ADD;
		else if (fi->events == 0)
			op = EPOLL_CTL_DEL;
		else
			op = EPOLL_CTL_MOD;

		e.data.fd = fd;
		e.events = fi->events;

		/* call epoll_ctl() commit change */
		if (unlikely(epoll_ctl(fe->epfd, op, fd, &e))) {
			_FE_ERR("epoll_ctl(%d) on fd %d failed: %s\n", 
				op, fd, _FE_ESTR);
			continue;
		}

		//printf("(%p) fd %d op %d events %d\n", fi, fd, op, fi->events);

		fi->pevents = fi->events;
		fi->is_updated = 0;
	}

	fe->nupdate = 0;

	return 0;
}

int 
fd_epoll_alloc_task(fd_epoll_t *fe, int fd, struct timeval *tm, fd_epoll_iocb cb)
{
	fd_item_t *fi;

	if (unlikely(!fe || fd < 0 || !tm || !cb)) {
		_FE_ERR("invalid argument\n");
		return -1;
	}

	if (unlikely(fd >= fe->maxfd)) {
		_FE_ERR("the socket fd %d execeed maxfd %d\n", fd, fe->maxfd);
		return -1;
	}

        fi = &fe->maps[fd];
	if (unlikely(!fi)) {
		_FE_ERR("the socket fd %d is failed\n", fd);
		return -1;
	}

	if (unlikely(fi->is_tasked)) {
		_FE_ERR("the socket fd %d added into task\n", fd);
		return -1;
	}	

	fe->ntask++;
	fe->tasks[fe->ntask - 1] = fd;
	fi->iocb = cb;
	fi->is_tasked = 1;
	fi->timestamp = *tm;

	//printf("%d add task success, now %d\n", fd, fe->ntask);

	return 0;
}

int 
fd_epoll_flush_task(fd_epoll_t *fe, struct timeval *tm)
{
	int i;
	int fd;
	fd_item_t *fi;
	int *tasks;
	int ntask;
	int ret = 1;

	if (unlikely(!fe || !tm)) {
		_FE_ERR("invalid argument\n");
		return -1;
	}

	if (fe->ntask < 1)
		return 0;

	tasks = fe->tasks;
	ntask = fe->ntask;
	fe->tasks = fe->tasks1;
	fe->tasks1 = tasks;
	fe->ntask = 0;

	for (i = 0; i < ntask; i++) {
		fd = tasks[i];
		
		if (unlikely(fd <= 0)) continue;

		//fi = &fe->maps[fd];
		fi = fd_epoll_map(fe, fd);
		if (unlikely(!fi)) continue;
		if (unlikely(!fi->arg1)) continue;
		if (unlikely(!fi->arg2)) continue;
		if (unlikely(!fi->is_tasked)) continue;
		if (unlikely(!fi->iocb)) continue;

		/* no time out, process task. */
		if (timeval_sub(tm, &fi->timestamp) < 100) {
			//printf("====%p fd %d call task\n", fi, fd);
			ret = fi->iocb(fd, 0, fi->arg1, fi->arg2);
			//printf("====%p fd %d call task finished\n", fi, fd);
		}

		/* task run success or timeout, add it to epoll event to handle later data */
		if (ret > 0) {
			fi->is_tasked = 0;
			fd_epoll_alloc_update(fe, fd, fi->events, fi->iocb);
			continue;
		}
		/* not recv any data, add to task */
		else {
			fe->ntask++;
			fe->tasks[fe->ntask - 1] = fd;
			fi->is_tasked = 1;
		}
	}

	return 0;
}

int 
fd_epoll_poll(fd_epoll_t *fe)
{
	int nfd;
	//int waittime;
	int events;
	int fd;
	fd_item_t *fi;
	int i;

	if (!fe) {
		_FE_ERR("invalid argument\n");
		return -1;
	}

	//waittime = fe->ntask > 0 ? 0 : fe->waittime;

	nfd = epoll_wait(fe->epfd, fe->events, fe->maxfd, fe->waittime);
	if (nfd < 0) {
		//_FE_ERR("epoll_wait failed: %s\n", _FE_ESTR);
		return -1;
	}
	else if (nfd == 0) {
		return 0;
	}

	//printf("total have %d events\n", nfd);

	for (i = 0; i < nfd; i++) {
		fd = fe->events[i].data.fd;
		events = fe->events[i].events;
		
		//printf("fd %d have event %d\n", fd, events);

		if (unlikely(events == 0)) {
			printf("%d fd have invalid events\n", fd);
			continue;
		}

		fi = fd_epoll_map(fe, fd);
		assert(fi);

		if (unlikely(fi->state != FD_READY)) {
			printf("%p %d is not ready\n", fi, fd);
			continue;
		}

		if (unlikely(!fi->iocb)) {
			printf("%p %d didn't have iocb\n", fi, fd);
			continue;
		}

		//printf("====%p fd %d call iocb events %d\n", fi, fd, events);
		fi->iocb(fd, events, fi->arg1, fi->arg2);
		//printf("====%p fd %d call iocb events %d finshed\n", fi, fd, events);
	}

	return 0;
}



