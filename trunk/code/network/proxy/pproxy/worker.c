/**
 *	@file	work5.c
 *
 *	@brief	worker which accept client, many policy in one worker.
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
#include "worker.h"
#include "childproc.h"
#include "proxy.h"
#include "task.h"
#include "session.h"
#include "ip_addr.h"
#include "sock_util.h"
#include "gcc_common.h"

#define NDEBUG          1

static int _worker_accept_client(int fd, int events, void *arg1, void *arg2);
static int _worker_connect(session_t *s, session_data_t *sd);
static int _worker_handshake(int fd, int events, void *arg1, void *arg2);
static int _worker_recv_data(int fd, int events, void *arg1, void *arg2);
static int _worker_send_data(int fd, int events, void *arg1, void *arg2);
static int _worker_run_task(task_t *t);


/**
 *	Init worker, alloc resources.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_worker_init(proxy_t *py)
{
	worker_t *wi;
	fd_item_t *fi;
	policy_t *pl;
	policy_fd_t *pfd;
	int i;

	assert(py);

	/* alloc work_info */
	wi = malloc(sizeof(worker_t));
	if (!wi) {
		ERR("malloc error %s\n", ERRSTR);
		return -1;
	}
	memset(wi, 0, sizeof(worker_t));

	/* alloc object pool */
	wi->pktpool = objpool_alloc(4096, 1024, 0);
	if (!wi->pktpool) {
		ERR("objpool_alloc failed\n");
		return -1;
	}
	DBG("work[%d] alloc packet pool(%p)\n", py->index, wi->pktpool);

	/* alloc session pool */
	wi->ssnpool = objpool_alloc(sizeof(session_t), 1024, 0);
	if (!wi->ssnpool) {
		ERR("objpool_alloc failed\n");
		return -1;
	}
	DBG("work[%d] alloc session pool(%p)\n", py->index, wi->ssnpool);

	/* alloc task queue */
	wi->taskq = task_alloc_queue();
	if (!wi->taskq) {
		ERR("alloc task queue failed\n");
		return -1;
	}
	DBG("work[%d] alloc task queue(%p)\n", py->index, wi->taskq);

	wi->fe = fd_epoll_alloc(g_proxy.maxfd, 1);
	if (!wi->fe) {
		ERR("alloc fd_epoll failed\n");
		return -1;
	}
	DBG("work[%d] alloc fd_epoll(%p)\n", py->index, wi->fe);

	for (i = 0; i < g_proxy.npolicy; i++) {
		pl = &g_proxy.policies[i];
		pfd = &wi->pfds[i];
		pfd->proxy = py;
		pfd->worker = wi;
		pfd->policy = pl;

		pfd->httpfd = sk_tcp_server(&pl->httpaddr, 1);
		if (pfd->httpfd < 0) {
			ERR("create http listen fd failed\n");
			return -1;
		}
		sk_set_nonblock(pfd->httpfd, 1);

		/* add listen fd to epoll event */
		fi = fd_epoll_map(wi->fe, pfd->httpfd);
		assert(fi);
		fd_epoll_init_item(fi);
		fi->state = FD_READY;
		fi->arg1 = pfd;
		if (unlikely(fd_epoll_alloc_update(wi->fe, pfd->httpfd, 
					   EPOLLIN, _worker_accept_client)))
		{
			ERR("add httpfd into fd epoll failed\n");
			return -1;
		}

		DBG("work[%d] create httpfd %d\n", py->index, pfd->httpfd);
	}
	wi->npfd = g_proxy.npolicy;

	py->priv = wi;

	return 0;
}

/**
 *	Release worker resource which alloced by _work_init()
 *
 *	No return.
 */
static void 
_worker_free(proxy_t *py)
{
	worker_t *wi;
	int i;

	assert(py);

	wi = py->priv;	
	if (!wi)
		return;

	if (wi->ssnpool)
		objpool_free(wi->ssnpool);
	
	if (wi->pktpool)
		objpool_free(wi->pktpool);

	for (i = 0; i < wi->npfd; i++) {
		if (wi->pfds[i].httpfd > 0)
			close(wi->pfds[i].httpfd);
	}

	if (wi->fe)
		fd_epoll_free(wi->fe);

	py->priv = NULL;
	free(wi);
}

