/*
 *	@file	recv.c
 *	@brief	process client connections and data
 *
 */

#include <assert.h>
#include <sys/epoll.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "recv.h"
#include "send.h"
#include "debug.h"
#include "proxy.h"
#include "mempool.h"
#include "sock.h"
#include "sesspool.h"
#include "packet.h"
#include "work.h"


/**
 *	Alloc resource for recv thread.
 */
static int 
_recv_init(thread_t *info)
{
	recv_t *rinfo = NULL;

	assert(info);
	
	rinfo = malloc(sizeof(recv_t));
	if (!rinfo) {
		ERR("malloc error: %s\n", strerror(errno));
		return -1;
	}
	memset(rinfo, 0, sizeof(recv_t));
		
	rinfo->epfd = epoll_create(FD_MAX);
	if (rinfo->epfd < 0) {
		ERR("epoll_create error\n");
		return -1;
	}

	DBG("recv(%d) create epoll fd %d\n", info->index, rinfo->epfd);
	
	info->priv = rinfo;

	return 0;
}


/**
 *	Release resource for recv thread.
 */
static int 
_recv_release(thread_t *info)
{
	recv_t *rinfo = NULL;

	assert(info);

	rinfo = info->priv;
	if (!rinfo)
		return 0;

	if (rinfo->epfd > 0) {
		DBG("recv %d close epoll fd %d\n", info->index, rinfo->epfd);
		close(rinfo->epfd);
	}

	free(rinfo);
	info->priv = NULL;
	
	return 0;
}


/**
 *	Free session in recv thread.
 */
static int 
_recv_free_session(thread_t *info, session_t *s)
{
	recv_t *rinfo;
	u_int32_t id;

	assert(info);
	assert(info->priv);
	assert(s);

	rinfo = info->priv;
	id = s->id;

	if (s->clifd > 0) {
		/* clear client fd from epoll queue */
		if (epoll_ctl(rinfo->epfd, EPOLL_CTL_DEL, s->clifd, NULL)) {
			ERR("id %u delete clifd %d from recv epoll failed: %s\n", 
			    s->id, s->clifd, strerror(errno));
		}
		else {
			DBG("recv(%d) id %u delete clifd %d from recv epoll\n", 
			    info->index, id, s->clifd);
			s->refcnt--;
		}
	}
	if (s->svrfd > 0) {
		/* clear client fd from epoll queue */
		if (epoll_ctl(rinfo->epfd, EPOLL_CTL_DEL, s->svrfd, NULL)) {
			ERR("id %u delete svrfd %d from recv epoll failed: %s\n", 
			    s->id, s->svrfd, strerror(errno));
		}
		else {
			DBG("recv(%d) id %u delete svrfd %d from recv epoll\n", 
			    info->index, id, s->svrfd);
			s->refcnt--;
		}
	}
	
	s->flags |= SESSION_DELETE;

	DBG("recv(%d) id %u free session, refcnt %d\n", 
	    info->index, id, s->refcnt);

	sesspool_put(g_proxy.sesspools[info->index], s);

//	work_del_id(info->index, id);

//	sesspool_print(g_proxy.sesspools[info->index]);

	return 0;
}



static int 
_recv_add_fd(thread_t *info)
{
	recv_t *rinfo;
	fd_pair_t pairs[FDPAIR_MAX];
	struct epoll_event e;
	int size;
	int *pair;
	int i;
	int len;
	session_t *s;

	assert(info);
	assert(info->priv);

	rinfo = info->priv;

	pthread_mutex_lock(&rinfo->infds.lock);
	
	size = rinfo->infds.size;
	if (size > 0) {
		len = sizeof(fd_pair_t) * size;
		memcpy(pairs, rinfo->infds.array, len);
		rinfo->infds.size = 0;
	}

	pthread_mutex_unlock(&rinfo->infds.lock);

	if (size < 1)
		return 0;

	/* add new fd into epoll */
	for (i = 0; i < size; i++) {
		memset(&e, 0, sizeof(e));
		pair = &e.data.fd;
		pair[0] = pairs[i].fd;
		pair[1] = pairs[i].id;
		e.events = EPOLLIN;
		
		/* check session */
		s = sesspool_find(g_proxy.sesspools[info->index], pairs[i].id);
		if (!s) {
			ERR("id %u can't find session\n", pairs[i].id);
			continue;
		}

		/* add socket to epoll */
		if (epoll_ctl(rinfo->epfd, EPOLL_CTL_ADD, pair[0], &e)) 
		{
			ERR("epoll_ctl add id %d, fd %d error: %s\n",
			    pair[1], pair[0], strerror(errno));
			_recv_free_session(info, s);
			continue;
		}

		DBG("recv(%d) id %u fd %d add to recv epoll\n", 
		    info->index, pairs[i].id, pairs[i].fd);

		s->refcnt++;

		sesspool_put(g_proxy.sesspools[info->index], s);
	}

	return 0;
}


