/*
 *	@file	work_epoll.c
 *
 *	@brief	work thread which get client fd from accept thread, 
 *		and recv client request data, parse request data, 
 *		connect to server, send request to server and recv 
 *		reply from server, parse reply, and send reply to 
 *		client. 
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2008-07-31
 */

#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include "work_epoll.h"
#include "debug.h"
#include "thread.h"
#include "proxy.h"
#include "session.h"
#include "sock.h"
#include "sockpool.h"
#include "mempool.h"

/**
 *	Init work thread, alloc resources.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_work_epoll_initiate(thread_t *info)
{
	work_info_t *winfo;
	int maxfds = 0;

	assert(info);

	/* alloc work_info */
	winfo = malloc(sizeof(work_info_t));
	if (!winfo) {
		ERR("malloc memory for work_info error\n");
		return -1;
	}
	memset(winfo, 0, sizeof(work_info_t));

	/* alloc memory for workfd */
	maxfds = (g_proxy.capacity / g_proxy.nworks) + 1;
	winfo->fds = malloc(sizeof(workfd_t) * maxfds);
	if (!winfo->fds) {
		ERR("malloc memory for workfd error\n");
		return -1;
	}
	winfo->nfds = 0;
	winfo->maxfds = maxfds;
	pthread_mutex_init(&winfo->lock, NULL);

	DBG(1, "work(%d) alloc %d workfd\n", info->index, maxfds);
	
	/* alloc memory for second work fd */
	winfo->fds2 = malloc(sizeof(workfd_t) * maxfds);
	if (!winfo->fds2) {
		ERR("malloc memory for second workfd error\n");
		return -1;
	}
	
	DBG(1, "work(%d) alloc %d second workfd\n", info->index, maxfds);

	/* alloc memory for epoll event */
	winfo->events = malloc(maxfds * sizeof(struct epoll_event) * 2);
	if (!winfo->events) {
		ERR("malloc memory for epoll event error\n");
		return -1;
	}
	winfo->nevents = maxfds;

	DBG(1, "work(%d) alloc %d epoll event\n", info->index, maxfds * 2);
	
	/* create epoll fd */
	winfo->epfd = epoll_create(maxfds);
	if (winfo->epfd < 0) {
		ERR("create epoll fd error\n");
		return -1;
	}

	DBG(1, "work(%d) create epoll fd %d\n", info->index, winfo->epfd);

	winfo->sesspool = sesspool_alloc(maxfds, 0);
	if (!winfo->sesspool) {
		ERR("create session pool error\n");
		return -1;
	}
	DBG(1, "work(%d) create session pool %d\n", info->index, maxfds);

	info->priv = winfo;

	return 0;
}


/**
 *	Release work thread resource which alloced by _work_initiate()
 *
 *	No return.
 */
static void 
_work_epoll_release(thread_t *info)
{
	work_info_t *winfo;

	assert(info);

	winfo = info->priv;	
	if (!winfo) {
		return;
	}

	if (winfo->fds) {
		free(winfo->fds);
		winfo->fds = NULL;
		DBG(1, "work(%d) freed workfd\n", info->index);
	}

	if (winfo->fds2) {
		free(winfo->fds2);
		winfo->fds2 = NULL;
		DBG(1, "work(%d) freed second workfd\n", info->index);
	}

	if (winfo->events) {
		free(winfo->events);
		winfo->events = NULL;
		DBG(1, "work(%d) free epoll event\n", info->index);
	}

	if (winfo->epfd >= 0) {
		close(winfo->epfd);
		DBG(1, "work(%d) close epoll fd %d\n", info->index, winfo->epfd);
	}

	if (winfo->sesspool) {
		sesspool_free(winfo->sesspool);
		DBG(1, "work(%d) free session pool\n", info->index);
	}

	info->priv = NULL;
	free(winfo);
}

