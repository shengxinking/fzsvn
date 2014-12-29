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

	fe->maxfd = maxfd;
	fe->maxwait = 200;
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
fd_epoll_add_event(fd_epoll_t *fe, int fd, int events, fd_func iocb)
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

	fi->events = events;
	fi->iocb = iocb;

	/* alloc new entry */
	if (fi->is_updated == 0) {
		fe->nupdate++;
		fe->updates[fe->nupdate - 1] = fd;
		fi->is_updated = 1;
	}

	return 0;
}

int 
fd_epoll_flush_events(fd_epoll_t *fe)
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

		if (unlikely(!fi->arg)) {
			continue;
		}

		if (unlikely(!fi->is_updated)) {
			_FE_ERR("fd %d is not add event\n", fd);
			continue;
		}

		if (unlikely(fi->state != FD_READY)) {
			_FE_ERR("fd %d is not ready\n", fd);
			fi->is_updated = 0;
			continue;
		}

		if (unlikely(fi->events == fi->pevents)) {
			fi->is_updated = 0;
			continue;
		}

		if (fi->pevents == 0)
			op = EPOLL_CTL_ADD;
		else if (fi->events == 0)
			op = EPOLL_CTL_DEL;
		else
			op = EPOLL_CTL_MOD;

		memset(&e, 0, sizeof(e));
		e.data.fd = fd;
		e.events = fi->events;

		/* call epoll_ctl() commit change */
		if (unlikely(epoll_ctl(fe->epfd, op, fd, &e))) {
			_FE_ERR("epoll_ctl(%d) on fd %d failed: %s\n", 
				op, fd, _FE_ESTR);
			continue;
		}

		//printf("%p fd %d op %d events %d\n", fi, fd, op, fi->events);

		fi->pevents = fi->events;
		fi->is_updated = 0;
	}

	fe->nupdate = 0;

	return 0;
}

int 
fd_epoll_poll(fd_epoll_t *fe)
{
	int i;
	int fd;
	int nfd;
	int events;
	fd_item_t *fi;

	if (!fe) {
		_FE_ERR("invalid argument\n");
		return -1;
	}

	nfd = epoll_wait(fe->epfd, fe->events, fe->maxwait, fe->waittime);
	if (nfd < 0) {
		//_FE_ERR("epoll_wait failed: %s\n", _FE_ESTR);
		return -1;
	}
	else if (nfd == 0) {
		return 0;
	}

	for (i = 0; i < nfd; i++) {
		fd = fe->events[i].data.fd;
		events = fe->events[i].events;
		
		if (unlikely(events == 0)) {
			_FE_ERR("%d fd have invalid events\n", fd);
			continue;
		}

		fi = fd_epoll_map(fe, fd);
		assert(fi);

		if (unlikely(fi->state != FD_READY)) {
			_FE_ERR("%p %d is not ready\n", fi, fd);
			continue;
		}

		if (unlikely(!fi->iocb)) {
			_FE_ERR("%p %d didn't have iocb\n", fi, fd);
			exit(0);
			continue;
		}

		if (unlikely(!fi->arg)) {
			_FE_ERR("%p %d didn't have arg\n", fi, fd);
			continue;
		}

		//printf("====%p fd %d call iocb events %d\n", fi, fd, events);
		fi->iocb(fd, events, fi->arg);
		//printf("====%p fd %d call iocb events %d finshed\n", fi, fd, events);
	}

	return 0;
}



