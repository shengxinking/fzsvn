/**
 *	file	proxy.c
 *
 *	brief	The proxy main APIs, it accept connections, recv data, 
 *		parse data and send data.
 *
 *	author	Forrest.zhang
 */

#include <stdio.h>
#include <error.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <sys/resource.h>
#include <signal.h>

#include "proxy.h"
#include "sock_util.h"
#include "objpool.h"

/**
 *	Define MACROs for print error message and debug message.
 */
#define _PROXY_DEBUG	1
#ifdef	_PROXY_DEBUG
#define	_PROXY_ERR(fmt, args...)	fprintf(stderr, "proxy:%s:%d: "fmt, \
						__FILE__, __LINE__, ##args)
#define _PROXY_FLOW(fmt, args...)	printf("proxy: "fmt, ##args)
#else
#define _PROXY_ERR(fmt, args...)
#define _PROXY_FLOW(fmt, args...)
#endif

#define		CLOSE_CLIENT	(1 << 0)
#define		CLOSE_SERVER	(1 << 1)
#define		CLOSE_BOTH	(CLOSE_CLIENT | CLOSE_SERVER)

static int	g_stop = 0;
static proxy_t	g_proxy;

/**
 *	convert IP address(uint32_t) to dot-decimal string
 *	like xxx.xxx.xxx.xxx 
 */
#ifndef NIPQUAD
#define NIPQUAD(addr) \
        ((unsigned char *)&addr)[0], \
        ((unsigned char *)&addr)[1], \
        ((unsigned char *)&addr)[2], \
        ((unsigned char *)&addr)[3]
#define NIPQUAD_FMT "%u.%u.%u.%u"
#endif

static void _proxy_accept_client(ep_event_t *ev);
static int _proxy_get_server(proxy_t *ctx, session_t *s);
static void _proxy_check_server(ep_event_t *ev);
static void _proxy_recvfrom_client(ep_event_t *ev);
static void _proxy_recvfrom_server(ep_event_t *ev);
static void _proxy_sendto_client(ep_event_t *ev);
static void _proxy_sendto_server(ep_event_t *ev);
static int _proxy_parse_client(proxy_t *ctx, session_t *s, ep_event_t *ev);
static int _proxy_parse_server(proxy_t *ctx, session_t *s, ep_event_t *ev);

/**
 *	The signal handler of SIGINT, it set global variable @g_stop, then stop
 *	the proxy.
 *
 *	No return.
 */
static void 
_proxy_stop(int signo)
{
	if (signo == SIGINT) {
		_PROXY_FLOW("proxy recved stop signal\n");
		g_stop = 1;
	}
}

/**
 *	Initiate a proxy_t struct @ctx. Create epoll socket and 
 *	listen socket. and alloc some resources
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_proxy_initiate(proxy_t *ctx)
{
	struct rlimit rlim;

	assert(ctx);

	/* create listen socket */
	ctx->listenfd = sk_tcp_server(&ctx->svraddr);
	if (ctx->listenfd < 0) {
		_PROXY_ERR("create listen fd error\n");
		return -1;
	}
	sk_set_nonblock(ctx->listenfd, 1);
	_PROXY_FLOW("create listen fd %d\n", ctx->listenfd);

	/* create memory pool */
	ctx->pktpool = objpool_alloc(1024, 1, 0);
	if (!ctx->pktpool) {
		_PROXY_ERR("create objpool for packet error\n");
		return -1;
	}
	_PROXY_FLOW("create memory pool for packet %p\n", ctx->pktpool);
	
	ctx->ssnpool = objpool_alloc(sizeof(session_t), 1, 0);
	if (!ctx->ssnpool) {
		_PROXY_ERR("create objpool for session error\n");
		return -1;
	}
	_PROXY_FLOW("create memory pool for session %p\n", ctx->ssnpool);
	
	/* create session table */
	ctx->sessions = session_table_alloc(ctx->capacity);
	if (!ctx->sessions) {
		_PROXY_ERR("create session table error\n");
		return -1;
	}
	_PROXY_FLOW("create session table %p\n", ctx->sessions);

	/* set max file descriptor to 2 * capacity if capacity > 512 */
	if (ctx->capacity > 512) {
		rlim.rlim_cur = 3 * ctx->capacity;
		rlim.rlim_max = 3 * ctx->capacity;
		
		if (setrlimit(RLIMIT_NOFILE, &rlim)) {
			_PROXY_ERR("setrlimit(RLIMIT_NOFILE) error: %s",
				   strerror(errno));
			return -1;
		}
		
		_PROXY_FLOW("set max file descriptor to %d\n", 
			    (2 * ctx->capacity));
	}

	/* create read epoll event context */
	ctx->epctx = ep_event_create(ctx->capacity);
	if (!ctx->epctx) {
		_PROXY_ERR("create read epoll event context failed\n");
		return -1;
	}

	/* add listen fd to read event */
	ctx->acceptev.data = NULL;
	ctx->acceptev.fd = ctx->listenfd;
	ctx->acceptev.read_func = _proxy_accept_client;
	ctx->acceptev.write_func = NULL;	
	if (ep_event_add_read(ctx->epctx, &ctx->acceptev)) {
		_PROXY_ERR("add accept event failed\n");
		return -1;
	}

	/* set SIGINT as quit signal*/
	signal(SIGINT, _proxy_stop);

	return 0;
}

