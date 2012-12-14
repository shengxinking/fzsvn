/**
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

#define	_GNU_SOURCE

#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <fcntl.h>

#include "debug.h"
#include "thread.h"
#include "proxy.h"
#include "session.h"
#include "sock_util.h"
#include "nb_splice.h"

#define NDEBUG          1

/**
 *	Init work thread, alloc resources.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_work_init(thread_t *info)
{
	work_t *wi;
	int maxfds = 0;

	assert(info);

	/* alloc work_info */
	wi = malloc(sizeof(work_t));
	if (!wi) {
		ERR("malloc error %s\n", ERRSTR);
		return -1;
	}
	memset(wi, 0, sizeof(work_t));

	maxfds = (g_proxy.max / g_proxy.nwork) + 1;

	/* alloc memory for input queue */
	wi->inq = malloc(sizeof(session_t *) * maxfds);
	if (!wi->inq) {
		ERR("malloc error %s\n", ERRSTR);
		return -1;
	}
	memset(wi->inq, 0, sizeof(session_t *) * maxfds);
	
	/* alloc memory for secondary input queue */
	wi->inq2 = malloc(sizeof(session_t *) * maxfds);
	if (!wi->inq2) {
		ERR("malloc error %s\n", ERRSTR);
		return -1;
	}
	memset(wi->inq2, 0, sizeof(session_t *) * maxfds);

	wi->ninq = 0;
	wi->max = maxfds;
	pthread_mutex_init(&wi->lock, NULL);

	DBG("work(%d) alloc %d input queue\n", info->index, maxfds);
	
	/* alloc memory for epoll event */
	wi->events = malloc(maxfds * sizeof(struct epoll_event) * 2);
	if (!wi->events) {
		ERR("malloc error %s\n", ERRSTR);
		return -1;
	}
	wi->nevent = maxfds;

#if 0

	/* alloc memory for packet pool */
	wi->pktpool = objpool_alloc(MAX_PKTLEN, 1000, 0);
	if (!wi->pktpool) {
		ERR("objpool_alloc failed\n");
		return -1;
	}

	DBG("alloc pktpool %p\n", wi->pktpool);

	/* alloc memory for session pool */
	wi->ssnpool = objpool_alloc(sizeof(session_t), 1000, 0);
	if (!wi->ssnpool) {
		ERR("objpool_alloc failed\n");
		return -1;
	}

	DBG("alloc ssnpool %p\n", wi->ssnpool);

#endif

	/* alloc session table */
	wi->sesstbl = session_table_alloc(g_proxy.max);
	if (!wi->sesstbl) {
		ERR("session_table_alloc failed\n");
		return -1;
	}

	DBG("work(%d) alloc %d epoll event\n", info->index, maxfds * 2);
	
	/* create send epoll fd */
	wi->send_epfd = epoll_create(maxfds);
	if (wi->send_epfd < 0) {
		ERR("epoll_create error %s\n", ERRSTR);
		return -1;
	}

	DBG("work(%d) create send epoll fd %d\n", info->index, wi->send_epfd);

	/* create send epoll fd */
	wi->recv_epfd = epoll_create(maxfds);
	if (wi->recv_epfd < 0) {
		ERR("epoll_create error %s\n", ERRSTR);
		return -1;
	}
	
	DBG("work(%d) create recv epoll fd %d\n", info->index, wi->recv_epfd);

	info->priv = wi;

	return 0;
}

/**
 *	Release work thread resource which alloced by _work_init()
 *
 *	No return.
 */
static void 
_work_free(thread_t *info)
{
	work_t *wi;

	assert(info);

	wi = info->priv;	
	if (!wi)
		return;

	if (wi->inq) {
		free(wi->inq);
		wi->inq = NULL;
		DBG("work(%d) freed input queue\n", info->index);
	}

	if (wi->inq2) {
		free(wi->inq2);
		wi->inq2 = NULL;
		DBG("work(%d) freed second input queue\n", info->index);
	}

	if (wi->events) {
		free(wi->events);
		wi->events = NULL;
		DBG("work(%d) free epoll event\n", info->index);
	}

#if 0

	if (wi->pktpool) {
		objpool_free(wi->pktpool);
		wi->pktpool = NULL;
		DBG("work(%d) free pktpool\n");
	}

	if (wi->ssnpool) {
		objpool_free(wi->ssnpool);
		wi->ssnpool = NULL;
		DBG("work(%d) free ssnpool\n");
	}

#endif

	if (wi->sesstbl) {
		session_table_free(wi->sesstbl);
		wi->sesstbl = NULL;
		DBG("work(%d) free session table\n", info->index);
	}

	if (wi->send_epfd >= 0) {
		close(wi->send_epfd);
		DBG("work(%d) close send epoll %d\n", 
		    info->index, wi->send_epfd);
	}

	if (wi->recv_epfd >= 0) {
		close(wi->recv_epfd);
		DBG("work(%d) close recv epoll %d\n", 
		    info->index, wi->recv_epfd);
	}

	info->priv = NULL;
	free(wi);
}