static inline session_t * 
_worker_alloc_session(proxy_t *py, worker_t *wi, policy_t *pl, 
		      int fd, ip_port_t *addr)
{
	session_t *s;

	assert(py);
	assert(pl);
	assert(wi);
	assert(fd > 0);
	assert(addr);

	s = objpool_get(wi->ssnpool);
	if (unlikely(!s)) {
		ERR("objpool_get failed\n");
		return NULL;
	}
	session_init(s);
	s->proxy = py;
	s->worker = wi;
	s->policy = pl;
	s->clidata.fd = fd;
	s->clidata.addr = *(addr);

	return s;
}

static void 
_worker_free_session_data(proxy_t *py, worker_t *wi, policy_t *pl, 
			  session_t *s, session_data_t *sd)
{
	packet_t *pkt, *bak;
	fd_item_t *fi;

	assert(py);
	assert(wi);
	assert(pl);
	assert(s);
	assert(sd);

	/* free input queue */
	CBLIST_FOR_EACH_SAFE(&sd->in, pkt, bak, list) {
		CBLIST_DEL(&pkt->list);
		objpool_put(pkt);
		s->nalloced--;
		WFLOW(2, "%s %d free (in)packet %p, nalloced %d\n", 
		      sd->side, sd->fd, pkt, s->nalloced);
	}

	/* free output queue */
	CBLIST_FOR_EACH_SAFE(&sd->out, pkt, bak, list) {
		CBLIST_DEL(&pkt->list);
		objpool_put(pkt);
		s->nalloced--;
		WFLOW(2, "%s %d free (out)packet %p, nalloced %d\n", 
		      sd->side, sd->fd, pkt, s->nalloced);
	}

	/* close socket fd, not need update event */
	if (sd->fd > 0) {
		fi = fd_epoll_map(wi->fe, sd->fd);
                assert(fi);
                fd_epoll_init_item(fi);
		close(sd->fd);
		WFLOW(1, "%s %d closed\n", sd->side, sd->fd);
		sd->fd = -1;
	}
}

/**
 *	Free session @s, release all resource used by @s.
 *
 *	No return.
 */
static void 
_worker_free_session(session_t *s, session_data_t *sd)
{	
	proxy_t *py;
	worker_t *wi;
	policy_t *pl; 

	assert(s);
	assert(sd);
	py = s->proxy;
	wi = s->worker;
	pl = s->policy;

	assert(py);
	assert(wi);
	assert(pl);

	/* free session data */
	_worker_free_session_data(py, wi, pl, s, sd);

	/* peer already in task queue, delay to peer delete */
	if (task_in_queue(&sd->peer->task)) {
		sd->peer->task.task = TASK_DELETE;
		WFLOW(1, "%s %d task in queue, delete later\n", 
		      sd->peer->side, sd->peer->fd);
		return;
	}
	else {
		_worker_free_session_data(py, wi, pl, s, sd->peer);
	}

	assert(s->nalloced == 0);

	/* delete session from session pool */
	WFLOW(1, "deleted\n");

	objpool_put(s);

	g_proxy.stat.nhttplive--;
	g_proxy.stat.nlive--;
}

/**
 *	Accept new client and add to session table.
 *
 */