static void 
_work_epoll_clear_session(thread_t *info, session_t *s)
{
	work_info_t *winfo;
	packet_t *pkt;

	assert(info);
	assert(info->priv);
	assert(s);

	winfo = info->priv;

	pkt = s->clicache.current;
	if (pkt) {
		mempool_put1(pkt);
		s->clicache.current = NULL;
		s->nallocs--;
	}

	pkt = pktqueue_out(&s->clicache.input);
	while (pkt) {
		mempool_put1(pkt);
		s->nallocs--;
		pkt = pktqueue_out(&s->clicache.input);
	}

	pkt = pktqueue_out(&s->clicache.output);
	while (pkt) {
		mempool_put1(pkt);
		s->nallocs--;
		pkt = pktqueue_out(&s->clicache.output);
	}

	pkt = pktqueue_out(&s->clicache.cached);
	while (pkt) {
		mempool_put1(pkt);
		s->nallocs--;
		pkt = pktqueue_out(&s->clicache.cached);
	}

	pkt = s->svrcache.current;
	if (pkt) {
		mempool_put1(pkt);
		s->svrcache.current = NULL;
		s->nallocs--;
	}

	pkt = pktqueue_out(&s->svrcache.input);
	while (pkt) {
		mempool_put1(pkt);
		s->nallocs--;
		pkt = pktqueue_out(&s->svrcache.input);
	}

	pkt = pktqueue_out(&s->svrcache.output);
	while (pkt) {
		mempool_put1(pkt);
		s->nallocs--;
		pkt = pktqueue_out(&s->svrcache.output);
	}

	pkt = pktqueue_out(&s->svrcache.cached);
	while (pkt) {
		mempool_put1(pkt);
		s->nallocs--;
		pkt = pktqueue_out(&s->svrcache.cached);
	}

	assert(s->nallocs == 0);
} 


/**
 *	Close session @s, release all resource used by @s.
 *
 *	No return.
 */
static void 
_work_epoll_close_session(thread_t *info, session_t *s)
{
	work_info_t *winfo;
	u_int32_t id;

	assert(info);
	assert(info->priv);
	assert(s);

	winfo = info->priv;
	id = s->id;

	/* clear cached packet */
	_work_epoll_clear_session(info, s);

	/* close client socket */
	if (s->clifd > -1) {		
		if (epoll_ctl(winfo->epfd, EPOLL_CTL_DEL, s->clifd, NULL)) {
			ERR("remove client %d from epoll %d error: %s\n",
				   s->clifd, winfo->epfd, strerror(errno));
		}
		
		ERR("session %d client %d closed\n", s->id, s->clifd);
		close(s->clifd);
	}

	/* close server socket */
	if (s->svrfd > -1) {
		if (epoll_ctl(winfo->epfd, EPOLL_CTL_DEL, s->svrfd, NULL)) {
			ERR("remove server %d from epoll %d error: %s\n",
				   s->svrfd, winfo->epfd, strerror(errno));
		}
		
		if (s->flags == SESSION_CLICLOSE &&
		    s->clihttp.state == HTTP_STE_FIN &&
		    s->svrhttp.state == HTTP_STE_FIN &&
		    s->svrhttp.code != 201)
			sockpool_put(g_proxy.sockpool, s->svrfd, 0);
		else
			sockpool_put(g_proxy.sockpool, s->svrfd, 1);

		ERR("ssn %d server %d closed\n", s->id, s->svrfd);
	}

	/* delete session from session pool */
	DBG(1, "work(%d) %d deleted\n", id);
	sesspool_del(g_proxy.sesspool, id);
}

/**
 *	Get workfd from @winfo->fds, we use the second workfd @winfo->fds2 and
 *	copy all workfd from @winfo->fds to @winfo->fds, this avoid long time
 *	lock.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_work_epoll_get_session(thread_t *info)
{
	work_info_t *winfo;
	int nfds = 0;
	int i = 0;
	int fd;
	struct epoll_event event;
	session_t *s = NULL;

	assert(info);
	assert(info->priv);

	winfo = info->priv;

	/* get work fd */
	pthread_mutex_lock(&winfo->lock);

	if (winfo->nfds > 0) {
		nfds = winfo->nfds;
		memmove(winfo->fds2, winfo->fds, nfds * sizeof(workfd_t));
		winfo->nfds = 0;
	}

	pthread_mutex_unlock(&winfo->lock);

	/* add client to session pool and epoll socket */
	for (i = 0; i < nfds; i++) {
		fd = winfo->fds2[i].fd;

		/* add client to session pool */
		s = sesspool_add_fd(winfo->sesspool, fd, -1);
		if (!s) {
			ERR("can't add session to sesspool\n");
			close(fd);
			continue;
		}

		/* add to epoll socket */
		event.data.fd = fd;
		event.events = EPOLLIN;
		if (epoll_ctl(winfo->epfd, EPOLL_CTL_ADD, fd, &event)) {
			ERR("epoll add %d error: %s\n", fd, strerror(errno));
			sesspool_del(winfo->sesspool, s->id);
			return -1;
		}
		
		s->sip = winfo->fds2[i].ip;
		s->sport = winfo->fds2[i].port;

		DBG(1, "work(%d) id %u add clifd %d\n", info->index, s->id, fd);
	}

	return 0;
}