/**
 *	Get a packet from pktpool if need, and put it to @q->pkt.
 *
 *	Return pointer to packet_t if success, NULL on error.
 */
static packet_t *
_work_get_packet(thread_t *info, session_t *s, session_queue_t *q)
{
	work_t *wi;
	packet_t *pkt;

	assert(info);
	assert(info->priv);
	assert(s);
	assert(q);

	wi = info->priv;

	pkt = q->pkt;
	if (pkt)
		return pkt;

	pkt = objpool_get(g_proxy.pktpool);
	if (!pkt) {
		ERR("can't get packet from pkt pool\n");
		return NULL;
	}
	s->nalloced++;
	q->pkt = pkt;
	pkt->len = 0;
	pkt->sendpos = 0;
	pkt->pos = 0;
	pkt->capacity = g_proxy.pktpool->objsize - sizeof(packet_t);

	if (q == &s->cliq) {
		FLOW(2, "work[%d]: ssn[%d] client %d get a packet(%p)\n", 
		     info->index, s->id, s->clifd, pkt);
	}
	else {
		FLOW(2, "work[%d]: ssn[%d] server %d get a packet(%p)\n",
		     info->index, s->id, s->svrfd, pkt);
	}

	return pkt;
}

/**
 *	Select a real server for session @s, use round-robin algo.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_work_get_server(thread_t *info, session_t *s)
{
	work_t *wi;
	struct epoll_event e;
	int fd;
	int wait;
	int *pair;
	static int i = 0;

	assert(info);
	assert(info->priv);
	assert(s);

	wi = info->priv;

	if (s->svrfd > 0)
		return 0;

	proxy_stat_lock();
	fd = sk_tcp_client_nb(&g_proxy.rsaddrs[i], &wait);
	if (fd < 0) {
		proxy_stat_unlock();
		s->is_svrerror = 1;
		ERR("connect server failed\n");
		return -1;
	}

	/* to next real server */
	i = (i + 1) % g_proxy.nrsaddr;
	proxy_stat_unlock();

	/* add it to send epoll to wait connect success */
	if (wait) {
		pair = (int *)&e.data.u64;	
		pair[0] = fd;
		pair[1] = s->id;
		e.events = EPOLLOUT;			
		if (epoll_ctl(wi->send_epfd, EPOLL_CTL_ADD, fd, &e)) {
			s->is_svrerror = 1;
			ERR("epoll add %d error %s\n", fd, ERRSTR);
			close(fd);
			return -1;
		}
		wi->nblocked ++;
		s->is_svrwait = 1;
		FLOW(1, "work[%d]: ssn[%d] client %d connect to server %d wait\n", 
		     info->index, s->id, s->clifd, fd);
	}
	/* add to recv epoll */
	else {
		pair = (int *)&e.data.u64;	
		pair[0] = fd;
		pair[1] = s->id;
		e.events = EPOLLIN;			
		if (epoll_ctl(wi->recv_epfd, EPOLL_CTL_ADD, fd, &e)) {
			ERR("epoll add %d error %s\n", fd, ERRSTR);
			s->is_svrerror = 1;
			close(fd);
			return -1;
		}
		FLOW(1, "work[%d]: ssn[%d] client %d connect to server %d success\n", 
		     info->index, s->id, s->clifd, fd);

	}

	s->svrfd = fd;

	return 0;
}

static session_t * 
_work_alloc_session(thread_t *info)
{
	work_t *wi;
	session_t *s;

	assert(info);
	assert(info->priv);

	wi = info->priv;

	s = objpool_get(g_proxy.ssnpool);
	if (!s) {
		ERR("objpool_get failed\n");
		return NULL;
	}
	memset(s, 0, sizeof(session_t));

	if (session_table_add(wi->sesstbl, s)) {
		ERR("session_table_add failed\n");
		objpool_put(s);
		return NULL;
	}

	return s;
}

/**
 *	Clean session queue @q
 *
 *	No return.
 */
static void 
_work_clean_session(thread_t *info, session_t *s, session_queue_t *q)
{
	packet_t *pkt;
	
	assert(info);
	assert(s);
	assert(q);
	
	pkt = q->pkt;
	if (pkt) {
		s->nalloced--;
		FLOW(2, "work[%d]: 1 ssn[%d] free a packet %p\n", 
		     info->index, s->id, pkt);
		objpool_put(pkt);
	}

	pkt = q->blkpkt;
	if (pkt) {
		s->nalloced--;
		FLOW(2, "work[%d]: 2 ssn[%d] free a packet %p\n", 
		     info->index, s->id, pkt);
		objpool_put(pkt);
	}

	pkt = pktq_out(&q->inq);
	while(pkt) {
		s->nalloced--;
		FLOW(2, "work[%d]: 3 ssn[%d] free a packet %p\n", 
		     info->index, s->id, pkt);
		objpool_put(pkt);
		pkt = pktq_out(&q->inq);
	}

	pkt = pktq_out(&q->outq);
	while (pkt) {
		s->nalloced--;
		FLOW(2, "work[%d]: 4 ssn[%d] free a packet %p\n", 
		     info->index, s->id, pkt);
		objpool_put(pkt);
		pkt = pktq_out(&q->outq);
	}
}