static int 
_worker_accept_client(int fd, int events, void *arg1, void *arg2)
{
	policy_t *pl;
	policy_fd_t *pfd;
	proxy_t *py;
	worker_t *wi;
	session_t *s;
	fd_item_t *fi;
	int clifd;
	ip_port_t cliaddr;
	u_int64_t nlive;
	int i;
	char ipstr1[IP_STR_LEN];
	char ipstr2[IP_STR_LEN];

	assert(fd > 0);
	assert(arg1);
	assert(arg2 == NULL);

	pfd = (policy_fd_t *)arg1;		
	py = pfd->proxy;
	wi = pfd->worker;
	pl = pfd->policy;

	assert(py);
	assert(wi);
	assert(pl);

	assert(fd = pfd->httpfd);

	if (unlikely(events & EPOLLERR))
		return -1;

	for (i = 0; i < 8; i++) {
		
		/* accept client */
		clifd = sk_tcp_accept(fd, &cliaddr);
		if (unlikely(clifd < 0)) {
			if (errno != EAGAIN) {
				ERR("accept %d failed: %s\n", fd, ERRSTR);
				return -1;
			}
			else {
				return 0;
			}
		}

		PROXY_LOCK();
		g_proxy.stat.naccept++;
		nlive = g_proxy.stat.nlive;
		PROXY_UNLOCK();

		/* check exceed max connection */
		if (unlikely(nlive >= (u_int32_t)g_proxy.max)) {
			close(clifd);
			ERR("work[%d] too many client %lu max %d\n", 
			    py->index, nlive, g_proxy.max);
			return -1;
		}

		wi->next_sid++;
		
		sk_set_nonblock(clifd, 1);
		sk_set_nodelay(clifd, 1);
		sk_set_quickack(clifd, 1);


		/* alloc new session */
		s = _worker_alloc_session(py, wi, pl, clifd, &cliaddr);
		if (unlikely(!s)) {
			ERR("alloc new session failed\n");
			close(clifd);
			return -1;
		}
		WFLOW(1, "client %d accepted(%s->%s)\n", clifd,
		      ip_port_to_str(&cliaddr, ipstr1, IP_STR_LEN),
		      ip_port_to_str(&pl->httpaddr, ipstr2, IP_STR_LEN));

		/* set task callback function */
		s->clidata.task.cb = _worker_run_task;
		s->svrdata.task.cb = _worker_run_task;
                
		/* map fd to fd_item, and init it */
                fi = fd_epoll_map(wi->fe, clifd);
                assert(fi);
		//fd_epoll_init_item(fi);
		fi->arg1 = s;
                fi->arg2 = &s->clidata;
		fi->state = FD_READY;
	
		/* add it to epoll event update for recv data */
		if (unlikely(fd_epoll_alloc_update(wi->fe, clifd, 
						   EPOLLIN|EPOLLRDHUP,
						   _worker_recv_data)))
		{
			ERR("alloc update failed\n");
			_worker_free_session(s, &s->clidata);
			return -1;
		}
		WFLOW(3, "client %d update alloc read|closed\n", clifd);

		/* connect to client first */
		if (unlikely(_worker_connect(s, &s->svrdata))) {
			ERR("connect to server failed\n");
			_worker_free_session(s, &s->clidata);
			return -1;
		}

		/* update proxy statistic */
		PROXY_LOCK();
		g_proxy.stat.nhttp++;
		g_proxy.stat.nhttplive++;
		g_proxy.stat.nlive++;
		PROXY_UNLOCK();
	}

	return 0;
}

/**
 *	Select a real server for session @s, use round-robin algo.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_worker_connect(session_t *s, session_data_t *sd)
{	
	policy_t *pl;
	proxy_t *py;
	worker_t *wi;
	static int i = 0;
	int fd;
	int wait = 0;
	int index;
	fd_item_t *fi;

	assert(s);
	assert(sd);

	py = s->proxy;
	wi = s->worker;
	pl = s->policy;

	assert(py);
	assert(wi);
	assert(pl);

	PROXY_LOCK();
	index = i;
	i = (i + 1) % pl->nrsaddr;
	PROXY_UNLOCK();

	fd = sk_tcp_client_nb(&pl->rsaddrs[index], &wait);
	sd->fd = fd;
	if (unlikely(fd < 0)) {
		ERR("%s connect failed\n", sd->side);
		return -1;
	}

	/* init fd_item */
	fi = fd_epoll_map(wi->fe, fd);
	assert(fi);
	//fd_epoll_init_item(fi);
	fi->state = FD_READY;
	fi->arg1 = s;
	fi->arg2 = sd;

	/* add to epoll if connect success */
	if (unlikely(wait == 0)) {
		sk_set_nodelay(fd, 1);
		sk_set_quickack(fd, 1);

		WFLOW(1, "%s %d connect success\n", sd->side, fd);

		/* add to epoll for read */
		if (unlikely(fd_epoll_alloc_update(wi->fe, fd, EPOLLIN | EPOLLRDHUP,
						_worker_recv_data)))
		{
			ERR("alloc update failed\n");
			return -1;
		}

		WFLOW(3, "%s %d event alloc read|closed\n", sd->side, fd);
	}
	/* add it to epoll to wait connect success */
	else {

		WFLOW(1, "%s %d connecting need wait\n", sd->side, fd);

		/* add to epoll for write */
		sd->flags |= SESSION_HANDSHAKE;
		if (unlikely(fd_epoll_alloc_update(wi->fe, fd, EPOLLOUT, 
						_worker_handshake)))
		{
			ERR("alloc update failed\n");
			return -1;
		}
		WFLOW(3, "%s %d update alloc write\n", sd->side, fd);
	}
	return 0;
}

