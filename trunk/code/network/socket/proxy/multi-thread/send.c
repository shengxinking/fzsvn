/**
 *	@file	send.c
 *
 *	@brief	the send thread implement.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2010-07-06
 */


#include <sys/epoll.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/epoll.h>

#include "send.h"
#include "debug.h"
#include "recv.h"
#include "proxy.h"
#include "sock.h"
#include "sesspool.h"
#include "mempool.h"


/**
 *	Init send thread resource.
 */
static int 
_send_init(thread_t *info)
{
	send_t *sinfo = NULL;

	assert(info);
	
	sinfo = malloc(sizeof(send_t));
	if (!sinfo) {
		ERR("malloc error: %s\n", strerror(errno));
		return -1;
	}
	memset(sinfo, 0, sizeof(send_t));
	
	sinfo->epfd = epoll_create(FD_MAX);
	if (sinfo->epfd < 0) {
		ERR("epoll_create error\n");
		return -1;
	}
	
	DBG("create epoll fd %d\n", sinfo->epfd);

	pthread_mutex_init(&sinfo->lock, NULL);

	info->priv = sinfo;
	
	return 0;
}


/**
 *	Free the send thread resource.
 */
static int 
_send_release(thread_t *info)
{
	send_t *sinfo = NULL;

	assert(info);

	sinfo = info->priv;

	if (sinfo) {
		if (sinfo->epfd > 0) {
			DBG("close epoll fd %d\n", sinfo->epfd);
			close(sinfo->epfd);
		}
	}
	free(sinfo);
	info->priv = NULL;

	return 0;
}


/**
 *	Free a session, it'll notify send thread.Free a session, 
 *	it'll notify send thread.
 */
static int 
_send_free_session(thread_t *info, session_t *s)
{
	send_t *sinfo;
	u_int32_t id;

	assert(info);
	assert(info->priv);
	assert(s);
	
	sinfo = info->priv;
	id = s->id;

	if (s->clifd > 0) {
		recv_del_fd(s->id, s->clifd);
	}

	if (s->svrfd > 0 && !(s->flags & SESSION_SVRWAIT)) {
		recv_del_fd(s->id, s->svrfd);
	}

	s->flags &= ~SESSION_SVRWAIT;

	s->flags |= SESSION_DELETE;

	DBG("id %u free session, refcnt %d\n", id, s->refcnt);

	sesspool_put(g_proxy.sesspool, s);

	return 0;
}


/**
 *	Get packet from @inq, and put them to @outq.
 */
static int 
_send_get_pkt(thread_t *info)
{
	send_t *sinfo;

	assert(info);
	assert(info->priv);

	sinfo = info->priv;

	pthread_mutex_lock(&sinfo->lock);
	
	if (sinfo->inq.len > 0) {
		DBG("get %d packet from inq\n", sinfo->inq.len);
		pktqueue_join(&sinfo->outq, &sinfo->inq);
	}

	pthread_mutex_unlock(&sinfo->lock);

	return 0;

}

/**
 *	Connect to real server.
 */
static int 
_send_get_svrfd(thread_t *info, session_t *s)
{
	send_t *sinfo;
	struct epoll_event event;
	int *pair;
	int svrfd;

	assert(info);
	assert(info->priv);
	assert(s);

	sinfo = info->priv;

	svrfd = sock_tcpcli(s->dip, s->dport, O_NONBLOCK, 0);
	if (svrfd < 0) {
		ERR("id %u connect to server %u.%u.%u.%u:%u failed\n", 
		    s->id, NIPQUAD(s->dip), ntohs(s->dport));
		g_proxy.stat.nsvrerrors++;
		return -1;
	}

	DBG("id %u get svrfd %d\n", s->id, svrfd);
	
	/* add it to epoll */	
	memset(&event, 0, sizeof(event));
	pair = &(event.data.fd);
	pair[0] = svrfd;
	pair[1] = s->id;
	event.events = EPOLLOUT;
	if (epoll_ctl(sinfo->epfd, EPOLL_CTL_ADD, svrfd, &event)) {
		ERR("id %u add new svrfd %d into epoll failed: %s\n", 
		    s->id, svrfd, strerror(errno));
		close(svrfd);
		g_proxy.stat.nsvrerrors++;
		return -1;
	}

	s->flags |= SESSION_SVRWAIT;
	s->svrfd = svrfd;
	
	DBG("id %d add svrfd %d to send epoll\n", s->id, svrfd);

	return 0;
}