/**
 *	Free resource alloced by _proxy_initiate().
 *
 *	No return.
 */
static void 
_proxy_release(proxy_t *ctx)
{
	assert(ctx);

	if (ctx->listenfd >= 0)
		close(ctx->listenfd);

	if (ctx->pktpool)
		objpool_free(ctx->pktpool);

	if (ctx->ssnpool)
		objpool_free(ctx->ssnpool);
	
	if (ctx->sessions)
		session_table_free(ctx->sessions);

	if (ctx->acceptev.is_read) {
		ep_event_del(ctx->epctx, &ctx->acceptev);
	}

	if (ctx->epctx) {
		ep_event_destroy(ctx->epctx);
	}

	_PROXY_FLOW("all resource is freed\n");
}

/**
 *	Alloc a new session
 *
 *	Return 0 if success, -1 on error.
 */
static session_t * 
_proxy_alloc_session(proxy_t *ctx)
{
	session_t *s;

	s = objpool_get(ctx->ssnpool);
	if (!s) {
		_PROXY_ERR("get session from ssnpool failed\n");
		return NULL;
	}

	memset(s, 0, sizeof(session_t));

	if (session_table_add(ctx->sessions, s)) {
		_PROXY_ERR("add session to session table failed\n");
		objpool_put(s);
		return NULL;
	}

	s->svrfd = -1;

	return s;
}

/**
 *	Clean the client/server session.
 *
 *	No return.
 */