static int 
_worker_handshake(int fd, int events, void *arg1, void *arg2)
{
	policy_t *pl;
	proxy_t *py;
	worker_t *wi;
	session_t *s;
	session_data_t *sd;
	fd_item_t *fi;
	int ret;

	assert(arg1);
	assert(arg2);

	s = arg1;
	sd = arg2;

	pl = s->policy;
	py = s->proxy;
	wi = s->worker;

	assert(pl);
	assert(py);
	assert(wi);
	assert(fd == sd->fd);

	fi = fd_epoll_map(wi->fe, fd);
	assert(fi);

	if (unlikely(events & EPOLLERR)) {
		ERR("handshake event error\n");
		goto failed;
	}

	ret = sk_is_connected(fd);
	if (unlikely(ret < 0)) {
		WFLOW(1, "%s %d handshake failed\n", sd->side, fd);
		goto failed;
	}

	WFLOW(1, "%s %d handshake success\n", sd->side, fd);

	sk_set_nodelay(fd, 1);
	sk_set_quickack(fd, 1);

	sd->flags &= ~SESSION_HANDSHAKE;
	
	/* handshake success, change it to read/readclose */
	if (unlikely(fd_epoll_alloc_update(wi->fe, fd, EPOLLIN | EPOLLRDHUP,
					_worker_recv_data)))
	{
		ERR("alloc update failed\n");
		goto failed;
	}
	WFLOW(3, "%s %d update alloc read|close\n", sd->side, fd);

	/* add task to send data if have */
	sd->task.task = TASK_SEND;
	if (unlikely(task_in_queue(&sd->task))) {
		ERR("task alreay in queue\n");
		return -1;
	}
	task_add_queue(wi->taskq, &sd->task);
	WFLOW(3, "%s %d task add parse\n", sd->side, sd->fd);

	return 0;

failed:

	/* delete session */
	sd->task.task = TASK_DELETE;
	if (unlikely(task_in_queue(&sd->task))) {
		ERR("task already in queue\n");
		return -1;
	}

	task_add_queue(wi->taskq, &sd->task);
	WFLOW(3, "%s %d task add delete\n", sd->side, sd->fd);

	return -1;
}

static int 
_worker_recv_data(int fd, int events, void *arg1, void *arg2)
{
	policy_t *pl;
	proxy_t *py;
	worker_t *wi;
	session_t *s;
	session_data_t *sd;
	packet_t *pkt;
	char *ptr;
	size_t len;
	int closed = 0;
	int n;
	int ret = 0;

	assert(fd > 0);
	assert(arg1);
	assert(arg2);

	s = arg1;
	sd = arg2;

	pl = s->policy;
	py = s->proxy;
	wi = s->worker;
	assert(pl);
	assert(py);
	assert(wi);
	assert(fd = sd->fd);

	if (unlikely(events & EPOLLERR)) {
		WFLOW(1, "%s %d recv error\n", sd->side, fd);
		//ERR("recv data epoll error\n");
		ret = -1;
		goto out_free;
	}

	pkt = CBLIST_GET_TAIL(&sd->in, packet_t *, list);

	/* no packet or last packet no space, alloc new packet */
	if (!pkt || pkt->len >= pkt->capacity) {
		pkt = objpool_get(wi->pktpool);
		if (unlikely(!pkt)) {
			ERR("alloc packet failed\n");
			goto out_free;
		}
		packet_init(pkt, wi->pktpool->objsize - sizeof(packet_t));
		CBLIST_ADD_TAIL(&sd->in, &pkt->list);
		s->nalloced++;
		WFLOW(2, "%s %d get a packet %p, nalloced %d\n", 
		      sd->side, sd->fd, pkt, s->nalloced);
	}	

	ptr = pkt->data + pkt->len;
	len = pkt->capacity - pkt->len;
	n = sk_recv(fd, ptr, len, &closed);
	WFLOW(1, "%s %d recv %d bytes\n", sd->side, fd, n);

	if (unlikely(n < 0))  {
		WFLOW(1, "%s %d recv error\n", sd->side, fd);
		ERR("recv data error\n");
		ret = -1;
		goto out_free;
	}

	pkt->len = n;

	if (closed || events & EPOLLRDHUP) {
		WFLOW(1, "%s %d recv read shutdown, events %d\n", 
		      sd->side, fd, events);
		sd->flags |= SESSION_SHUTRD;
		if (unlikely(fd_epoll_alloc_update(wi->fe, fd, 0, NULL))) {
			ERR("alloc events failed\n");
			ret = -1;
			goto out_free;
		}
		WFLOW(3, "%s %d event alloc delete\n", sd->side, fd);
	}

	/* alloc task when recv data in event callback mode */
	if (events) {
		sd->task.task = TASK_PARSE;
		if (unlikely(task_in_queue(&sd->task))) {
			ERR("task already in queue\n");
			return -1;
		}

		task_add_queue(wi->taskq, &sd->task);
		WFLOW(3, "%s %d task alloc parse\n", sd->side, fd);
	}

	return n;

out_free:

	/* delete session */
	sd->task.task = TASK_DELETE;
	if (unlikely(task_in_queue(&sd->task))) {
		ERR("task already in queue\n");
		return -1;
	}

	task_add_queue(wi->taskq, &sd->task);			
	WFLOW(3, "%s %d task alloc delete\n", sd->side, fd);

	return ret;
}

