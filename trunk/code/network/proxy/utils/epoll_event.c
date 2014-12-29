/**
 *	@file	epoll_event.c
 *
 *	@brief	epoll_event implement.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2012-08-28
 */

#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/epoll.h>

#include "epoll_event.h"

/* likely()/unlikely() for performance */
#ifndef unlikely
#define unlikely(x)  __builtin_expect(!!(x), 0)
#endif

#ifndef	likely
#define likely(x)    __builtin_expect(!!(x), 1)
#endif

/**
 *	Define debug MACRO to print debug information
 */
#define _EE_DBG	1
#ifdef _EE_DBG
#define _EE_ERR(fmt, args...)	fprintf(stderr, "%s:%d: "fmt,\
					__FILE__, __LINE__, ##args)
#else
#define _EE_ERR(fmt, args...)
#endif

static int 
_ep_event_add_read(ep_ctx_t *ctx, ep_event_t *ev)
{
	struct epoll_event e;
	int cmd;
	
	if (unlikely(ev->is_read)) {
		_EE_ERR("read event already exist\n");
		return -1;
	}

	memset(&e, 0, sizeof(e));
	e.data.ptr = ev;
	if (ev->is_write) {
		cmd = EPOLL_CTL_MOD;
		e.events = EPOLLIN | EPOLLOUT | EPOLLERR;
	}
	else {
		cmd = EPOLL_CTL_ADD;
		e.events = EPOLLIN | EPOLLERR;
	}
	
	if (epoll_ctl(ctx->epfd, cmd, ev->fd, &e)) {
		_EE_ERR("epoll_ctx failed: %s\n", strerror(errno));
		return -1;
	}
	
	ev->is_read = 1;

	if (cmd == EPOLL_CTL_ADD)
		ctx->nfd++;

	return 0;
}

static int 
_ep_event_add_write(ep_ctx_t *ctx, ep_event_t *ev)
{
	struct epoll_event e;
	int cmd;

	if (unlikely(ev->is_write)) {
		_EE_ERR("write event already exist\n");
		return -1;
	}

	memset(&e, 0, sizeof(e));
	e.data.ptr = ev;
	if (ev->is_read) {
		cmd = EPOLL_CTL_MOD;
		e.events = EPOLLIN | EPOLLOUT | EPOLLERR;
	}
	else {
		cmd = EPOLL_CTL_ADD;
		e.events = EPOLLOUT | EPOLLERR;
	}
	
	if (epoll_ctl(ctx->epfd, cmd, ev->fd, &e)) {
		_EE_ERR("epoll_ctx failed: %s\n", strerror(errno));
		return -1;
	}
	
	ev->is_write = 1;

	if (cmd == EPOLL_CTL_ADD)
		ctx->nfd++;

	return 0;
}


static int 
_ep_event_del_read(ep_ctx_t *ctx, ep_event_t *ev)
{
	struct epoll_event e;
	int cmd;

	if (unlikely(!ev->is_read)) {
		_EE_ERR("not read event exist\n");
		return -1;
	}

	memset(&e, 0, sizeof(e));
	e.data.ptr = ev;
	if (ev->is_write) {
		cmd = EPOLL_CTL_MOD;
		e.events = EPOLLOUT | EPOLLERR;
	}
	else {
		cmd = EPOLL_CTL_DEL;
	}
	
	if (epoll_ctl(ctx->epfd, cmd, ev->fd, &e)) {
		_EE_ERR("epoll_ctx failed: %s\n", strerror(errno));
		return -1;
	}

	ev->is_read = 0;

	if (cmd == EPOLL_CTL_DEL)
		ctx->nfd--;

	return 0;
}

static int 
_ep_event_del_write(ep_ctx_t *ctx, ep_event_t *ev)
{
	struct epoll_event e;
	int cmd;

	if (unlikely(!ev->is_write)) {
		_EE_ERR("not write event exist\n");
		return -1;
	}

	memset(&e, 0, sizeof(e));
	e.data.ptr = ev;
	if (ev->is_read) {
		cmd = EPOLL_CTL_MOD;
		e.events = EPOLLIN | EPOLLERR;
	}
	else {
		cmd = EPOLL_CTL_DEL;
	}
	
	if (epoll_ctl(ctx->epfd, cmd, ev->fd, &e)) {
		_EE_ERR("epoll_ctx failed: %s\n", strerror(errno));
		return -1;
	}

	ev->is_write = 0;

	if (cmd == EPOLL_CTL_DEL)
		ctx->nfd--;

	return 0;
}