/**
 *	Free session @s, release all resource used by @s.
 *
 *	No return.
 */
static void 
_work_free_session(thread_t *info, session_t *s)
{
	work_t *wi;
	int fd;
	int epfd;

	assert(info);
	assert(info->priv);	
	assert(s);

	wi = info->priv;

	if (g_proxy.use_splice) {
		if (s->read_pipe > 0)
			close(s->read_pipe);
		if (s->write_pipe > 0)
			close(s->write_pipe);
	}

	/* close client socket */
	if (s->clifd > 0) {
		fd = s->clifd;

		epfd = wi->send_epfd;
		if (s->is_cliblock && epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL)) {
			ERR("epoll del %d error %s\n", fd, ERRSTR);
		}
		
		epfd = wi->recv_epfd;
		if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL)) {
			ERR("epoll del %d error %s\n", fd, ERRSTR);
		}
		
		FLOW(1, "work[%d]: ssn[%d] client %d closed\n", 
		     info->index, s->id, fd);
		close(s->clifd);
	}

	/* close server socket */
	if (s->svrfd > 0) {

		fd = s->svrfd;
		epfd = wi->send_epfd;
		if (s->is_svrwait) { 
			if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL))
				ERR("epoll del %d error %s\n", fd, ERRSTR);
		}
		else {
			if (s->is_svrblock && 
			    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL)) 
			{
				ERR("epoll del %d error %s\n", fd, ERRSTR);
			}
			
			epfd = wi->recv_epfd;
			if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL))
				ERR("epoll del %d error %s\n", fd, ERRSTR);

			FLOW(1, "work[%d]: ssn[%d] server %d closed\n", 
			     info->index, s->id, fd);
		}

		close(s->svrfd);
	}

	_work_clean_session(info, s, &s->cliq);

	_work_clean_session(info, s, &s->svrq);

	assert(s->nalloced == 0);

	/* delete session from session pool */
	FLOW(1, "work[%d]: ssn[%d] deleted\n", info->index, s->id);

	session_table_del(wi->sesstbl, s->id);

	objpool_put(s);

	proxy_stat_lock();
	g_proxy.stat.nlive--;
	proxy_stat_unlock();
}

/**
 *	Get session from input queue 
 *
 */
static int 
_work_get_session(thread_t *info)
{
	work_t *wi;
	struct epoll_event e;
	session_t *s;
	workfd_t *wfd;
	int ninq;
	int *pair;
	int pipefd[2];
	int i;
	
	assert(info);
	assert(info->priv);

	wi = info->priv;

	/* get session from input queue */
	pthread_mutex_lock(&wi->lock);
	ninq = wi->ninq;
	if (ninq > 0) {
		memcpy(wi->inq2, wi->inq, ninq * sizeof(workfd_t));
		memset(wi->inq, 0, ninq * sizeof(workfd_t));
		wi->ninq = 0;
	}
	pthread_mutex_unlock(&wi->lock);

	if (ninq < 1)
		return 0;

	for (i = 0; i < ninq; i++) {
	
		wfd = &wi->inq2[i];
		assert(wfd->fd > 0);
		
		s = _work_alloc_session(info);
		if (!s) {
			close(wfd->fd);
			continue;
		}
		
		memset(&e, 0, sizeof(e));
		pair = (int *)&e.data.u64;
		pair[0] = wfd->fd;
		pair[1] = s->id;
		e.events = EPOLLIN;
		if (epoll_ctl(wi->recv_epfd, EPOLL_CTL_ADD, wfd->fd, &e)) {
			ERR("epoll add %d error %s\n", wfd->fd, ERRSTR);
			close(wfd->fd);
			_work_free_session(info, s);
			continue;
		}
		
		s->clifd = wfd->fd;

		if (g_proxy.use_splice || g_proxy.use_nb_splice) {
			if (_work_get_server(info, s)) {
				_work_free_session(info, s);
				continue;
			}
		}
		
		if (g_proxy.use_splice) {
			if (pipe2(pipefd, O_NONBLOCK)) {
				ERR("pipe2 error %s\n", ERRSTR);
				_work_free_session(info, s);
				continue;
			}
			
			s->read_pipe = pipefd[0];
			s->write_pipe = pipefd[1];

			FLOW(1, "work[%d]: ssn[%d] create splice pipe %d:%d\n",
			     info->index, s->id, s->read_pipe, s->write_pipe);
		}

	}

	return 0;
}