static int 
_recv_del_fd(thread_t *info)
{
	recv_t *rinfo;
	fd_pair_t pairs[FDPAIR_MAX];
	int len;
	int size;
	int i;
	session_t *s;

	assert(info);
	assert(info->priv);

	rinfo = info->priv;

	pthread_mutex_lock(&rinfo->delfds.lock);
	
	size = rinfo->delfds.size;
	if (size > 0) {
		len = sizeof(fd_pair_t) * size;
		memcpy(pairs, rinfo->delfds.array, len);
		rinfo->delfds.size = 0;
	}

	pthread_mutex_unlock(&rinfo->delfds.lock);

	if (size < 1)
		return 0;

	/* delete socket from epoll */
	for (i = 0; i < size; i++) {
		DBG("recv(%d) id %u fd %d remove from recv thread\n", 
		    info->index, pairs[i].id, pairs[i].fd);
		
		s = sesspool_find(g_proxy.sesspools[info->index], pairs[i].id);
		if (!s) {
			ERR("id %u not found session\n", pairs[i].id);
			continue;
		}
		
		if (epoll_ctl(rinfo->epfd, EPOLL_CTL_DEL, pairs[i].fd, NULL)) {
			ERR("id %u delete fd %d from epoll failed: %s\n", 
			    pairs[i].id, pairs[i].fd, strerror(errno));
		}
		else {
			DBG("recv(%d) id %u delete fd %d from epoll\n", 
			    info->index, pairs[i].id, pairs[i].fd);
			s->refcnt--;
		}
		
		sesspool_put(g_proxy.sesspools[info->index], s);
	}
	
	return 0;
}


/**
 *	Receive data from socket @fd, and assign the packet to 
 *	Parse thread.
 */
static int 
_recv_pkt(thread_t *info, u_int32_t id, int fd)
{
	int n;
	packet_t *pkt = NULL;
	u_int32_t capacity;
	int closed = 0;
	session_t *s;
	
	assert(info);
	assert(info->priv);

	s = sesspool_find(g_proxy.sesspools[info->index], id);
	if (!s) {
		ERR("id %u can't find session\n", id);
		return -1;
	}

	/* session is need delete, not recv packet */
	if (s->flags & SESSION_DELETE) {
		DBG("recv(%d) id %u is deleted\n", info->index, s->id);
		sesspool_put(g_proxy.sesspools[info->index], s);
		return 0;
	}

	/* session is deleted, client/server socket blocked */
	if ( (s->flags & SESSION_DELETE) ||
	     (fd == s->clifd && s->flags & SESSION_SVRBLOCK) ||
	     (fd == s->svrfd && s->flags & SESSION_CLIBLOCK) ) 
	{
		sesspool_put(g_proxy.sesspools[info->index], s);
		return 0;
	}

	pkt = mempool_get(g_proxy.pktpool);
	if (!pkt) {
		ERR("can't get packet from memory pool\n");
		_recv_free_session(info, s);
		return -1;
	}

        capacity = ((mempool_t *)(g_proxy.pktpool))->objsize;
	capacity -= sizeof(packet_t);
	pkt->capacity = capacity;
	pkt->sid = id;
	pkt->len = 0;
	pkt->flags = 0;
	pkt->sendpos = 0;	
	
	/* one socket only recv one packet */
	n = sock_tcprecv(fd, pkt->data, RECV_PKTSIZE, 0, &closed);

	DBG("recv(%d) id %u fd %u recved %d bytes\n", 
	    info->index, id, fd, n);
	
	if (n < 0) {
		/* the peer is failed */
		ERR("id %u fd %d recv failed\n", id, fd);
		mempool_put1(pkt);
		_recv_free_session(info, s);
		return -1;
	}
	/* peer closed */
	if (closed && n == 0) {
		DBG("recv(%d) id %u fd %d closed\n", 
		    info->index, id, fd);
		mempool_put1(pkt);
		if (s->svrfd == fd) {
			s->flags |= SESSION_SVRCLOSE;
			g_proxy.stat.nsvrcloses++;
		}
		else {
			s->flags |= SESSION_CLICLOSE;
			g_proxy.stat.nclicloses++;
		}
		_recv_free_session(info, s);
		return 0;
	}

	/* no data recved */
	if (n == 0) {
		mempool_put1(pkt);
		sesspool_put(g_proxy.sesspools[info->index], s);
		return 0;
	}
	
	pkt->len = n;
	
	if (s->svrfd == fd) {
		PACKET_SET_SVR2CLI(pkt->flags);
		g_proxy.stat.nsvrrecvs += n;
	}
	else {
		PACKET_SET_CLI2SVR(pkt->flags);
		g_proxy.stat.nclirecvs += n;
	}

	s->refcnt++;
	sesspool_put(g_proxy.sesspools[info->index], s);

	send_add_pkt(info->index, pkt);

	return 0;
}