static int 
_worker_send_data(int fd, int events, void *arg1, void *arg2)
{
	policy_t *pl;
	proxy_t *py;
	worker_t *wi;
	session_t *s;
	session_data_t *sd;
	fd_item_t *fi;
	packet_t *pkt, *bk;
	char *ptr;
	size_t len;
	int n;
	int total;

	assert(fd > 0);
	assert(arg1);
	assert(arg2);

	s = arg1;
	sd = arg2;

	pl = s->policy;
	py = s->proxy;
	wi = s->worker;

	assert(pl);
	assert(py);
	assert(wi);
	assert(fd == sd->fd);

	/* in handshake */
	if (sd->flags & SESSION_HANDSHAKE)
		return 0;

	total = 0;

	/* send all packet out in once if can. */
	CBLIST_FOR_EACH_SAFE(&sd->out, pkt, bk, list) {

		ptr = pkt->data + pkt->sendpos;
		len = pkt->len - pkt->sendpos;

		n = sk_send(fd, ptr, len);
		if (unlikely(n < 0)) {
			ERR("%d send %lu bytes error\n", fd, len);
			goto out_failed;
		}

		pkt->sendpos += n;
		total += n;

		/* send blocked, add it to event */
		if (unlikely(n != (int)len)) {
			sd->flags |= SESSION_BLOCKED;
			WFLOW(1, "%s %d send %lu bytes blocked(%d)\n", 
					sd->side, fd, len, n);

			/* already in events */
			if (events)
				return 0;

			fi = fd_epoll_map(wi->fe, fd);
			assert(fi);			
			fi->arg1 = s;
			fi->arg2 = sd;

			/* alloc events update for write */
			if (unlikely(fd_epoll_alloc_update(wi->fe, fd, EPOLLOUT,
							_worker_send_data)))
			{
				ERR("alloc update failed\n");
				goto out_failed;
			}
			WFLOW(3, "%s %d event alloc write\n", sd->side, fd);

			/* stop peer side recv */
			fi = fd_epoll_map(wi->fe, sd->peer->fd);
			assert(fi);
			fi->state = FD_STOP;

			return 0;
		}

		WFLOW(1, "%s %d send %d bytes\n", sd->side, fd, n);

		CBLIST_DEL(&pkt->list);
		objpool_put(pkt);
		s->nalloced--;
		WFLOW(2, "%s %d free packet %p, nalloced %d\n", 
		      sd->side, fd, pkt, s->nalloced);
	}

	/* peer closed read */
	if (sd->peer->flags & SESSION_SHUTRD) {
		shutdown(fd, SHUT_WR);
		sd->peer->flags |= SESSION_SHUTWR;
		WFLOW(1, "%s %d send SHUT_WR\n", sd->side, fd);
	}

	return total;

out_failed:

	/* failed need delete session in event callback */
	if (events) {
		sd->task.task = TASK_DELETE;
		if (unlikely(task_in_queue(&sd->task)))
			ERR("task already in queue\n");

		task_add_queue(wi->taskq, &sd->task);
		WFLOW(3, "%s %d task alloc delete\n", sd->side, fd);
	}

	return -1;
}