static int 
_work_check_server(thread_t *info, session_t *s)
{
	work_t *wi;
	struct epoll_event event;
	int ret = 0;
	int fd;
	int *pair;

	assert(info);
	assert(info->priv);
	assert(s);

	wi = info->priv;
	fd = s->svrfd;

	ret = sk_is_connected(fd);
	if (ret <= 0) {
		FLOW(1, "work[%d]: ssn[%d] server %d connect failed\n", 
		     info->index, s->id, fd);
		return -1;
	}
	s->is_svrwait = 0;
	wi->nblocked --;

	/* remove it from socket */
	if (epoll_ctl(wi->send_epfd, EPOLL_CTL_DEL, fd, NULL)) {
		ERR("epoll del  %d error %s\n", fd, ERRSTR);
		return -1;
	}

	/* add to recv epoll */
	pair = (int *)&event.data.u64;
	pair[0] = fd;
	pair[1] = s->id;
	event.events = EPOLLIN;
	if (epoll_ctl(wi->recv_epfd, EPOLL_CTL_ADD, fd, &event)) {
		ERR("epoll add %d error %s\n", fd, ERRSTR);
		return -1;
	}

	FLOW(1, "work[%d]: ssn[%d] client %d connect to server %d success\n", 
	     info->index, s->id, s->clifd, fd);

	return 0;
}


static int 
_work_recvfrom_client(thread_t *info, session_t *s)
{
	work_t *wi;
	packet_t *pkt;
	int fd;
	int n;
	int blen = 0;
	int close = 0;

	assert(info);
	assert(info->priv);
	assert(s);
	assert(s->clifd >= 0);
	
	wi = info->priv;
	fd = s->clifd;

	/* get packet to restore recved data */
	pkt = _work_get_packet(info, s, &s->cliq);
	if (!pkt)
		return -1;
	
	blen = pkt->capacity - pkt->len;
	assert(blen > 0);
	n = sk_recv(fd, pkt->data, blen, &close);
	if (n < 0) {
		s->is_clierror = 1;
		FLOW(1, "work[%d]: ssn[%d] client %d recv error\n", 
		     info->index, s->id, fd);
		return -1;
	}
	
	pkt->len += n;

	FLOW(1, "work[%d]: ssn[%d] client %d recv %d bytes\n", 
	     info->index, s->id, fd, n);

	if (pkt->len == 0) {
		objpool_put(pkt);
		s->cliq.pkt = NULL;
		s->nalloced--;

		FLOW(2, "work[%d]: ssn[%d] client %d free packet %p\n", 
		     info->index, s->id, fd, pkt);
	}

	if (close) {
		s->is_cliclose = 1;
		FLOW(1, "work[%d]: ssn[%d] client %d recv closed\n", 
		     info->index, s->id, fd);
	}

	return 0;
}

static int 
_work_recvfrom_server(thread_t *info, session_t *s)
{
	work_t *wi;
	packet_t *pkt;
	int n;
	int blen = 0;
	char *ptr;
	int fd;
	int close = 0;

	assert(info);
	assert(info->priv);
	assert(s);
	assert(s->svrfd >= 0);
	
	fd = s->svrfd;
	wi = info->priv;

	pkt = _work_get_packet(info, s, &s->svrq);
	if (unlikely(!pkt)) {
		return -1;
	}

	/* recv data */
	ptr = pkt->data + pkt->recvpos;
	blen = pkt->capacity - pkt->recvpos;
	assert(blen > 0);
	n = sk_recv(fd, ptr, blen, &close);
	if (n < 0) {
		s->is_svrerror = 1;
		FLOW(1, "work[%d]: ssn[%d] server %d recv error\n", 
		     info->index, s->id, fd);
		return -1;
	}
	
	pkt->len += n;
	if (pkt->len == 0) {
		objpool_put(pkt);
		s->svrq.pkt = NULL;
		s->nalloced--;
		FLOW(2, "work[%d] ssn[%d] server %d free packet %p\n", 
		     info->index, s->id, fd, pkt);
	}

	if (close) {
		s->is_svrclose = 1;
		FLOW(1, "work[%d]: ssn[%d] server %d closed\n", 
		     info->index, s->id, fd);
	}

	FLOW(1, "work[%d]: ssn[%d] server %d recv %d bytes\n", 
	     info->index, s->id, fd, n);

	return 0;
}


/**
 *	Parse client data
 *
 *	Return 0 if parse success, -1 on error.
 */
