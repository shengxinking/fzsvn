/**
 *	@file	connection.c
 *
 *	@brief	connection APIs
 *	
 *	@author	Forrest.zhang	
 *
 *	@date
 */

#include "sock_util.h"
#include "worker.h"
#include "thread.h"
#include "packet.h"
#include "session.h"
#include "policy.h"
#include "connection.h"
#include "proxy.h"
#include "proxy_debug.h"

#define	CFLOW(level, fmt, args...)		\
	FLOW(level, "%s(%04x) %d "fmt,		\
	     c->side, c->flags, c->fd, ##args)

int 
conn_init(connection_t *c, struct session *s, int dir, const char *side)
{
	if (!c || !s || !side)
		ERR_RET(-1, "invalid argument\n");
	
	c->s = s;
	c->fd = -1;
	c->ssl = NULL;
	c->dir = dir;
	c->side = side;

	memset(&c->peer, 0, sizeof(c->peer));
	memset(&c->local, 0, sizeof(c->local));

	CBLIST_INIT(&c->in);
	CBLIST_INIT(&c->out);

	task_init(&c->task, c, session_run_task);

	c->flags = 0;

	return 0;
}

int 
conn_free(connection_t *c)
{
	session_t *s;
	fd_item_t *fi;
	worker_t *wi;
	thread_t *ti;
	packet_t *pkt, *bak;

	if (unlikely(!c))
		ERR_RET(-1, "invalid argument\n");

	assert(c->s);
	s = c->s;
	assert(s->worker);
	assert(s->thread);
	wi = s->worker;
	ti = s->thread;

	/* free input queue */
	if (unlikely(!CBLIST_IS_EMPTY(&c->in))) {
		CBLIST_FOR_EACH_SAFE(&c->in, pkt, bak, list) {
			CBLIST_DEL(&pkt->list);
			objpool_put(pkt);
			s->nalloced--;
			CFLOW(2, "free (in)packet(%p), nalloced %d\n", 
			     pkt, s->nalloced);
		}
	}

	/* free output queue */
	if (unlikely(!CBLIST_IS_EMPTY(&c->out))) {
		CBLIST_FOR_EACH_SAFE(&c->out, pkt, bak, list) {
			CBLIST_DEL(&pkt->list);
			objpool_put(pkt);
			s->nalloced--;
			CFLOW(2, "free (out)packet(%p), nalloced %d\n", 
			     pkt, s->nalloced);
		}
	}

	/* close socket fd, not need update event */
	if (c->fd > 0) {
		fi = fd_epoll_map(wi->fe, c->fd);
		assert(fi);
		memset(fi, 0, sizeof(*fi));
		close(c->fd);
		CFLOW(1, "closed\n");

		c->fd = -1;
	}

	/* reset task type */
	c->task.task = TASK_NONE;

	return 0;
}

int 
conn_recv_data(int fd, int events, void *arg)
{
	int n;
	int ret;
	int len;
	char *ptr;
	int closed;
	int handshake;
	worker_t *wi;
	thread_t *ti;
	session_t *s;
	packet_t *pkt;
	connection_t *c;

	if (fd < 0 || !events || !arg)
		ERR_RET(-1, "invalid argument\n");

	c = arg;
	assert(c->s);
	s = c->s;
	assert(s->worker);
	assert(s->thread);
	wi = s->worker;
	ti = s->thread;
	assert(fd == c->fd);

	if (unlikely(events & EPOLLERR)) {
		//ERR("recv data epoll error\n");
		c->flags |= CONN_F_ERROR;
		ret = -1;
		goto err_free;
	}

ssl_read:

	/* try to recv data to last packet for save memory */
	pkt = CBLIST_GET_TAIL(&c->in, packet_t *, list);

	/* no packet or last packet no space, alloc new packet */
	if (!pkt || pkt->len >= pkt->max) {
		pkt = objpool_get(wi->pktpool);
		if (unlikely(!pkt)) {
			ERR("alloc packet failed\n");
			ret = -1;
			goto err_free;
		}
		PKT_INIT(pkt);
		CBLIST_ADD_TAIL(&c->in, &pkt->list);
		s->nalloced++;
		CFLOW(2, "get a packet(%p), nalloced %d\n", 
		      pkt, s->nalloced);
	}

	ptr = pkt->data + pkt->len;
	len = pkt->max - pkt->len;
	n = conn_raw_recv(fd, c->ssl, ptr, len, &closed, &handshake);
	CFLOW(1, "recv %d bytes\n", n);

	if (unlikely(n < 0))  {
		c->flags |= CONN_F_ERROR;
		CFLOW(1, "recv error: %s\n", ERRSTR);
		goto err_free;
	}
	pkt->len += n;
	
	/* recv handshake */
	if (handshake) {
		c->flags |= CONN_F_SSLHSK;
		CFLOW(1, "recv renegoatiate\n");
		ret = fd_epoll_add_event(wi->fe, fd, FD_OUT, conn_handshake);
		if (unlikely(ret)) {
			ERR("add event failed\n");
			goto err_free;
		}
		CFLOW(3, "add event(write)\n");
	}

	/* recv closed and not recv data */
	if (closed || events & EPOLLRDHUP) {
		c->flags |= CONN_F_SHUTRD;
		CFLOW(1, "recv read shutdown, events %d\n", events);
		if (c->ssl) {
			c->flags |= CONN_F_SSLSHUT;
			ret = fd_epoll_add_event(wi->fe, fd, FD_OUT, conn_shutdown);
			if (unlikely(ret)) {
				ERR("add event failed\n");
				goto err_free;
			}
			CFLOW(3, "add event(write)\n");
		}
		else {
			ret = fd_epoll_add_event(wi->fe, fd, 0, NULL);
			if (unlikely(ret)) {
				ERR("add event failed\n");
				goto err_free;
			}
			CFLOW(3, "add event(delete)\n");
		}

		/* empty packet */
		if (pkt->len == 0) {
			CBLIST_DEL(&pkt->list);
			objpool_put(pkt);
			s->nalloced--;
			CFLOW(2, "free packet(%p), nalloced %d\n", 
			      pkt, s->nalloced);
		}
	}

	/* handle ssl pending data */
	if (c->ssl && SSL_pending(c->ssl))
		goto ssl_read;

	/* alloc task when recv data in event callback mode */
	if (events) {
		c->task.task = TASK_PARSE;
		if (unlikely(task_in_queue(&c->task))) {
			ERR("task already in queue\n");
			return -1;
		}

		task_add_queue(wi->taskq, &c->task);
		CFLOW(3, "add task(parse)\n");
	}

	return n;

err_free:

	/* delete session */
	c->task.task = TASK_DELETE;
	if (unlikely(task_in_queue(&c->task))) {
		ERR("task already in queue\n");
		return -1;
	}

	task_add_queue(wi->taskq, &c->task);			
	CFLOW(3, "add task(delete)\n");

	return -1;
}

int 
conn_send_data(int fd, int events, void *arg)
{
	int n;
	int ret;
	int len;
	char *ptr;
	int total;
	fd_item_t *fi;
	worker_t *wi;
	thread_t *ti;
	session_t *s;
	packet_t *pkt, *bk;
	connection_t *c;
	connection_t *peer;
	
	if (fd < 0 || !arg)
		ERR_RET(-1, "invalid argument\n");

	c = arg;
	assert(c->s);
	s = c->s;
	assert(s->worker);
	assert(s->thread);
	assert(s->policy);
	wi = s->worker;
	ti = s->thread;
	assert(fd == c->fd);
	peer = &s->conns[(c->dir + 1) % 2];

	if ((c->flags & CONN_F_HSK) || (c->flags & CONN_F_SSLHSK))
		return 0;

	total = 0;

	/* send all packet out in once if can. */
	CBLIST_FOR_EACH_SAFE(&c->out, pkt, bk, list) {

		ptr = pkt->data + pkt->sendpos;
		len = pkt->len - pkt->sendpos;
		n = conn_raw_send(fd, c->ssl, ptr, len);
		if (unlikely(n < 0)) {
			CFLOW(1, "send %d bytes error: %s\n", len, ERRSTR);
			c->flags |= CONN_F_ERROR;
			goto err_free;
		}

		pkt->sendpos += n;
		total += n;

		/* send blocked, add it to event */
		if (unlikely(n != len)) {
			
			CFLOW(1, "send %d bytes blocked(%d)\n", len, n);

			/* already blocked */
			if (c->flags & CONN_F_BLOCKED)
				return 0;

			fi = fd_epoll_map(wi->fe, fd);
			assert(fi);			
			fi->arg = c;

			/* alloc events update for write */
			ret = fd_epoll_add_event(wi->fe, fd, FD_OUT, conn_send_data);
			if (unlikely(ret)) {
				ERR("alloc update failed\n");
				goto err_free;
			}
			CFLOW(3, "add event(write)\n");

			/* delete events update for peer read */
			fi = fd_epoll_map(wi->fe, peer->fd);
			assert(fi);
			ret = fd_epoll_add_event(wi->fe, peer->fd, 0, NULL);
			if (unlikely(ret)) {
				ERR("alloc update failed\n");
				goto err_free;
			}
			FLOW(3, "%s(%04x) %d add event(delete)\n" ,
			      peer->side, peer->flags, peer->fd);

			c->flags |= CONN_F_BLOCKED;
			return 0;
		}

		CFLOW(1, "send %d bytes\n", n);

		CBLIST_DEL(&pkt->list);
		objpool_put(pkt);
		s->nalloced--;
		CFLOW(2, "free packet(%p), nalloced %d\n", 
		      pkt, s->nalloced);
	}

	CFLOW(1, "send total %d bytes\n", total);

	/* clear blocked status */
	if (c->flags & CONN_F_BLOCKED) {
		fi = fd_epoll_map(wi->fe, fd);
		assert(fi);
		fi->arg = c;

		/* add events update for read */
		ret = fd_epoll_add_event(wi->fe, fd, FD_IN, conn_recv_data);
		if (unlikely(ret)) {
			ERR("alloc update failed\n");
			goto err_free;
		}
		FLOW(3, "%s(%04x) %d add event(read)\n" ,
				c->side, c->flags, c->fd);

		/* add events update for peer read */
		fi = fd_epoll_map(wi->fe, peer->fd);
		assert(fi);
		fi->arg = peer;
		ret = fd_epoll_add_event(wi->fe, peer->fd, FD_IN, conn_recv_data);
		if (unlikely(ret)) {
			ERR("alloc update failed\n");
			goto err_free;
		}
		FLOW(3, "%s(%04x) %d add event(read)\n" ,
				peer->side, peer->flags, peer->fd);

		c->flags &= ~CONN_F_BLOCKED;
	}

	/* peer closed read */
	if (peer->flags & CONN_F_SHUTRD) {
		if (c->ssl) {
			c->flags |= CONN_F_SSLSHUT;
			ret = fd_epoll_add_event(wi->fe, fd, FD_OUT, conn_shutdown);
			if (unlikely(ret)) {
				ERR("add event failed\n");
				goto err_free;
			}
			CFLOW(3, "add event(write)\n");
		}
		else {
			shutdown(fd, SHUT_WR);
			c->flags |= CONN_F_SHUTWR;
			CFLOW(1, "send SHUT_WR\n");
		}
	}

	return 0;

err_free:

	/* failed need delete session in event callback */
	if (events) {
		c->task.task = TASK_DELETE;
		if (unlikely(task_in_queue(&c->task)))
			ERR("task already in queue\n");

		task_add_queue(wi->taskq, &c->task);
		CFLOW(3, "add task(delete)\n");
	}

	return -1;
}

int 
conn_handshake(int fd, int events, void *arg)
{
	int e = 0;
	ssl_wt_e wait;
	thread_t *ti;
	worker_t *wi;
	session_t *s;
	connection_t *c;
	int ret;

	if (fd < 0 || !events || !arg)
		ERR_RET(-1, "invalid argument\n");

	c = arg;
	assert(c->s);
	s = c->s;
	assert(s->worker);
	assert(s->thread);
	assert(fd == c->fd);

	ti = s->thread;
	wi = s->worker;

	if (unlikely(events & (EPOLLERR | EPOLLRDHUP))) {
		ERR("handshake event error\n");
		goto err_free;
	}

	/* do tcp handshake */
	if (c->flags & CONN_F_HSK) {
		
		ret = sk_is_connected(fd);
		if (unlikely(ret < 0)) {
			c->flags |= CONN_F_ERROR;
			CFLOW(1, "handshake failed\n");
			goto err_free;
		}

		sk_set_nodelay(fd, 1);
		sk_set_quickack(fd, 1);
		sk_set_keepalive(fd);

		c->flags &= ~CONN_F_HSK;

		if (c->ssl)
			c->flags |= CONN_F_SSLHSK;
		CFLOW(1, "handshake success\n");

	}

	/* do SSL handshake */
	if (c->flags & CONN_F_SSLHSK) {
		ret = ssl_handshake(c->ssl, &wait);
		if (unlikely(ret)) {
			ERR("ssl handshake failed\n");
			goto err_free;
		}
		if ((wait == SSL_WT_READ) && (events & EPOLLOUT))
			e = FD_IN;
		else if ((wait == SSL_WT_WRITE) && (events & EPOLLIN))
			e = FD_OUT;
		
		if (e) {
			ret = fd_epoll_add_event(wi->fe, fd, e, conn_handshake);
			if (unlikely(ret)) {
				ERR("add event failed\n");
				goto err_free;
			}
			CFLOW(3, "add event(%s)\n", 
			     wait == SSL_WT_READ ? "read" : "write");
		}

		if (wait)
			return 0;

		c->flags &= ~CONN_F_SSLHSK;
		CFLOW(2, "ssl(%p) handshake success\n", c->ssl);
	}
	
	/* handshake success, change it to read/readclose */
	ret = fd_epoll_add_event(wi->fe, fd, FD_IN, conn_recv_data);
	if (unlikely(ret))
	{
		ERR("add event failed\n");
		goto err_free;
	}
	CFLOW(3, "add event(read)\n");
	
	/* add task to send data if have data. */
	c->task.task = TASK_SEND;
	if (unlikely(task_in_queue(&c->task))) {
		ERR("task alreay in queue\n");
		return -1;
	}
	
	task_add_queue(wi->taskq, &c->task);
	CFLOW(3, "add task(send)\n");

	return 0;

err_free:

	/* delete session */
	c->task.task = TASK_DELETE;
	if (unlikely(task_in_queue(&c->task))) {
		ERR("task already in queue\n");
		return -1;
	}

	task_add_queue(wi->taskq, &c->task);
	CFLOW(3, "add task(delete)\n");

	return -1;
}

int 
conn_shutdown(int fd, int events, void *arg)
{
	int e = 0;
	int ret;
	ssl_wt_e wait;
	thread_t *ti;
	worker_t *wi;
	session_t *s;
	connection_t *c;

	if (fd < 0 || !events || !arg)
		ERR_RET(-1, "invalid argument\n");

	c = arg;
	assert(c->s);
	s = c->s;
	assert(s->worker);
	assert(s->thread);
	assert(fd == c->fd);

	ti = s->thread;
	wi = s->worker;

	if (unlikely(events & EPOLLERR)) {
		CFLOW(1, "shutdown event error\n");
		goto err_free;
	}

	if (c->flags & CONN_F_SSLSHUT) {
		ret = ssl_shutdown(c->ssl, &wait);
		if (unlikely(ret)) {
			CFLOW(1, "ssl(%p) shutdown failed\n", c->ssl);
			goto err_free;
		}

		if ((wait == SSL_WT_READ) && (events & EPOLLOUT)) 
			e = FD_IN;
		else if ((wait == SSL_WT_WRITE) && (events & EPOLLIN))
			e = FD_OUT;

		if (e) {
			ret = fd_epoll_add_event(wi->fe, fd, e, conn_handshake);
			if (unlikely(ret)) {
				ERR("add event failed\n");
				goto err_free;
			}
			CFLOW(3, "add event(%s)\n", 
			     wait == SSL_WT_READ ? "read" : "write");
		}

		if (wait)
			return 0;

		CFLOW(1, "ssl(%p) shutdown success\n", c->ssl);

		ret = ssl_free(c->ssl);
		if (unlikely(ret)) {
			ERR("ssl_free failed\n");
			goto err_free;
		}
		c->ssl = NULL;
	}

	/* free fd from epoll */
	ret = fd_epoll_add_event(wi->fe, fd, 0, NULL);	
	if (unlikely(ret)) {
		ERR("alloc update failed\n");
		goto err_free;
	}
	CFLOW(3, "add event(delete)\n");

	return 0;

err_free:

	/* delete session */
	c->task.task = TASK_DELETE;
	if (unlikely(task_in_queue(&c->task))) {
		ERR("task already in queue\n");
		return -1;
	}

	task_add_queue(wi->taskq, &c->task);
	CFLOW(3, "add task(delete)\n");

	return -1;
}