/**
 *	the main function of receive thread 
 */
static int 
_recv_loop(thread_t *info)
{
	struct epoll_event events[FD_MAX];
	int n, i, fd;
	int *data;
	u_int32_t id;
	recv_t *rinfo;

	assert(info);
	assert(info->priv);

	rinfo = info->priv;

	while (!(g_proxy.stop)) {

		_recv_add_fd(info);

		_recv_del_fd(info);

		n = epoll_wait(rinfo->epfd, events, FD_MAX, RECV_TIMEOUT);
		if (n < 0) {
			if (errno == EINTR || errno == EAGAIN)
				continue;			
			ERR("epoll_wait error: %s\n", strerror(errno));
		}
		
		if (n == 0) {
			sched_yield();
			continue;
		}
		
		for (i = 0; i < n; i++) {
			data = &events[i].data.fd;
			fd = data[0];
			id = data[1];
			
//			DBG("id %u fd %d have data\n", id, fd);

			_recv_pkt(info, id, fd);
		}
	}

	return 0;
}

void *recv_run(void *arg)
{
	thread_t	*info;

	usleep(300);

	info = arg;
	assert(info);
	DBG("recv(%d) thread start\n", info->index);

	
	if (_recv_init(info))
		return NULL;

	_recv_loop(info);

	_recv_release(info);

	DBG("recv(%d) thread exited\n", info->index);

	pthread_exit(0);
}


/**
 *	add a client fd to epoll fd
 */
int recv_add_fd(int index, u_int32_t id, int fd)
{
	thread_t *info;
	recv_t *rinfo;
	int size;
	int ret = -1;

	assert(index >= 0 && index < g_proxy.nworks);
	assert(id > 0 && id <= g_proxy.capacity);
	assert(fd >= 0);

	info = &g_proxy.recvs[index];
	rinfo = info->priv;
	if (!rinfo)
		return -1;

	pthread_mutex_lock(&rinfo->infds.lock);

	size = rinfo->infds.size;

	if ( (size + 1) > FDPAIR_MAX) {
		ERR("too many fd add to recv input fd array\n");
	}
	else {
		rinfo->infds.array[size].id = id;
		rinfo->infds.array[size].fd = fd;
		rinfo->infds.size++;
		ret = 0;
	}
	
	pthread_mutex_unlock(&rinfo->infds.lock);

	return ret;
}


/** 
 *	delete a fd from epoll fd
 */
int recv_del_fd(int index, u_int32_t id, int fd)
{
	thread_t *info;
	recv_t *rinfo;
	int size;
	int ret = -1;

	assert(index >= 0 && index < g_proxy.nworks);
	assert(id > 0 && id <= g_proxy.capacity);
	assert(fd >= 0);

	info = &g_proxy.recvs[index];
	rinfo = info->priv;
	if (!rinfo)
		return ret;

	pthread_mutex_lock(&rinfo->delfds.lock);

	size = rinfo->delfds.size;

	if ( (size + 1) > FDPAIR_MAX) {
		ERR("too many fd add to recv input fd array\n");
	}
	else {
		rinfo->delfds.array[size].id = id;
		rinfo->delfds.array[size].fd = fd;
		rinfo->delfds.size++;
		ret = 0;
	}

	pthread_mutex_unlock(&rinfo->delfds.lock);

	return ret;
}