static int 
_work_parse_client(thread_t *info, session_t *s)
{
	packet_t *pkt;
//	int len;
//	void *ptr;

	assert(info);
	assert(info->priv);
	assert(s);

	/* client is closed */
	if (!s->cliq.pkt)
		return 0;

	pkt = s->cliq.pkt;
//	ptr = pkt->data + pkt->recvpos;
//	len = pkt->len - pkt->recvpos;

#if 0
	/* do HTTP parse */
	assert(len >= 0);
	if (len > 0) {
		if (http_parse_request(&s->clihttp, ptr, len, 0)) {
			ERR("work[%d]: http request parse error\n",
			    info->index);
			return -1;
		}
	}
	/* client is closed */
	else {
		if (s->clihttp.state != HTTP_STE_FIN) {
			ERR("work[%d] http request not finish before closed\n",
			    info->index);
			return -1;
		}
	}

	FLOW(1, "work[%d]: ssn[%d] client %d http parse success\n", 
	     info->index, s->id, s->clifd);
	
	/* move packet to output queue */
	if (pkt->len == pkt->capacity ||
	    s->clihttp.state == HTTP_STE_FIN) 
	{
		s->clicache.pkt = NULL;
		pktqueue_in(&s->clicache.output, pkt);

		FLOW(1, "work[%d] ssn[%d] client %d move packet %p to output\n", 
		     info->index, s->id, s->clifd, pkt);
	}
#endif 
	s->cliq.pkt = NULL;
	pktq_in(&s->cliq.outq, pkt);

	FLOW(1, "work[%d]: ssn[%d] client %d move packet %p to output\n", 
	     info->index, s->id, s->clifd, pkt);

	return 0;
}

/**
 *	Parse server data.
 *
 *	Return 0 if success, -1 on error.
 */
static int
_work_parse_server(thread_t *info, session_t *s)
{
	packet_t *pkt;
//	int len;
//	void *ptr;

	assert(info);
	assert(info->priv);
	assert(s);
	
	/* server is closed */
	if (!s->svrq.pkt)
		return 0;

	pkt = s->svrq.pkt;
//	ptr = pkt->data + pkt->recvpos;
//	len = pkt->len - pkt->recvpos;

#if 0
	if (len > 0) {
		if (http_parse_response(&s->svrhttp, ptr, len, 0)) {
			s->flags |= SESSION_SVRERROR;
			ERR("http response parse error\n");
			return -1;
		}
	}
	/* server is closed */
	else {
		if (s->svrhttp.state != HTTP_STE_FIN) {
			ERR("http response not fin before closed\n");
			return -1;
		}
	}
	
	FLOW(1, "work[%d] ssn %d server %d http parse success\n", 
	     info->index, s->id, s->svrfd);
	
	/* move packet to output queue for send */
	if (pkt->len == pkt->capacity ||
	    s->svrhttp.state == HTTP_STE_FIN) 
	{
		s->svrcache.current = NULL;
		pktqueue_in(&s->svrcache.output, pkt);

		FLOW(1, "work[%d] ssn %d server %d move packet %p to output\n", 
		     info->index, s->id, s->svrfd);	
	}

#endif
	s->svrq.pkt = NULL;
	pktq_in(&s->svrq.outq, pkt);

	FLOW(1, "work[%d]: ssn[%d] server %d move packet %p to output\n", 
	     info->index, s->id, s->svrfd, pkt);

	return 0;
}



static int 
_work_sendto_client(thread_t *info, session_t *s)
{
	work_t *wi;
	packet_t *pkt;
	struct epoll_event e;
	void *ptr;
	int *pair;
	int len;
	int n;
	int fd;
	int len1;

	assert(info);
	assert(info->priv);
	assert(s);
	assert(s->clifd > -1);

	wi = info->priv;
	fd = s->clifd;

	if (s->svrq.outq.len < 1 && ! s->svrq.blkpkt)
		return 0;

	/* check block packet first */
	if (s->svrq.blkpkt) {
		pkt = s->svrq.blkpkt;
		s->svrq.blkpkt = NULL;
	}
	else {
		pkt = pktq_out(&s->svrq.outq);
	}

	while (pkt) {
		ptr = pkt->data + pkt->sendpos;
		len = pkt->len - pkt->sendpos;

		len1 = len;
//		if (len > 100) len1 = len - 100;

		n = sk_send(fd, ptr, len1);
		if (n < 0) {
			FLOW(1, "work[%d]: ssn[%d] server %d sendto client %d failed\n", 
			     info->index, s->id, s->svrfd, fd);
			s->is_clierror = 1;
			s->nalloced--;
			objpool_put(pkt);
			return -1;
		}
		else if (n < len) {

			pkt->sendpos += n;			
			if (s->is_cliblock)
				break;
			
			/* add it to send epoll and set block */
			memset(&e, 0, sizeof(e));
			e.events = EPOLLOUT;
			pair = (int *)&e.data.u64;
			pair[0] = fd;
			pair[1] = s->id;
			if (epoll_ctl(wi->send_epfd, EPOLL_CTL_ADD, fd, &e)) {
				ERR("epoll add %d error %s\n", fd, ERRSTR);
				objpool_put(pkt);
				s->nalloced--;
				return -1;
			}
			s->svrq.blkpkt = pkt;
			s->is_cliblock = 1;
			wi->nblocked++;
			FLOW(1, "work[%d]: ssn[%d] server %d sendto client %d blocked(%d:%d)\n",
			     info->index, s->id, s->svrfd, fd, len, n);
			break;
		}

		if (s->is_cliblock) {
			if (epoll_ctl(wi->send_epfd, EPOLL_CTL_DEL, fd, NULL)) {
				ERR("epoll del %d error %s\n", fd, ERRSTR);
				objpool_put(pkt);
				s->nalloced--;
				FLOW(2, "work[%d]: ssn[%d] server %d free a packet %p\n", 
				     info->index, s->id, s->svrfd, pkt);
				return -1;
			}
			s->is_cliblock = 0;
			wi->nblocked--;
		}

		FLOW(1, "work[%d]: ssn[%d] server %d sendto client %d %d bytes\n", 
		     info->index, s->id, s->svrfd, s->clifd, len);

		objpool_put(pkt);
		s->nalloced--;

		FLOW(2, "work[%d]: ssn[%d] server %d free packet\n", 
		     info->index, s->id, s->svrfd);
		
		pkt = pktq_out(&s->svrq.outq);
	}
	
	return 0;

}