/**
 *	Check the socket connect to real server is finished or error.
 */
static int 
_send_check_svrfd(thread_t *info)
{
	send_t *sinfo;	
	struct epoll_event events[FD_MAX];
	int n, i, fd;
	int *data;
	u_int32_t id;
	session_t *s;
	int err;
	size_t len;

	assert(info);
	assert(info->priv);

	sinfo = info->priv;

	n = epoll_wait(sinfo->epfd, events, FD_MAX, SEND_TIMEOUT);
	if (n < 0) {
		if (errno == EINTR)
			return 0;
		
		ERR("epoll_wait error: %s\n", strerror(errno));
		return -1;
	}
	
	if (n == 0) {
		sched_yield();
		return 0;
	}
	
	for (i = 0; i < n; i++) {
		data = &events[i].data.fd;
		fd = data[0];
		id = data[1];
		
		/* find session */
		s = sesspool_find(g_proxy.sesspool, id);
		if (!s) {
			ERR("id %u can't find session\n", id);
			continue;
		}

		/* remove svrfd from epoll */
		if (epoll_ctl(sinfo->epfd, EPOLL_CTL_DEL, s->svrfd, NULL)) {
			ERR("id %u del svrfd %d from epoll failed: %s\n", 
			    id, fd, strerror(errno));
			g_proxy.stat.nsvrerrors++;
		}

		/* check session flags */
		if (s->flags & SESSION_DELETE ||
		    events[i].events & EPOLLERR) 
		{
			ERR("connect to server failed\n");
			s->flags |= SESSION_SVRERROR;
			_send_free_session(info, s);
			g_proxy.stat.nsvrerrors++;
			continue;
		}

		len = sizeof(err);
		if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len)) {
			ERR("id %u svrfd %d getsockopt(SO_ERROR) error: %s\n", 
			    id, fd, strerror(errno));
			s->flags |= SESSION_SVRERROR;
			_send_free_session(info, s);
			g_proxy.stat.nsvrerrors++;
			continue;
		}

		/* check connect error */
		if (err) {
			ERR("connect to server failed\n");
			s->flags |= SESSION_SVRERROR;
			_send_free_session(info, s);
			g_proxy.stat.nsvrerrors++;
			continue;
		}

		/* clear flags */
		s->flags &= ~SESSION_SVRWAIT;

		sesspool_put(g_proxy.sesspool, s);

		DBG("id %u svrfd %d connect success\n", id, fd);

		/* notify recv thread */
		recv_add_fd(id, fd);
	}

	return 0;
}


/**
 *	Check the session flags and packet is need send or cached.
 *	Return 0 if need send, -1 need drop, 1 need cache.
 */
static int 
_send_check_pkt(thread_t *info, packet_t *pkt, session_t *s)
{
	assert(info);
	assert(info->priv);
	assert(pkt);
	assert(s);

	/* check svrfd is exist or not */
	if (PACKET_IS_CLI2SVR(pkt->flags)) {

		if (s->flags & SESSION_SVRERROR) {
			DBG("id %u svrfd is error\n", pkt->sid);
			s->refcnt--;
			sesspool_put(g_proxy.sesspool, s);
			return -1;
		}

		if (s->flags & SESSION_SVRCLOSE) {
			DBG("id %u svrfd is closed\n", pkt->sid);
			s->refcnt--;
			sesspool_put(g_proxy.sesspool, s);
			return -1;
		}

		/* connect to server */
		if (s->svrfd < 0 && _send_get_svrfd(info, s)) {
			DBG("id %u get svrfd failed\n", pkt->sid);
			s->refcnt--;
			_send_free_session(info, s);
			return -1;
		}
		
		/* the svr connect is no finished */
		if (s->flags & SESSION_SVRWAIT) {
			DBG("id %u need wait svrfd %d\n", 
			    pkt->sid, s->svrfd);
			sesspool_put(g_proxy.sesspool, s);
			return 1;
		}
	}
	else {			
		if (s->flags & SESSION_SVRERROR) {
			ERR("id %u svrfd is error\n", pkt->sid);
			s->refcnt--;
			sesspool_put(g_proxy.sesspool, s);
			return -1;
		}
		
		if (s->flags & SESSION_SVRCLOSE) {
			ERR("id %u svrfd is closed\n", pkt->sid);
			s->refcnt--;
			sesspool_put(g_proxy.sesspool, s);
			return -1;
		}
	}
	
	return 0;
}