/**
 *	Recv client data.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_work_epoll_cli_recv(thread_t *info, session_t *s)
{
	work_info_t *winfo;
	int fd, n, len;
	packet_t *pkt;

	assert(info);
	assert(info->priv);
	assert(s->clifd > -1);

	winfo = info->priv;
	fd = s->clifd;
	pkt = s->clicache.current;

	/* get packet from memory pool */
	if (!pkt) {
		pkt = mempool_get(g_proxy.pktpool);
		if (!pkt) {
			ERR("can't get memory from memory pool\n");
			return -1;
		}
		pkt->len = 0;
		pkt->start = 0;
		pkt->capacity = PKT_SIZE - sizeof(packet_t);
		s->clicache.current = pkt;

		DBG(2, "work %d client %d get a packet\n", info->index, fd);
	}
	else {
		pkt->start = pkt->len;
	}

	/* recv data */
	len = pkt->capacity - pkt->len;
	if (sock_tcprecv(fd, pkt->data + pkt->start, len, &n) < 0) {
		s->flags |= SESSION_CLIERROR;
		ERR("recv client %d error\n", fd);
		return -1;
	}

	/* client close socket */
	if (n == 0) {
		s->flags |= SESSION_CLICLOSE;
		DBG(2, "work %d client %d closed\n", info->index, fd);
	}

	pkt->len += n;
	if (pkt->len == 0) {
		mempool_put1(pkt);
		DBG(2, "work %d client %d free a packet\n", info->index, fd);
	}

	return 0;
}

/**
 *	Parse client data
 *
 *	Return 0 if success, -1 on error.
 */
static int
_work_epoll_cli_parse(thread_t *info, session_t *s)
{
	work_info_t *winfo;
	void *ptr;
	int len;
	packet_t *pkt;

	assert(info);
	assert(info->priv);

	winfo = info->priv;
	pkt = s->clicache.current;
	if (!pkt) {
		if (s->clihttp.state != HTTP_STE_FIN)
			return -1;
		else
			return 0;
	}

	ptr = pkt->data + pkt->start;
	len = pkt->len - pkt->start;
	if (len > 0) {
		if (http_parse_request(&s->clihttp, ptr, len, 0) < 0) {
			ERR("http parse request error\n");
			return -1;
		}
	}
	else {
		if (s->clihttp.state != HTTP_STE_FIN)
			return -1;
	}

	/* put packet to output queue */
	if (s->clihttp.state == HTTP_STE_FIN ||
	    pkt->len == pkt->capacity) {
		s->clicache.current = NULL;
		pktqueue_in(&s->clicache.output, pkt);

		DBG(1, "work(%d) move current packet to output", info->index);
	}

	return 0;
}


/**
 *	Send client data to server
 *
 *	Return 0 if send success, -1 on error.
 */
static int 
_work_epoll_cli_send(thread_t *info, session_t *s)
{
	work_info_t *winfo;
	packet_t *pkt;
	int svrfd;
	struct epoll_event event;

	assert(info);
	assert(info->priv);

	winfo = info->priv;
	svrfd = s->svrfd;

	pkt = pktqueue_out(&s->clicache.output);
	while (pkt) {
		/* get server socket */
		if (svrfd < 0) {

			svrfd = sockpool_get(g_proxy.sockpool, g_proxy.ip, g_proxy.port);
			if (svrfd < 0) {
				ERR("can't get server fd\n");
				return -1;
			}

			/* add to epoll queue */
			memset(&event, 0, sizeof(struct epoll_event));
			event.data.fd = svrfd;
			event.events = EPOLLIN;
			if (epoll_ctl(winfo->epfd, EPOLL_CTL_ADD, svrfd, &event)) {
				ERR("work(%d) add server %d to epoll error\n", info->index, svrfd);
				close(svrfd);
				return -1;
			}

			/* map client socket and server socket */
			if (sesspool_map(winfo->sesspool, s->clifd, s->svrfd)) {
				ERR("map client %d to server %d error\n", s->clifd, svrfd);
				close(svrfd);
				return -1;
			}

		}		

		/* send data */
		if (sock_tcpsend(svrfd, pkt->data, pkt->len)) {
			ERR("work(%d) send data to server %d error\n", info->index, svrfd);			
		}
	}

	winfo = info->priv;

	return 0;
}


static int 
_work_epoll_cli_process(thread_t *info, session_t *s)
{
	work_info_t *winfo;

	assert(info);
	assert(info->priv);

	winfo = info->priv;

	if (_work_epoll_cli_recv(info, s)) {
		_work_close_session(info, s);
		return -1;
	}

	if (_work_epoll_cli_parse(info, s)) {
		_work_close_session(info, s);
		return -1;
	}

	if (_work_epoll_cli_send(info, s)) {
		_work_close_session(info, s);
	}

	return 0;
}