/**
 *	Send client data to server.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_work_sendto_server(thread_t *info, session_t *s)
{
	work_t *wi;
	packet_t *pkt;
	struct epoll_event event;
	int fd;
	void *ptr;
	int len;
	int n;
	int *pair;
	int len1;

	assert(info);
	assert(info->priv);
	assert(s);

	wi = info->priv;

	if (s->cliq.outq.len < 1 && ! s->cliq.blkpkt)
		return 0;

	if (_work_get_server(info, s)) 
		return -1;

	if (s->is_svrwait) {
		return 0;
	}

	fd = s->svrfd;

	/* send block packet first */
	if (s->cliq.blkpkt) {
		pkt = s->cliq.blkpkt;
		s->cliq.blkpkt = NULL;
	}
	else {
		pkt = pktq_out(&s->cliq.outq);
	}

	while (pkt) {

		/* send packet to server */
		ptr = pkt->data + pkt->sendpos;
		len = pkt->len - pkt->sendpos;
		len1 = len;
//		if (len > 100) len1 = len - 100;
		n = sk_send(fd, ptr, len1);
		/* send error */
		if (n <= 0) {
			s->is_svrerror = 1;
			objpool_put(pkt);
			s->nalloced--;
			FLOW(1, "work[%d]: ssn[%d] client %d sendto server %d error\n", 
			     info->index, s->id, s->clifd, fd);
			return -1;
		}
		/* send not finished, block client */
		else if (n < len) {

			pkt->sendpos = pkt->sendpos + n;			
			if (s->is_svrblock)
				break;

			/* add it to send epoll */
			pair = (int *)&event.data.u64;
			pair[0] = fd;
			pair[1] = s->id;
			event.events = EPOLLOUT;			
			if (epoll_ctl(wi->send_epfd, EPOLL_CTL_ADD, fd, &event)) {
				ERR("epoll add %d error %s\n", fd, ERRSTR);
				s->is_svrerror = 1;
				objpool_put(pkt);
				s->nalloced--;
				FLOW(2, "work[%d]: ssn[%d] client %d free a packet %p\n", 
				     info->index, s->id, s->clifd, pkt);
				
				return -1;
			}
			s->cliq.blkpkt = pkt;
			s->is_svrblock = 1;
			wi->nblocked++;
			FLOW(1, "work[%d]: ssn[%d] client %d sendto server %d %d:%d blocked\n", 
			     info->index, s->id, s->clifd, fd, len, n);
			break;
		}

		FLOW(1, "work[%d]: ssn[%d] client %d sendto server %d %d bytes\n", 
		     info->index, s->id, s->clifd, fd, len);

		/* clear block flags because send success */
		if (s->is_svrblock) {
			if (epoll_ctl(wi->send_epfd, EPOLL_CTL_DEL, fd, NULL)) {
				ERR("epoll del %d error %s\n", fd, ERRSTR);
				s->is_svrerror = 1;
				objpool_put(pkt);
				s->nalloced--;
				return -1;
			}
			s->is_svrblock = 0;
			wi->nblocked--;
		}

		s->nalloced--;
		objpool_put(pkt);

		pkt = pktq_out(&s->cliq.outq);
	}

	return 0;
}

/**
 *	nb_splice client data to server.	
 *
 *	Return nb_splice bytes if success, -1 on error.
 */
static int 
_work_nb_splice_cli2svr(thread_t *info, session_t *s)
{
	int n;

	assert(info);
	assert(s);
	assert(s->clifd > 0);
	assert(s->svrfd > 0);

	if (s->is_svrwait)
		return 0;

	n = nb_splice(g_proxy.nb_splice_fd, s->clifd, s->svrfd, 16384, 0);
	
	FLOW(1, "work[%d] ssn[%d] client %d nb_splice server %d %d bytes\n",
	     info->index, s->id, s->clifd, s->svrfd, n);

	return n;
}

