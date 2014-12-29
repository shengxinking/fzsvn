/**
 *	@file	work.c
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

#include "work.h"
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
_work_initiate(thread_t *info)
{
	work_info_t *winfo;
	int maxfd = 0;

	assert(info);

	/* alloc work_info */
	winfo = malloc(sizeof(work_info_t));
	if (!winfo) {
		ERR("malloc memory for work_info error\n");
		return -1;
	}
	memset(winfo, 0, sizeof(work_info_t));

	/* alloc memory for workfd */
	maxfd = (g_proxy.capacity / g_proxy.nworks) + 1;
	winfo->fds = malloc(sizeof(workfd_t) * maxfds);
	if (!winfo->fds) {
		ERR("malloc memory for workfd error\n");
		return -1;
	}

	/* alloc memory for second work fd */
	winfo->fds2 = malloc(sizeof(workfd_t) * maxfds);
	if (!winfo->fds2) {
		ERR("malloc memory for second workfd error\n");
		return -1;
	}
	winfo->nfd = 0;
	winfo->maxfd = maxfd;
	pthread_mutex_init(&winfo->lock, NULL);

	DBG(1, "work(%d) alloc %d workfd\n", info->index, maxfds);
			
	/* alloc memory for epoll event */
	winfo->events = malloc(maxfds * sizeof(struct epoll_event) * 2);
	if (!winfo->events) {
		ERR("malloc memory for epoll event error\n");
		return -1;
	}
	winfo->nevents = maxfds;

	DBG(1, "work(%d) alloc %d epoll event\n", info->index, maxfds * 2);
	
	/* create recv epoll fd */	
	winfo->recvfd = epoll_create(maxfds);
	if (winfo->recvfd < 0) {
		ERR("create recv epoll fd error\n");
		return -1;
	}

	DBG(1, "work(%d) create recv epoll fd %d\n", info->index, winfo->recvfd);

	/* create send epoll fd */
	winfo->sendfd = epoll_create(maxfds);
	if (winfo->sendfd < 0) {
		ERR("create send epoll fd error\n");
		return -1;
	}
	DBG(1, "work(%d) create send epoll fd %d\n", info->index, winfo->sendfd);

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

	if (winfo->recvfd >= 0) {
		close(winfo->epfd);
		DBG(1, "work(%d) close recv fd %d\n", info->index, winfo->recvfd);
	}

	if (winfo->sendfd >= 0) {
		close(winfo->sendfd);
		DBG(1, "work(%d) close send fd %d\n", info->index, winfo->sendfd);
	}

	info->priv = NULL;

	free(winfo);
}


/**
 *	Clear session resources.
 *
 *	No return.
 */
static void 
_work_clear_sesscache(thread_t *info, session_t *s, int issvr)
{
	work_info_t *winfo = NULL;
	sesscache_t *cache = NULL;
	packet_t *pkt = NULL;

	assert(info);
	assert(info->priv);
	assert(s);

	winfo = info->priv;

	if (issvr) {
		cache = &s->svrcache;
	}
	else {
		cache = &s->clicache;
	}

	pkt = pktq_out(&s->clicache.input);
	while (pkt) {
		mempool_put1(pkt);
		s->nallocs--;
		pkt = pktq_out(&s->clicache.input);
	}

	pkt = pktq_out(&s->clicache.output);
	while (pkt) {
		mempool_put1(pkt);
		s->nallocs--;
		pkt = pktq_out(&s->clicache.output);
	}
} 



/**
 *	Close session @s, release all resource used by @s.
 *
 *	No return.
 */
static void 
_work_close_session(thread_t *info, session_t *s)
{
	work_info_t *winfo;
	u_int32_t id;

	assert(info);
	assert(info->priv);
	assert(s);

	winfo = info->priv;
	id = s->id;

	/* clear cached packet */
	_work_clear_sesscache(info, s, 0);

	_work_clear_sesscache(info, s, 1);

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
	sesstbl_del(g_proxy.sesspool, id);
}