static void 
_proxy_clean_session(proxy_t *ctx, session_t *s, session_queue_t *q)
{
	packet_t *pkt;
	
	assert(ctx);
	assert(s);
	assert(q);

	pkt = q->pkt;
	if (pkt) {
		s->nalloced--;
		_PROXY_FLOW("1 ssn[%d] free a packet %p\n", s->id, pkt);
		objpool_put(pkt);
	}

	pkt = q->blkpkt;
	if (pkt) {
		s->nalloced--;
		_PROXY_FLOW("2 ssn[%d] free a packet %p\n", s->id, pkt);
		objpool_put(pkt);
	}

	pkt = pktq_out(&q->inq);
	while(pkt) {
		s->nalloced--;
		_PROXY_FLOW("3 ssn[%d] free a packet %p\n", s->id, pkt);
		objpool_put(pkt);
		pkt = pktq_out(&q->inq);
	}

	pkt = pktq_out(&q->outq);
	while (pkt) {
		s->nalloced--;
		_PROXY_FLOW("4 ssn[%d] free a packet %p\n", s->id, pkt);
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
_proxy_free_session(proxy_t *ctx, session_t *s, int side)
{
	assert(ctx);
	assert(s);
	int fd;

	/* close client socket */
	if (side & CLOSE_CLIENT) {
		
		_proxy_clean_session(ctx, s, &s->cliq);

		fd = s->clifd;

		if (s->cliev.is_read) {
			ep_event_del_read(ctx->epctx, &s->cliev);
		}
		
		_PROXY_FLOW("ssn[%d] client %d closed\n", s->id, fd);
		close(s->clifd);
		s->clifd = -1;
	}

	/* close server socket */
	if (side & CLOSE_SERVER) {

		_proxy_clean_session(ctx, s, &s->svrq);

		fd = s->svrfd;

		if (s->svrev.is_read || s->svrev.is_write)
			ep_event_del(ctx->epctx, &s->svrev);
		
		_PROXY_FLOW("ssn[%d] server %d closed\n", s->id, fd);
		close(s->svrfd);
		s->svrfd = -1;
	}

	if (s->clifd < 0 && s->svrfd < 0) {
		assert(s->nalloced == 0);
		/* delete session from session pool */
		_PROXY_FLOW("ssn[%d] deleted\n", s->id);
		session_table_del(ctx->sessions, s->id);

		ctx->stat.nliveconn--;
	}
}

/**
 *	Get a packet and put it in session_queue.
 *
 *	Return NULL if failed.
 */
extern packet_t * 
_proxy_get_packet(proxy_t *ctx, session_t *s, session_queue_t *q)
{
	packet_t *pkt;

	/* get packet to restore recved data */
	pkt = q->pkt;
	if (pkt) {
		return pkt;
	}

	pkt = objpool_get(ctx->pktpool);
	if (!pkt) {
		_PROXY_ERR("can't get packet from memory pool\n");
		return NULL;
	}
	s->nalloced++;
	q->pkt = pkt;
	pkt->len = 0;
	pkt->sendpos = 0;
	pkt->pos = q->pos;
	pkt->capacity = ctx->pktpool->objsize - sizeof(packet_t);

	if (q == &s->cliq)
		_PROXY_FLOW("ssn[%d] client %d get a packet(%p)\n", 
			    s->id, s->clifd, pkt);
	else 
		_PROXY_FLOW("ssn[%d] server %d get a packet(%p)\n",
			    s->id, s->svrfd, pkt);
	
	return pkt;
}

/**
 *	The error event.
 *
 *	No return.
 */
static void 
_proxy_event_error(ep_event_t *ev)
{
	proxy_t *ctx;
	session_t *s;

	ctx = &g_proxy;
	s = ev->data;

	if (ev->fd == s->clifd) {
		s->is_clierror = 1;
		_proxy_free_session(ctx, s, CLOSE_CLIENT);
	}
	else if (ev->fd == s->svrfd) {
		s->is_svrerror = 1;
		_proxy_free_session(ctx, s, CLOSE_SERVER);		
	}
	else {
		_PROXY_ERR("unknowed event fd %d\n", ev->fd);
	}
}

/**
 *	Accept client TCP connection, and add it to epoll socket and
 *	session pool.
 *
 *	Return 0 if accept success, -1 on error.
 */
static void
_proxy_accept_client(ep_event_t *ev)
{
	int clifd;
	ip_port_t cliaddr;
	session_t *s;
	proxy_t *ctx;
	char buf[512];

	ctx = &g_proxy;
	assert(ev);
	
	clifd = sk_accept(ev->fd, &cliaddr);
	if (clifd < 0) {
		if (errno != EAGAIN && errno != EAGAIN)
			_PROXY_ERR("accept error: %s\n", strerror(errno));
		return ;
	}

	/* add client socket to session pool */
	s = _proxy_alloc_session(&g_proxy);
	if (!s) {
		close(clifd);
		return;
	}
	s->clifd = clifd;

	/* add it to epoll event */
	s->cliev.data = s;
	s->cliev.read_func = _proxy_recvfrom_client;
	s->cliev.write_func = NULL;
	s->cliev.error_func = _proxy_event_error;
	s->cliev.fd = clifd;
	if (ep_event_add_read(ctx->epctx, &s->cliev)) {
		_PROXY_ERR("add client read event failed\n");
		_proxy_free_session(ctx, s, CLOSE_BOTH);
	}

	g_proxy.stat.nliveconn++;

	_PROXY_FLOW("ssn[%d] client %d accept, %s\n", 
		    s->id, clifd, ip_port_to_str(&cliaddr, buf, sizeof(buf)));
}

/**
 *	Select a server and connect to it.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_proxy_get_server(proxy_t *ctx, session_t *s)
{
	int fd;
	int wait;
	static int i = 0;

	fd = sk_tcp_client_nb(&ctx->rsaddrs[i], &wait);
	if (fd < 0) {
		s->is_svrerror = 1;
		_PROXY_ERR("get server socket error\n");
		return -1;
	}
	s->svrfd = fd;

	/* to next real server */
	i = (i + 1) % ctx->nrsaddr;

	/* add it to send epoll to wait connect success */
	if (wait) {
		/* add it to write event */
		s->svrev.data = s;
		s->svrev.fd = fd;
		s->svrev.write_func = _proxy_check_server;
		s->svrev.read_func = NULL;
		s->svrev.error_func = _proxy_event_error;
		if (ep_event_add_write(ctx->epctx, &s->svrev))
			return -1;
		
		s->is_svrwait = 1;
		_PROXY_FLOW("ssn[%d] client %d connect to server %d wait\n", 
			    s->id, s->clifd, fd);
	}

	else {
		/* add to read event */
		s->svrev.data = s;
		s->svrev.fd = fd;
		s->svrev.write_func = NULL;
		s->svrev.read_func = _proxy_recvfrom_server;
		s->svrev.error_func = _proxy_event_error;
		if (ep_event_add_read(ctx->epctx, &s->svrev))
			return -1;

		/* send data to server */
		_proxy_sendto_server(&s->svrev);
	}

	return 0;
}



/**
 *	Recv data from client.
 *
 *	No return.
 */
static void 
_proxy_recvfrom_client(ep_event_t *ev)
{
	proxy_t *ctx;
	packet_t *pkt;
	session_t *s;
	int fd;
	int n;
	int blen = 0;
	int close = 0;

	ctx = &g_proxy;
	s = ev->data;

	assert(s);
	assert(s->clifd >= 0);
	
	fd = s->clifd;

	if (s->is_svrwait || s->is_svrblock)
		return;

	/* get packet */
	pkt = _proxy_get_packet(ctx, s, &s->cliq);
	if (!pkt) {
		s->is_clierror = 1;
		_proxy_free_session(ctx, s, CLOSE_CLIENT);
		return;
	}
	
	/* recv data */
	blen = pkt->capacity - pkt->len;
	assert(blen > 0);
	n = sk_recv(fd, pkt->data, blen, &close);

	/* recv client data failed */
	if (n < 0) {
		s->is_clierror = 1;
		_PROXY_FLOW("ssn[%d] client %d recv error\n", s->id, fd);
		_proxy_free_session(ctx, s, CLOSE_CLIENT);
		return;
	}

	s->cliq.nbyte += n;
	pkt->len += n;

	_PROXY_FLOW("ssn[%d] client %d recv %d bytes\n", s->id, fd, n);

	if (pkt->len == 0) {
		s->nalloced--;
		s->cliq.pkt = NULL;
		objpool_put(pkt);
		_PROXY_FLOW("ssn[%d] client %d free packet %p\n",
			    s->id, fd, pkt);
	}

	/* client closed */
	if (close) {
		s->is_cliclose = 1;		
		if (s->cliq.nbyte < 1) {
			if (s->svrfd > 0) {
				shutdown(s->svrfd, SHUT_WR);
			}
			_proxy_free_session(ctx, s, CLOSE_CLIENT);
		}
		_PROXY_FLOW("ssn[%d] client %d recv closed\n", s->id, fd);
	}

	if (_proxy_parse_client(ctx, s, ev)) {
		_proxy_free_session(ctx, s, CLOSE_CLIENT);
	}
}

static void 
_proxy_recvfrom_server(ep_event_t *ev)
{
	proxy_t *ctx;
	session_t *s;
	int svrfd;
	packet_t *pkt;
	int n;
	int blen = 0;
	char *ptr;
	int close = 0;

	ctx = &g_proxy;
	s = ev->data;

	assert(ctx);
	assert(s);
	assert(s->svrfd >= 0);
	
	svrfd = s->svrfd;
	pkt = s->svrq.pkt;

	/* get a new packet */
	pkt = _proxy_get_packet(ctx, s, &s->svrq);
	if (!pkt) {
		s->is_svrerror = 1;
		_proxy_free_session(ctx, s, CLOSE_SERVER);
		return;
	}
	
	/* recv data */
	ptr = pkt->data + pkt->recvpos;
	blen = pkt->capacity - pkt->recvpos;
	assert(blen > 0);
	n = sk_recv(svrfd, ptr, blen, &close);

	/* recv failed */
	if (n < 0) {
		s->is_svrerror = 1;
		_proxy_free_session(ctx, s, CLOSE_SERVER);
		_PROXY_FLOW("ssn[%d] server %d recv error\n", s->id, svrfd);
		return;
	}

	pkt->len += n;
	s->svrq.nbyte += n;

	
	_PROXY_FLOW("ssn[%d] server %d recv %d bytes\n", 
		    s->id, svrfd, n);

	if (pkt->len == 0) {
		s->svrq.pkt = NULL;
		s->nalloced--;
		objpool_put(pkt);
		_PROXY_FLOW("ssn[%d] server %d free packet\n",
			    s->id, svrfd);
	}
	
	/* server close */
	if (close) {
		s->is_svrclose = 1;
		s->svrev.read_func = NULL; 
		_PROXY_FLOW("ssn[%d] server %d closed\n", s->id, svrfd);
	}

	if (_proxy_parse_server(ctx, s, ev)) {
		_proxy_free_session(ctx, s, CLOSE_SERVER);
	}
}

/**
 *	Parse client data
 *
 *	Return 0 if parse success, -1 on error.
 */
static int 
_proxy_parse_client(proxy_t *ctx, session_t *s, ep_event_t *ev)
{
	packet_t *pkt;

	assert(ctx);
	assert(s);

	pkt = s->cliq.pkt;

	if (pkt) {
		s->cliq.pkt = NULL;
		pktq_in(&s->cliq.outq, pkt);
	}

	if (s->svrfd < 0) {
		/* client closed without any data */
		if (s->is_cliclose && s->cliq.nbyte < 1)
			_proxy_free_session(ctx, s, CLOSE_CLIENT);
		else 
			return _proxy_get_server(ctx, s);
	}
	else {
		_proxy_sendto_server(ev);
	}
	return 0;
}

/**
 *	Parse server data.
 *
 *	Return 0 if success, -1 on error.
 */
static int
_proxy_parse_server(proxy_t *ctx, session_t *s, ep_event_t *ev)
{
	packet_t *pkt;

	assert(ctx);
	assert(s);
	
	pkt = s->svrq.pkt;

	if (pkt) {
		s->svrq.pkt = NULL;
		pktq_in(&s->svrq.outq, pkt);
		_PROXY_FLOW("ssn[%d] server %d move packet to output\n", 
			    s->id, s->svrfd);
	}

	if (!s->is_svrclose && s->svrq.nbyte < 1) {
		_proxy_free_session(ctx, s, CLOSE_SERVER);
	}

	_proxy_sendto_client(ev);

	return 0;
}

static void 
_proxy_check_server(ep_event_t *ev)
{
	int ret = 0;
	int fd;
	session_t *s;
	proxy_t *ctx;

	s = ev->data;
	ctx = &g_proxy;

	assert(ctx);
	assert(s);

	fd = ev->fd;

	ret = sk_is_connected(fd);
	if (ret <= 0) {
		_PROXY_FLOW("ssn[%d] server %d connect failed\n", 
			    s->id, fd);
		return;
	}
	s->is_svrwait = 0;
	ctx->nblocked --;

	/* add it to epoll read event */
	ev->read_func = _proxy_recvfrom_server;
	ev->error_func = _proxy_event_error;	
	if (ep_event_add_read(ctx->epctx, ev)) {
		printf("add read failed\n");
		s->is_svrerror = 1;
		_proxy_free_session(ctx, s, CLOSE_SERVER);
	}

	/* delete it from write event */
	ev->write_func = NULL;
	if (ep_event_del_write(ctx->epctx, ev)) {
		printf("del write failed\n");
		s->is_svrerror = 1;
		_proxy_free_session(ctx, s, CLOSE_SERVER);
	}

	_PROXY_FLOW("ssn[%d] client %d connect to server %d success\n", 
		    s->id, s->clifd, fd);

	_proxy_sendto_server(ev);
}

static void 
_proxy_sendto_client(ep_event_t *ev)
{
	proxy_t *ctx;
	session_t *s;
	packet_t *pkt;
	void *ptr;
	int len;
	int n;
	int fd;
	int len1;

	ctx = &g_proxy;
	s = ev->data;

	assert(ctx);
	assert(s);
	assert(s->clifd > -1);

	fd = s->clifd;

	/* server side error, close client side */
	if (s->is_svrerror) {
		_PROXY_ERR("ssn[%d] server error, close client\n", s->id);
		_proxy_free_session(ctx, s, CLOSE_CLIENT);
		return;
	}

	/* server side close, shutdown client side */
	if (s->is_svrclose && s->svrq.nbyte < 1) {
		printf("server close and no bytes need send\n");
		if (s->is_cliclose && s->cliq.nbyte < 1) {			
			_proxy_free_session(ctx, s, CLOSE_CLIENT);
		}
		else {
			shutdown(s->clifd, SHUT_WR);
		}
		return;
	}
	
	/* check block packet first */
	if (s->is_cliblock) {
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

#if 0
		if (len > 100)
			len1 = len - 100;
#endif

		n = sk_send(fd, ptr, len1);
		if (n < 0) {
			_PROXY_FLOW("ssn[%d] server %d sendto client %d failed\n",
				    s->id, s->svrfd, fd);
			s->is_clierror = 1;
			s->nalloced--;
			objpool_put(pkt);
			_proxy_free_session(ctx, s, CLOSE_CLIENT);
			return;
		}
		
		s->svrq.nbyte -= n;
		
		if (n < len) {
			pkt->sendpos += n;

			if (s->is_cliblock)
				break;
			
			s->svrq.blkpkt = pkt;
			s->is_cliblock = 1;
			ctx->nblocked++;
			_PROXY_FLOW("ssn[%d] server %d sendto client %d blocked\n",
				    s->id, s->svrfd, fd);
			
			/* add write event for client */
			ev->write_func = _proxy_sendto_client;
			ev->error_func = _proxy_event_error;
			if (ep_event_add_write(ctx->epctx, ev)) {
				s->is_clierror = 1;
				_proxy_free_session(ctx, s, CLOSE_CLIENT);
			}
			break;
		}		

		s->nalloced--;
		objpool_put(pkt);		

		_PROXY_FLOW("ssn[%d] server %d sendto client %d %d bytes\n", 
			    s->id, s->svrfd, s->clifd, len);
		
		_PROXY_FLOW("ssn[%d] server %d free packet\n", 
			    s->id, s->svrfd);
		
		if (s->is_cliblock) {
			s->is_cliblock = 0;
			ctx->nblocked --;
			ev->write_func = NULL;
			ep_event_del_write(ctx->epctx, ev);
		}

		pkt = pktq_out(&s->svrq.outq);
	}
}

/**
 *	Send the client's data to server.
 *
 *	No return.
 */
static void 
_proxy_sendto_server(ep_event_t *ev)
{
	proxy_t *ctx;
	session_t *s;
	packet_t *pkt;
	int fd;
	void *ptr;
	int len;
	int n;
	int len1;

	ctx = &g_proxy;
	s = ev->data;

	assert(ctx);
	assert(s);
	fd = s->svrfd;
       
	if (s->is_clierror) {
		_proxy_free_session(ctx, s, CLOSE_SERVER);
		return;
	}

	if (s->is_cliclose && s->cliq.nbyte < 1) {
		if (s->is_svrclose && s->svrq.nbyte < 1) {
			_proxy_free_session(ctx, s, CLOSE_BOTH);
		}
		else {
			shutdown(s->svrfd, SHUT_WR);
		}
		return;
	}

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
#if 0
		if (len > 100)
			len1 = len - 100;
#endif

		n = sk_send(fd, ptr, len1);

		/* send error */
		if (n < 0) {
			s->is_svrerror = 1;
			s->nalloced--;
			objpool_put(pkt);
			_PROXY_FLOW("ssn[%d] client %d sendto server %d error\n", 
				    s->id, s->clifd, fd);
			_proxy_free_session(ctx, s, CLOSE_SERVER);
			return;
		}
		
		s->cliq.nbyte -= n;
		
		/* send blocked */
		if (n < len) {	/* send not finished, block client */
			pkt->sendpos = pkt->sendpos + n;			
			if (s->is_svrblock)
				break;

			/* add it to send epoll */
			s->cliq.blkpkt = pkt;
			s->is_svrblock = 1;
			ctx->nblocked++;
			_PROXY_FLOW("ssn[%d] client %d sendto server %d blocked\n", 
				    s->id, s->clifd, fd);
			ev->write_func = _proxy_sendto_server;
			ev->error_func = _proxy_event_error;
			if (ep_event_add_write(ctx->epctx, ev)) {
				s->is_svrerror = 1;
				_proxy_free_session(ctx, s, CLOSE_SERVER);
			}

			break;
		}

		_PROXY_FLOW("ssn[%d] client %d sendto server %d %d bytes\n", 
			    s->id, s->clifd, fd, len);

		/* clear block flags because send success */
		if (s->is_svrblock) {
			s->is_svrblock = 0;
			ctx->nblocked--;
			ev->write_func = NULL;
			ep_event_del_write(ctx->epctx, ev);
		}

		s->nalloced--;
		objpool_put(pkt);
		_PROXY_FLOW("ssn[%d] client %d free packet(%p)\n", 
			    s->id, s->clifd, pkt);

		pkt = pktq_out(&s->cliq.outq);
	}
}