/**
 *	splice(linux) client data to server
 *
 *	Return splice bytes if success, -1 on error.
 */
static int 
_work_splice_cli2svr(thread_t *info, session_t *s)
{
	int n, m;
	
	assert(info);
	assert(info->priv);
	assert(s);
	assert(s->clifd > 0);
	assert(s->svrfd > 0);
	
	if (s->is_svrwait)
		return 0;

	n = splice(s->clifd, NULL, s->write_pipe, NULL, 
		   16384, SPLICE_F_MORE | SPLICE_F_NONBLOCK);
	FLOW(1, "work[%d]: ssn[%d] client %d splice(1) server %d  %d bytes\n", 
	     info->index,s->id, s->clifd, s->svrfd, n);
	if (n <= 0){
		if (n < 0)
			ERR("splice failed: %s\n", strerror(errno));
		return -1;
	}

	m = splice(s->read_pipe, NULL, s->svrfd, NULL, 
		   n, SPLICE_F_MORE | SPLICE_F_NONBLOCK);
	FLOW(1, "work[%d]: ssn[%d] client %d splice(2) server %d %d bytes\n", 
	     info->index, s->id, s->clifd, s->svrfd, m);

	return m;
}

/**
 *	Process client request, include recv client data, parse client data, 
 *	and send client data to server 
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_work_process_client(thread_t *info, session_t *s)
{
	int ret = 0;

	assert(info);
	assert(info->priv);
	assert(s);
	assert(s->clifd >= 0);

	if (g_proxy.use_nb_splice) {
		ret = _work_nb_splice_cli2svr(info, s);
		if (ret < 0) {
			_work_free_session(info, s);		
			return -1;
		}
		return ret;
	}

	if (g_proxy.use_splice) {
		ret = _work_splice_cli2svr(info, s);
		if (ret < 0) {
			_work_free_session(info, s);		
			return -1;
		}
		return ret;
	}

	if (_work_recvfrom_client(info, s)) {
		_work_free_session(info, s);
		return -1;
	}
       
	if (_work_parse_client(info, s)) {
		_work_free_session(info, s);
		return -1;
	}
		
	if (_work_sendto_server(info, s)) {
		_work_free_session(info, s);
		return -1;
	}

	if (s->is_clierror || s->is_cliclose) {
		_work_free_session(info, s);
		return -1;
	}

	return 0;
}

/**
 *	nb_splice server data to client.	
 *
 *	Return splice bytes if success, -1 on error.
 */
static int 
_work_nb_splice_svr2cli(thread_t *info, session_t *s)
{
	int n;

	assert(info);
	assert(s);
	assert(s->clifd > 0);
	assert(s->svrfd > 0);

	n = nb_splice(g_proxy.nb_splice_fd, s->svrfd, s->clifd, 16384, 0);

	FLOW(1, "work[%d]: ssn[%d] client %d nb_splice server %d %d bytes\n", 
	     info->index, s->id, s->svrfd, s->clifd, n);

	return n;
}

/**
 *	Splice server data to client.
 *
 *	Return splice bytes is success, -1 on error.
 */
static int 
_work_splice_svr2cli(thread_t *info, session_t *s)
{
	int n, m;
	
	assert(info);
	assert(s);
	assert(s->clifd > 0);
	assert(s->svrfd > 0);
	
	n = splice(s->svrfd, NULL, s->write_pipe, NULL, 16384, SPLICE_F_NONBLOCK);
	FLOW(1, "work[%d]: ssn[%d] server %d splice(1) client %d %d bytes\n", 
	     info->index, s->id, s->svrfd, s->clifd, n);
	if (n <= 0) {
		if (n < 0)
			ERR("splice failed %s\n", strerror(errno));
		return -1;
	}

	m = splice(s->read_pipe, NULL, s->clifd, NULL, n, SPLICE_F_NONBLOCK);
	FLOW(1, "work[%d]: ssn[%d] server %d splice(2) client %d %d bytes\n", 
	     info->index, s->id, s->svrfd, s->clifd, m);

	return m;
}

/**
 *	Process server data, include recv data data, parse server data, and
 *	send server data to client.
 *
 *	Return 0 if success, -1 on error.
 */
static int
_work_process_server(thread_t *info, session_t *s)
{
	int ret = 0;

	assert(info);
	assert(info->priv);
	assert(s);
	assert(s->svrfd > -1);

	if (g_proxy.use_nb_splice) {
		ret = _work_nb_splice_svr2cli(info, s);
		if (ret < 0) {
			_work_free_session(info, s);
			return -1;
		}
		
		return ret;
	}

	if (g_proxy.use_splice) {
		ret = _work_splice_svr2cli(info, s);
		if (ret < 0) {
			_work_free_session(info, s);
			return -1;
		}
		return ret;
	}

	if (_work_recvfrom_server(info, s)) {
		_work_free_session(info, s);
		return -1;
	}

	if (_work_parse_server(info, s)) {
		_work_free_session(info, s);
		return -1;
	}

	if (_work_sendto_client(info, s)) {
		_work_free_session(info, s);
		return -1;
	}

	if (s->is_svrerror || s->is_svrclose) {
		_work_free_session(info, s);
	}

	return 0;
}