static int 
_worker_parse_data(session_t *s, session_data_t *sd)
{
	policy_t *pl;
	proxy_t *py;
	//worker_t *wi;
	packet_t *pkt, *bk;
	session_data_t *peer;

	assert(s);
	assert(sd);
	
	pl = s->policy;
	py = s->proxy;
	//wi = s->worker;
	peer = sd->peer;

	/* put in data into peer side out */
	CBLIST_FOR_EACH_SAFE(&sd->in, pkt, bk, list)
	{
		CBLIST_DEL(&pkt->list);
		
		/* skip the empty packet */
		if (pkt->len == 0) {
			objpool_put(pkt);
			s->nalloced--;
			WFLOW(2, "%s %d free packet %p, nalloced %d\n", 
			      sd->side, sd->fd, pkt, s->nalloced);
			continue;
		}

		CBLIST_ADD_TAIL(&peer->out, &pkt->list);
		WFLOW(2, "%s %d move packet %p to %s %d, nalloced %d\n", 
		      sd->side, sd->fd, pkt, peer->side, peer->fd, s->nalloced);
	}
	
	WFLOW(1, "%s %d parse data success\n", sd->side, sd->fd);

        return 0;
}

static int 
_worker_run_task(task_t *t)
{
	policy_t *pl;
	proxy_t *py;
	worker_t *wi;
        session_t *s;
	session_data_t *sd, *peer;
        int ret = 0;

	assert(t);
	assert(t->arg1);
	assert(t->arg2);

        s = t->arg1;
	sd = t->arg2;

	pl = s->policy;
	py = s->proxy;
	wi = s->worker;
	assert(pl);
	assert(py);
	assert(wi);

	switch (t->task) {
	
	case TASK_NONE:
		ERR("invalid TASK_NONE\n");
		break;

	case TASK_DELETE:
		_worker_free_session(s, sd);
		break;

	case TASK_PARSE:
		/* parse data and put it into peer out list */
		ret = _worker_parse_data(s, sd);
		if (ret < 0) {
			_worker_free_session(s, sd);
			return -1;
		}

		/* check peer is connect or not */
		peer = sd->peer;
		if (peer->fd < 0 && _worker_connect(s, peer)) {
			_worker_free_session(s, sd);
			return -1;
		}

		/* send data to peer */
		ret = _worker_send_data(peer->fd, 0, s, peer);
		if (ret < 0) {
			_worker_free_session(s, sd);
			return -1;
		}
		break;

	case TASK_SEND:
		/* send data */
		ret = _worker_send_data(sd->fd, 0, s, sd);
		if (ret < 0) {
			_worker_free_session(s, sd);
			return -1;
		}
		break;

	default:
		ERR("invalid task type %d\n", t->task);
		return -1;
	}

	if ((s->clidata.flags & SESSION_SHUTWR) && 
	    (s->svrdata.flags & SESSION_SHUTWR)) 
	{
		WFLOW(1, "both side are SHUTWR, need delete session\n");
		_worker_free_session(s, sd);
	}

        return 0;
}

/**
 *	worker main loop
 *	
 *	Always return 0.
 */
static int 
_worker_loop(proxy_t *py)
{
	worker_t *wi;
	
	assert(py);
	assert(py->priv);
	wi = py->priv;

	while (!g_proxy.stop) {
		fd_epoll_flush_update(wi->fe);
		fd_epoll_poll(wi->fe);
		task_run_queue(wi->taskq);
	}

	return 0;
}

/**
 *	The main function of worker. 
 *
 *	Return NULL always.
 */
void *
worker_run(void *arg)
{
	proxy_t *py;
	int cpu;

	py = arg;
	assert(py);

	DBG("work[%d] started\n", py->index);

	/* bind to cpu */
	if (py->bind_cpu) {
		cpu = cproc_bind_cpu(getpid(), py->index, 
				      py->bind_cpu_algo, 
				      py->bind_cpu_ht);
		if (cpu < 0) {
			ERR("work[%d] bind CPU failed", py->index);
			return NULL;
		}

		DBG("work[%d] bind to cpu %d\n", py->index, cpu);
	}

	if (_worker_init(py)) {
		return NULL;
	}

	_worker_loop(py);

	_worker_free(py);

	DBG("work[%d] stoped\n", py->index);

	return NULL;
}