/**
 *	The main loop of proxy, it use epoll to detect which socket
 *	need process
 *
 *	Return 0 if success, -1 on error.
 */
static int
_proxy_loop(proxy_t *ctx)
{
	int n;

	assert(ctx);

	

	while (!g_stop) {
		n = ep_event_loop(ctx->epctx, 100);
		if (n < 0)
			break;
	}

	return 0;
}

/**
 *	The main entry of proxy, the proxy is listen at @ip:@port, the
 *	real server is at @rip:@rport, and the max concurrency connections 
 *	is @capacity, and proxy will exit after process @maxconns client 
 *	connections.
 *
 *	return 0 if success, -1 on error.
 */
int 
proxy_main(ip_port_t *svraddr, ip_port_t *rsaddrs, int nrsaddr, 
	   int max, int use_splice, int use_nb_splice)
{
	proxy_t *ctx;
	char buf[512] = {0};
	int i;

	if (!svraddr || !rsaddrs || nrsaddr < 1 || max < 1) {
		_PROXY_ERR("invalid arguments\n");
		return -1;
	}

	ctx = &g_proxy;
	memset(ctx, 0, sizeof(ctx));
	ctx->svraddr = *svraddr;
	memcpy(ctx->rsaddrs, rsaddrs, nrsaddr * sizeof(ip_port_t));
	ctx->nrsaddr = nrsaddr;
	ctx->capacity = max;

	if (_proxy_initiate(ctx)) {
		_proxy_release(ctx);
		return -1;
	}

	_PROXY_FLOW("start proxy %s, capacity %d\n", 
		    ip_port_to_str(svraddr, buf, sizeof(buf)), max);
	_PROXY_FLOW("real servers(%d):\n", nrsaddr);
	for (i = 0; i < nrsaddr; i++) {
		_PROXY_FLOW("\t%d: %s\n", i, 
			    ip_port_to_str(&rsaddrs[i], buf, sizeof(buf)));
	}

	_proxy_loop(ctx);

	_proxy_release(ctx);

	return 0;
}