/**
 *	Recv data from socket 
 *
 *	Return 0 if success, -1 on error.
 */
static void 
_work_recv_data(thread_t *info)
{
	work_t *wi;
	session_t *s;
	struct epoll_event *e;
	int *pair;
	int fd;
	int id;
	int nfds;
	int i;

	assert(info);
	assert(info->priv);

	wi = info->priv;

	nfds = epoll_wait(wi->recv_epfd, wi->events, 
			  wi->nevent, WORK_TIMEOUT);
	
	if (nfds < 0) {
		if (errno == EINTR)
			return;
		ERR("epoll_wait error %s", ERRSTR);
		return;
	}

	if (nfds == 0)
		return;

	for (i = 0; i < nfds; i++) {
		
		e = &wi->events[i];

		pair = (int *)&e->data.u64;
		fd = pair[0];
		id = pair[1];
		s = session_table_find(wi->sesstbl, id);
		if (!s) {
			continue;
		}
		
		if (e->events & EPOLLERR) {
			_work_free_session(info, s);
			continue;
		}

		if (s->clifd == fd) {
			if (!s->is_svrblock)
				_work_process_client(info, s);
		}
		else if (s->svrfd == fd) {
			if (!s->is_cliblock)
				_work_process_server(info, s);
		}
		else {
			ERR("the wrong fd %d is in session %d\n", fd, id);
		}
	}

}


static void  
_work_send_data(thread_t *info) 
{
	work_t *wi;
	session_t *s;
	struct epoll_event *e;
	int *pair;
	int fd;
	int id;
	int nfds;
	int i;

	assert(info);
	assert(info->priv);

	wi = info->priv;

	assert(wi->nblocked >= 0);

	if (wi->nblocked < 1)
		return;

	nfds = epoll_wait(wi->send_epfd, wi->events, 
			  wi->nevent, WORK_TIMEOUT);
	
	if (nfds < 0) {
		if (errno == EINTR || errno == EAGAIN)
			return;

		ERR("epoll_wait error: %s", strerror(errno));
		return;
	}

	if (nfds == 0)
		return;

	for (i = 0; i < nfds; i++) {

		e = &wi->events[i];

		/* recv data */
		pair = (int *)&e->data.u64;
		fd = pair[0];
		id = pair[1];
		s = session_table_find(wi->sesstbl, id);
		if (!s) {
			continue;
		}

		if (e->events & EPOLLERR) {
			_work_free_session(info, s);
			continue;
		}

		if (s->clifd == fd) {
			if (_work_sendto_client(info, s))
				_work_free_session(info, s);
		}
		else if (s->svrfd == fd) {
			if (s->is_svrwait && _work_check_server(info, s))
				_work_free_session(info, s);

			if (_work_sendto_server(info, s)) 
				_work_free_session(info, s);
		}
		else {
			ERR("the error fd %d is in session %d", fd, id);
		}
	}
}




static int 
_work_loop(thread_t *info)
{
	while (!g_proxy.stop) {
		_work_get_session(info);
		
		_work_send_data(info);

		_work_recv_data(info);
	}

	return 0;
}

/**
 *	The main function of work thread. 
 *
 *	Return NULL always.
 */
void *
work_run(void *arg)
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

	DBG("work(%d) started\n", info->index);

	if (_work_init(info)) {
		if (g_proxy.main_tid > 0) {
			pthread_kill(g_proxy.main_tid, SIGINT);
		}
		pthread_exit(0);
	}

	_work_loop(info);

	_work_free(info);

	DBG("work(%d) stoped\n", info->index);

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
work_add_session(int index, workfd_t *wfd)
{
	work_t *wi;
	int ninq = 0;

	if (unlikely(index < 0 || index >= g_proxy.nwork))
		return -1;

	if (unlikely(!wfd))
		return -1;

	wi = g_proxy.works[index].priv;
	if (unlikely(!wi))
		return -1;

	/* locked */
	pthread_mutex_lock(&wi->lock);

	/* check array parameter */
	if (!wi->inq || wi->max < 1 || 
	    wi->ninq < 0 || wi->ninq >= wi->max) 
	{
		ERR("the work(%d) inq have problem\n", index);
		pthread_mutex_unlock(&wi->lock);
		return -1;
	}

	/* put client infomation */
	ninq = wi->ninq;
	wi->inq[ninq].fd = wfd->fd;
	wi->inq[ninq].is_ssl = wfd->is_ssl;
	wi->ninq++;

	/* unlock */
	pthread_mutex_unlock(&wi->lock);

	return 0;
}