static int 
_ep_event_del(ep_ctx_t *ctx, ep_event_t *ev)
{
	if (unlikely(!ev->is_read && !ev->is_write)) {
		_EE_ERR("no read/write event exist\n");
		return -1;
	}

	if (epoll_ctl(ctx->epfd, EPOLL_CTL_DEL, ev->fd, NULL)) {
		_EE_ERR("epoll_ctl failed: %s\n", strerror(errno));
		return -1;
	}
	
	ev->is_read = 0;
	ev->is_write = 0;

	ctx->nfd--;

	return 0;
}

ep_ctx_t *
ep_event_create(int max)
{
	ep_ctx_t *ctx = NULL;

	if (unlikely(max < 1)) {
		_EE_ERR("invalid argument\n");
		return ctx;
	}

	/* alloc memory for @ctx */
	ctx = malloc(sizeof(*ctx));
	if (!ctx) {
		_EE_ERR("malloc failed: %s\n", strerror(errno));
		return ctx;
	}
	memset(ctx, 0, sizeof(*ctx));

	/* create epoll fd */
	ctx->epfd = epoll_create(max);
	if (ctx->epfd < 0) {
		_EE_ERR("epoll_create failed: %s\n", strerror(errno));
		free(ctx);
		return NULL;
	}

	/* alloc memory for struct epoll_event */
	ctx->events = malloc(max * sizeof(struct epoll_event));
	if (!ctx->events) {
		_EE_ERR("malloc memory for event failed\n");
		close(ctx->epfd);
		free(ctx);
		return NULL;
	}
	ctx->nevent = max;

	return ctx;
}

int 
ep_event_destroy(ep_ctx_t *ctx)
{
	if (unlikely(!ctx)) {
		_EE_ERR("invalid argument\n");
		return -1;
	}
	
	if (ctx->nfd > 0)
		_EE_ERR("have %d socket fd in epoll not deleted\n", ctx->nfd);

	if (ctx->epfd >= 0)
		close(ctx->epfd);

	free(ctx);

	return 0;
}

int 
ep_event_add_read(ep_ctx_t *ctx, ep_event_t *ev)
{
	if (unlikely(!ctx || !ev)) {
		_EE_ERR("invalid argument\n");
		return -1;
	}

	if (unlikely(!ev->read_func)) {
		_EE_ERR("not read function\n");
		return -1;
	}

	return _ep_event_add_read(ctx, ev);
}

int 
ep_event_add_write(ep_ctx_t *ctx, ep_event_t *ev)
{
	if (unlikely(!ctx || !ev)) {
		_EE_ERR("invalid argument\n");
		return -1;
	}

	if (unlikely(!ev->write_func)) {
		_EE_ERR("not write function\n");
		return -1;
	}

	return _ep_event_add_write(ctx, ev);
}

int 
ep_event_del_read(ep_ctx_t *ctx, ep_event_t *ev)
{
	if (unlikely(!ctx || !ev)) {
		_EE_ERR("invalid argument\n");
		return -1;
	}

	return _ep_event_del_read(ctx, ev);
}

int 
ep_event_del_write(ep_ctx_t *ctx, ep_event_t *ev)
{
	if (unlikely(!ctx || !ev)) {
		_EE_ERR("invalid argument\n");
		return -1;
	}

	return _ep_event_del_write(ctx, ev);
}

int 
ep_event_del(ep_ctx_t *ctx, ep_event_t *ev)
{
	if (unlikely(!ctx || !ev)) {
		_EE_ERR("invalid argument\n");
		return -1;
	}

	return _ep_event_del(ctx, ev);
}

int 
ep_event_loop(ep_ctx_t *ctx, int timeout)
{
	int n;
	int i;
	ep_event_t *ev;
	struct epoll_event *e;

	if (unlikely(!ctx)) {
		_EE_ERR("invalid argument\n");
		return -1;
	}

	n = epoll_wait(ctx->epfd, ctx->events, ctx->nevent, timeout);
	if (n < 0) {
		if (errno == EAGAIN || errno == EINTR) {
			return 0;
		}
		_EE_ERR("epoll_wait failed: %s\n", strerror(errno));
		return -1;
	}

	for (i = 0; i < n; i++) {
	
		e = &ctx->events[i];
		ev = e->data.ptr;
		if (!ev) {
			_EE_ERR("no event in epoll\n");
			continue;
		}

		/* error occur */
		if (e->events & EPOLLERR && ev->error_func) {
			ev->error_func(ev);			
			ctx->stat.nerror++;
			continue;
		}

		/* write event */
		if ((e->events & EPOLLOUT && ev->write_func)) {
			ev->write_func(ev);
			ctx->stat.nwrite++;
		}
		
		/* read event */
		if ((e->events & EPOLLIN) && ev->read_func) {
			ev->read_func(ev);
			ctx->stat.nread++;
		}
	}

	return n;
}