/**
 *	Send packets in @outq.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_send_pkt(thread_t *info)
{
	send_t *sinfo;
	pktqueue_t pq = {0, NULL, NULL};
	packet_t *pkt;
	session_t *s;
	int n, m;
	int ret = 0;

	assert(info);
	assert(info->priv);

	sinfo = info->priv;

	if (sinfo->outq.len < 1)
		return 0;

	/* try send all packet in @outq */
	pkt = pktqueue_out(&sinfo->outq);
	while (pkt) {

		/* find session */
		s = sesspool_find(g_proxy.sesspool, pkt->sid);
		if (!s) {
			ERR("id %u can't find session", pkt->sid);
			mempool_put1(pkt);
			pkt = pktqueue_out(&sinfo->outq);
			continue;
		}

		ret = _send_check_pkt(info, pkt, s);
		if (ret) {
			if (ret == 1) {
				pktqueue_in(&pq, pkt);
				pkt = pktqueue_out(&sinfo->outq);
				continue;
			}
			else {
				mempool_put1(pkt);
				pkt = pktqueue_out(&sinfo->outq);
				continue;
			}
		}
		
		/* send packet */
		m = pkt->len - pkt->sendpos;
		if (PACKET_IS_CLI2SVR(pkt->flags)) {			
			n = sock_tcpsend(s->svrfd, pkt->data + pkt->sendpos, 
					 m, 0);
		}
		else {
			n = sock_tcpsend(s->clifd, pkt->data + pkt->sendpos, 
					 m, 0);
		}

		/* check send results */
		if (n < 0) {
			ERR("id %u send %d bytes failed\n", m, pkt->sid);
			s->refcnt--;
			_send_free_session(info, s);
			if (PACKET_IS_CLI2SVR(pkt->flags)) {
				g_proxy.stat.nsvrerrors++;
			}
			else {
				g_proxy.stat.nclierrors++;
			}
			mempool_put1(pkt);
			pkt = pktqueue_out(&sinfo->outq);	
			continue;
		}
		
		if (PACKET_IS_CLI2SVR(pkt->flags))
			g_proxy.stat.nsvrsends += n;
		else
			g_proxy.stat.nclisends += n;

		/* send finished */
		if (n ==  (pkt->len - pkt->sendpos)) {
			DBG("id %u send %d bytes finished\n", pkt->sid, m);
			if (PACKET_IS_CLI2SVR(pkt->flags)) {
				s->flags &= ~SESSION_SVRBLOCK;
			}
			else {
				s->flags &= ~SESSION_CLIBLOCK;
			}
			s->refcnt--;
			mempool_put1(pkt);
		}
		/* not send all data, need block */
		else {
			DBG("id %u send packet blocked(%d:%d)\n", 
			    pkt->sid, m, n);
			pkt->sendpos += n;
			pktqueue_in(&pq, pkt);
			if (PACKET_IS_CLI2SVR(pkt->flags))
				s->flags |= SESSION_SVRBLOCK;
			else
				s->flags |= SESSION_CLIBLOCK;
		}

		sesspool_put(g_proxy.sesspool, s);

		pkt = pktqueue_out(&sinfo->outq);
	}

	pktqueue_join(&sinfo->outq, &pq);

	return 0;
}


/**
 *	the main process of send thread 
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_send_loop(thread_t *info)
{
	assert(info);
	assert(info->priv);

	while (!(g_proxy.stop)) {

		_send_get_pkt(info);

		_send_check_svrfd(info);

		_send_pkt(info);		
	}

	return 0;
}


void *
send_run(void *arg)
{
	thread_t	*info;

	usleep(150);

	info = arg;
	assert(info);

	DBG("send thread start\n");

	if (_send_init(info))
		return NULL;

	_send_loop(info);

	_send_release(info);

	DBG("send thread exited\n");

	pthread_exit(0);
}


/**
 *	add a client fd to epoll fd
 */
int 
send_add_pkt(packet_t *pkt)
{
	thread_t *info;
	send_t *sinfo;

	assert(pkt);

	info = &g_proxy.send;
	sinfo = info->priv;
	if (!sinfo) {
		ERR("no send_t object\n");
		return -1;
	}

	pthread_mutex_lock(&sinfo->lock);

	pktqueue_in(&sinfo->inq, pkt);
	
	pthread_mutex_unlock(&sinfo->lock);

	return 0;
}