static int 
_work_epoll_svr_recv(thread_t *info, session_t *s)
{
	work_info_t *winfo;

	assert(info);
	assert(info->priv);

	winfo = info->priv;

	

	return 0;
}

static int
_work_epoll_svr_parse(thread_t *info, session_t *s)
{
	work_info_t *winfo;

	assert(info);
	assert(info->priv);

	winfo = info->priv;

	return 0;
}

static int 
_work_epoll_svr_send(thread_t *info, session_t *s)
{
	work_info_t *winfo;

	assert(info);
	assert(info->priv);

	winfo = info->priv;

	return 0;
}


static int 
_work_epoll_svr_process(thread_t *info, session_t *s)
{
	work_info_t *winfo;

	assert(info);
	assert(info->priv);

	winfo = info->priv;

	if (_work_epoll_svr_recv(info, s)) {
		_work_epoll_close_session(info, s);
		return -1;
	}

	if (_work_epoll_svr_parse(info, s)) {
		_work_epoll_close_session(info, s);
		return -1;
	}

	if (_work_epoll_svr_send(info, s)) {
		_work_epoll_close_session(info, s);
	}

	return 0;
}


static int 
_work_epoll_recv(thread_t *info, int fd)
{
	session_t *s;
	work_info_t *winfo;

	assert(info);
	assert(info->priv);

	winfo = info->priv;

	s = sesspool_find(winfo->sesspool, fd);
	if (!s) {
		ERR("can't find fd %d in sesspool\n", fd);
		exit(-1);
	}

	if (fd == s->clifd) {
		_work_epoll_cli_process(info, s);
	}
	else if (fd == s->svrfd) {
		_work_epoll_svr_process(info, s);
	}
	else {
		ERR("fd is not of session fd\n");
		exit(-1);
	}

	return 0;
}

static int 
_work_epoll_loop(thread_t *info)
{
	work_info_t *winfo;
	int n, i;
	assert(info);
	assert(info->priv);

	winfo = info->priv;

	while (!g_proxy.stop) {
		_work_epoll_get_session(info);
		
		n = epoll_wait(winfo->epfd, winfo->events, winfo->nevents, 100);
		if (n < 0) {
			if (errno == EINTR)
				continue;
			if (errno == EAGAIN)
				continue;

			ERR("epoll wait error: %s\n", strerror(errno));
			continue;
		}
		if (n == 0) {
			sched_yield();
			continue;
		}

		for (i = 0; i < n; i++) {
			_work_epoll_recv(info, winfo->events[i].data.fd);
		}
	}

	return 0;
}


/**
 *	The main function of work thread. 
 *
 *	Return NULL always.
 */
void *
work_epoll_run(void *arg)
{
	thread_t *info;

	info = arg;
	if (!info) {		
		ERR("the work thread need argument\n");

		if (g_proxy.main_tid > 0) {
			pthread_kill(g_proxy.main_tid, SIGINT);
		}
		pthread_exit(0);
	}

	DBG(1, "work(%d) started\n", info->index);

	if (_work_epoll_initiate(info)) {
		if (g_proxy.main_tid > 0) {
			pthread_kill(g_proxy.main_tid, SIGINT);
		}
		pthread_exit(0);
	}

	_work_epoll_loop(info);

	_work_epoll_release(info);

	DBG(1, "work thread %d stoped\n", info->index);

	pthread_exit(0);
}

/**
 *	Assign client @fd to work thread @index, the @index
 *	is the index of thread in @g_proxy.works, it's not
 *	the thread id. the client's IP is @ip, port is @port.
 *
 *	Return 0 if success, -1 on error.
 */
int 
work_epoll_add(int fd, u_int32_t ip, u_int16_t port, int index)
{
	work_info_t *winfo;
	int nfds = 0;

	if (index < 0 || index >= g_proxy.nworks)
		return -1;

	winfo = g_proxy.works[index].priv;
	if (!winfo)
		return -1;

	/* locked */
	pthread_mutex_lock(&winfo->lock);

	/* check array parameter */
	if (!winfo->fds || winfo->maxfds < 1 || 
	    winfo->nfds < 0 || winfo->nfds >= winfo->maxfds) 
	{
		pthread_mutex_unlock(&winfo->lock);
		return -1;
	}

	/* put client infomation */
	nfds = winfo->nfds;
	winfo->fds[nfds].fd = fd;
	winfo->fds[nfds].ip = ip;
	winfo->fds[nfds].port = port;
	winfo->nfds++;

	/* unlock */
	pthread_mutex_unlock(&winfo->lock);

	return 0;
}