/**
 *	Add new session into proxy.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_work_new_session(thread_t *info, workfd_t *wfd)
{
	int fd;
	struct epoll_event event;
	session_t *s = NULL;
	int id;

	assert(info);
	assert(info->priv);
	assert(wfd);

	winfo = info->priv;

	/* get a free id */
	id = sessmap_get_freeid(g_proxy.sessmap, winfo->low, winfo->high, winfo->start);
	if (id < 0) {
		return -1;
	}

	s = malloc(sizeof(session_t));
	if (!s) {
		ERR("malloc memory failed: %s\n", strerror(errno));
		return -1;
	}
	memset(s, 0, sizeof(session_t));
	
	s->magic = SESSION_MAGIC;
	s->clifd = wfd->fd;
	s->id = id;
	s->sip = wfd->ip;
	s->sport = wfd->port;

	/* add client to session pool */
	s = sesstbl_add(winfo->sesstbl, id);
	if (!s) {
		ERR("can't add session to sesspool\n");
		sessmap_free_id(g_proxy.sessmap, id);
		free(s);
		return -1;
	}
	
	/* add to epoll socket */
	event.data.fd = wfd->fd;
	event.events = EPOLLIN;
	if (epoll_ctl(winfo->recvfd, EPOLL_CTL_ADD, wfd->fd, &event)) {
		ERR("epoll add %d error: %s\n", wfd->fd, strerror(errno));
		sessmap_free_id(g_proxy.sessmap, id);
		sesstbl_del(g_proxy.sesstbl, id);
		free(s);
		return -1;
	}
	
	DBG(1, "work(%d) id %u add clifd %d\n", info->index, s->id, fd);

	return 0;
}


/**
 *	Get workfd from @winfo->fds, we use the second workfd @winfo->fds2 and
 *	copy all workfd from @winfo->fds to @winfo->fds, this avoid long time
 *	lock.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_work_get_session(thread_t *info)
{
	work_info_t *winfo;
	int nfd = 0;
	int i = 0;
	workfd_t *wfd;

	assert(info);
	assert(info->priv);

	winfo = info->priv;

	/* get work fd */
	pthread_mutex_lock(&winfo->lock);

	if (winfo->nfds > 0) {
		nfd = winfo->nfd;
		memmove(winfo->fds2, winfo->fds, nfds * sizeof(workfd_t));
		winfo->nfd = 0;
	}

	pthread_mutex_unlock(&winfo->lock);

	/* add client to session pool and epoll socket */
	for (i = 0; i < nfds; i++) {
		wfd = &winfo->fds2[i];
		if (_work_new_session(info, wfd)) {
			close(wfd->fd);
		}
	}

	return 0;
}


/**
 *	Recv data from socket @fd, if @ssl != NULL, using ssl recv.
 *
 *	Return the >0 bytes, -1 on error.
 */
static int 
_work_recv_fin(int fd, SSL *ssl, char *buf, size_t len, int *closed)
{
	int n;

	assert(fd >= 0);
	assert(buf);
	assert(len > 0);
	assert(close);

	if (ssl) {
		return 0;
	}
	else {
		return sock_recv(fd, buf, len, closed);
	}
}


/**
 *	Recv client data.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_work_recv_data(thread_t *info, session_t *s, int issvr)
{
	work_info_t *winfo;
	int fd, n, len;
	SSL *ssl;
	sesscache_t *cache;
	packet_t *pkt;
	char *side = NULL;
	int errflag = 0;
	int closeflag = 0;
	int closed = 0;

	assert(info);
	assert(info->priv);

	winfo = info->priv;

	if (issvr) {
		cache = s->svrcache;
		fd = s->svrfd;
		ssl = s->svrssl;
		side = "server";
		errflag = SESSION_SVRERROR;
		closeflag = SESSION_SVRCLOSE;
	}
	else {
		cache = s->clicache;
		fd = s->clifd;
		ssl = s->clissl;
		side = "client";
		errflag = SESSION_CLIERROR;
		closeflag = SESSION_CLICLOSE;
	}
	
	/* get packet from memory pool */
	pkt = mempool_get(g_proxy.pktpool);
	if (!pkt) {
		ERR("can't get memory from memory pool\n");
		return -1;
	}
	pkt->len = 0;
	pkt->start = 0;
	pkt->capacity = PKT_SIZE - sizeof(packet_t);
	cache->pkt = pkt;
	
	DBG(2, "work %d %s %d get a packet\n", info->index, side, fd);

	/* recv data */
	len = pkt->capacity - pkt->len;
	n = _work_recv_fin(fd, pkt->data, len, &n) < 0) {
		s->flags |= errflag;
		ERR("recv %s %d error\n", side, fd);
		return -1;
	}

	/* client close socket */
	if (n == 0) {
		s->flags |= closeflag;
		DBG(2, "work %d %s %d closed\n", info->index, side, fd);
	}

	pkt->len += n;
	if (pkt->len == 0) {
		mempool_put1(pkt);
		DBG(2, "work %d %s %d free a packet\n", info->index, side, fd);
	}

	return 0;
}


/**
 *	Parse client data
 *
 *	Return 0 if success, -1 on error.
 */
static int
_work_parse_client(thread_t *info, session_t *s)
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
		pktq_in(&s->clicache.output, pkt);

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
_work_send_client(thread_t *info, session_t *s)
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


/**
 *	Process the data recved by proxy, if @issvr !=0, process
 *	server->client data, or else process client->server data.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_work_process_data(thread_t *info, session_t *s, int issvr)
{
	work_info_t *winfo;

	assert(info);
	assert(info->priv);

	winfo = info->priv;

	if (_work_recv_data(info, s, issvr)) {
		_work_epoll_close_session(info, s);
		return -1;
	}

	if (_work_parse_data(info, s, issvr)) {
		_work_epoll_close_session(info, s);
		return -1;
	}

	if (_work_send_data(info, s, issvr)) {
		_work_epoll_close_session(info, s);
	}

	return 0;
}


static int 
_work_send_data(thread_t *info)
{
	session_t *s;
	work_info_t *winfo;
	int n = 0;
	int *pair;
	int id;
	int fd;

	assert(info);
	assert(info->priv);

	winfo = info->priv;

	n = epoll_wait(winfo->sendfd, winfo->events, winfo->nevents, 100);
	if (n < 0) {
		if (errno == EINTR || errno == EAGAIN)
			return 0;;
		
		ERR("send epoll wait error: %s\n", strerror(errno));
		return -1;;
	}
	if (n == 0) {
		sched_yield();
		return 0;
	}
	
	for (i = 0; i < n; i++) {

		/* find session */
		pair = &winfo->events[i].data;
		fd = pair[0];
		id = pair[1];
		
		s = sesstbl_find(g_proxy.sesstbl, id);
		if (!s) {
			ERR("can't find session %d in table\n", id);
			continue;
		}

		/* process session data */
		if (fd == s->clifd) {
			_work_send_client(info, s);
		}
		else {
			_work_send_server(info, s);
		}
	}

	return 0;
}


static int 
_work_recv_data(thread_t *info)
{
	int n = 0;
	session_t *s;
	int *pair;
	int id;
	int fd;

	n = epoll_wait(winfo->epfd, winfo->events, winfo->nevents, 100);
	if (n < 0) {
		if (errno == EINTR || errno == EAGAIN)
			return 0;;
		
		ERR("epoll wait error: %s\n", strerror(errno));
		return -1;;
	}
	if (n == 0) {
		sched_yield();
		return 0;
	}
	
	for (i = 0; i < n; i++) {

		/* find session */
		pair = &winfo->events[i].data;
		fd = pair[0];
		id = pair[1];
		
		s = sesstbl_find(g_proxy.sesstbl, id);
		if (!s) {
			ERR("can't find session %d in table\n", id);
			continue;
		}

		/* process session data */
		if (fd == s->clifd) {
			_work_process_client(info, s);
		}
		else {
			_work_process_server(info, s);
		}			
	}

	return 0;
}


/**
 *	The work thread main loop function.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_work_loop(thread_t *info)
{
	work_info_t *winfo;
	int n, i;
	assert(info);
	assert(info->priv);

	winfo = info->priv;

	while (!g_proxy.stop) {

		_work_new_session(info);

		_work_send_data(info);

		_work_recv_data(info);
	}

	return 0;
}


void *
work_run(void *arg)
{
	thread_t *info;

	info = arg;
	assert(info);

	DBG(1, "work(%d) started\n", info->index);

	if (_work_epoll_initiate(info)) {
		if (g_proxy.main_tid > 0) {
			pthread_kill(g_proxy.main_tid, SIGINT);
		}
		pthread_exit(0);
	}

	_work_loop(info);

	_work_release(info);

	DBG(1, "work thread %d stoped\n", info->index);

	pthread_exit(0);
}


int 
parse_add(int id, parse, int ssl, int index)
{
	work_info_t *winfo;
	int nfd = 0;

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
	winfo->fds[nfd].fd = fd;
	winfo->fds[nfd].ip = ip;
	winfo->fds[nfd].port = port;
	winfo->fds[nfd].ssl = ssl;
	winfo->nfds++;

	/* unlock */
	pthread_mutex_unlock(&winfo->lock);

	return 0;
}




