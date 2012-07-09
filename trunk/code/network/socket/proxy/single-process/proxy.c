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
#include "sock.h"
#include "http_protocol.h"
#include "mempool.h"

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

static int _g_stop = 0;

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

/**
 *	The signal handler of SIGINT, it set global variable @_g_stop, then stop
 *	the proxy.
 *
 *	No return.
 */
static void 
_proxy_stop(int signo)
{
	if (signo == SIGINT) {
		_PROXY_FLOW("proxy recved stop signal\n");
		_g_stop = 1;
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
	ctx->listenfd = sock_tcpsvr(ctx->ip, htons(ctx->port));
	if (ctx->listenfd < 0) {
		_PROXY_ERR("create listen fd error\n");
		return -1;
	}
	sock_set_nbio(ctx->listenfd, 1);
	_PROXY_FLOW("create listen fd %d\n", ctx->listenfd);

	/* create recv epoll socket */
	ctx->recv_epfd = epoll_create(ctx->capacity);
	if (ctx->recv_epfd < 0) {
		_PROXY_ERR("create epoll fd error\n");
		return -1;
	}
	_PROXY_FLOW("create recv epoll fd %d\n", ctx->recv_epfd);

	/* create send epoll socket */
	ctx->send_epfd = epoll_create(ctx->capacity);
	if (ctx->send_epfd < 0) {
		_PROXY_ERR("create send epoll fd error\n");
		return -1;
	}
	_PROXY_FLOW("create send epoll fd %d\n", ctx->send_epfd);

	/* malloc memory for epoll event */
	ctx->events = malloc(ctx->capacity * sizeof(struct epoll_event));
	if (!ctx->events) {
		_PROXY_ERR("malloc memory for epoll event error: %s\n",
			   strerror(errno));
		return -1;
	}
	_PROXY_FLOW("create epoll events %p\n", ctx->events);

	/* create memory pool */
	ctx->pktpool = mempool_alloc(168, 1, 0);
	if (!ctx->pktpool) {
		_PROXY_ERR("create mempool for packet error\n");
		return -1;
	}
	_PROXY_FLOW("create memory pool for packet %p\n", ctx->pktpool);
	
	ctx->ssnpool = mempool_alloc(sizeof(session_t), 1, 0);
	if (!ctx->ssnpool) {
		_PROXY_ERR("create mempool for session error\n");
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
		rlim.rlim_cur = 2 * ctx->capacity;
		rlim.rlim_max = 2 * ctx->capacity;
		
		if (setrlimit(RLIMIT_NOFILE, &rlim)) {
			_PROXY_ERR("setrlimit(RLIMIT_NOFILE) error: %s",
				   strerror(errno));
			return -1;
		}
		
		_PROXY_FLOW("set max file descriptor to %d\n", 
			    (2 * ctx->capacity));
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

	if (ctx-> recv_epfd >= 0)
		close(ctx-> recv_epfd);

	if (ctx-> send_epfd >= 0)
		close(ctx-> send_epfd);

	if (ctx->events)
		free(ctx->events);

	if (ctx->pktpool)
		mempool_free(ctx->pktpool);

	if (ctx->ssnpool)
		mempool_free(ctx->ssnpool);
	
	if (ctx->sessions)
		session_table_free(ctx->sessions);

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

	s = mempool_get(ctx->ssnpool);
	if (!s) {
		_PROXY_ERR("get session from ssnpool failed\n");
		return NULL;
	}

	memset(s, 0, sizeof(session_t));

	if (session_table_add(ctx->sessions, s)) {
		_PROXY_ERR("add session to session table failed\n");
		mempool_put(s);
		return NULL;
	}

	s->svrfd = -1;

	return s;
}


static void 
_proxy_clean_session(proxy_t *ctx, session_t *s, int issvr)
{
	packet_t *pkt;
	sesscache_t *cache;
	
	assert(ctx);
	assert(s);
	
	if (issvr)
		cache = &s->svrcache;
	else
		cache = &s->clicache;

	pkt = cache->pkt;
	if (pkt) {
		s->nalloced--;
		_PROXY_FLOW("1 ssn[%d] free a packet %p\n", s->id, pkt);
		mempool_put(pkt);
	}

	pkt = cache->blkpkt;
	if (pkt) {
		s->nalloced--;
		_PROXY_FLOW("2 ssn[%d] free a packet %p\n", s->id, pkt);
		mempool_put(pkt);
	}

	pkt = pktq_out(&cache->input);
	while(pkt) {
		s->nalloced--;
		_PROXY_FLOW("3 ssn[%d] free a packet %p\n", s->id, pkt);
		mempool_put(pkt);
		pkt = pktq_out(&cache->input);
	}

	pkt = pktq_out(&cache->output);
	while (pkt) {
		s->nalloced--;
		_PROXY_FLOW("4 ssn[%d] free a packet %p\n", s->id, pkt);
		mempool_put(pkt);
		pkt = pktq_out(&cache->output);
	}
}


/**
 *	Free session @s, release all resource used by @s.
 *
 *	No return.
 */
static void 
_proxy_free_session(proxy_t *ctx, session_t *s)
{
	assert(ctx);
	assert(s);
	int fd;
	int epfd;

	/* close client socket */
	if (s->clifd > -1) {
		fd = s->clifd;

		epfd = ctx->send_epfd;
		if (s->is_cliblock && epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL))
			_PROXY_ERR("del client %d error %s\n", 
				    fd, strerror(errno));

		epfd = ctx->recv_epfd;
		if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL)) {
			_PROXY_ERR("del client %d from error: %s\n",
				   fd, strerror(errno));
		}
		
		_PROXY_FLOW("ssn[%d] client %d closed\n", s->id, fd);
		close(s->clifd);
	}

	/* close server socket */
	if (s->svrfd > -1) {
		fd = s->svrfd;

		epfd = ctx->send_epfd;
		if (s->is_svrwait) {
			if (epoll_ctl(ctx->send_epfd, EPOLL_CTL_DEL, fd, NULL))
				_PROXY_ERR("del server %d error %s\n", 
					   fd, strerror(errno));
		}
		else {
			if (s->is_svrblock && epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL))
				_PROXY_ERR("del server %d error %s\n", 
					   fd, strerror(errno));

			epfd = ctx->recv_epfd;
			if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL))
				_PROXY_ERR("del server %d error %s\n", 
					   fd, strerror(errno));
		}

		_PROXY_FLOW("ssn[%d] server %d closed\n", s->id, fd);
		close(s->svrfd);
	}

	_proxy_clean_session(ctx, s, 0);

	_proxy_clean_session(ctx, s, 1);

	assert(s->nalloced == 0);

	/* delete session from session pool */
	_PROXY_FLOW("ssn[%d] deleted\n", s->id);
	session_table_del(ctx->sessions, s->id);

	ctx->stat.nconcurrency--;
}




/**
 *	Accept client TCP connection, and add it to epoll socket and
 *	session pool.
 *
 *	Return 0 if accept success, -1 on error.
 */
static int
_proxy_accept_client(proxy_t *ctx)
{
	int clifd;
	struct sockaddr_in cliaddr;
	socklen_t alen;
	struct epoll_event event;
	session_t *s;
	int *pair;

	assert(ctx);
	
	alen = sizeof(cliaddr);

	clifd = accept(ctx->listenfd, (struct sockaddr *)&cliaddr, &alen);
	if (clifd < 0) {
		if (errno == EAGAIN)
			return 0;
		if (errno == EINTR)
			return 0;

		_PROXY_ERR("accept error: %s\n", strerror(errno));
		return -1;
	}

	/* add client socket to session pool */
	s = _proxy_alloc_session(ctx);
	if (!s) {
		close(clifd);
		return -1;
	}

	/* add client socket to epoll socket */
	event.events = EPOLLIN;
	pair = (int *)&event.data.u64;
	pair[0] = clifd;
	pair[1] = s->id;
	if (epoll_ctl(ctx->recv_epfd, EPOLL_CTL_ADD, clifd, &event)) {
		_PROXY_ERR("epoll_ctl(EPOLL_CTL_ADD) error: %s\n",
			   strerror(errno));
		close(clifd);
		_proxy_free_session(ctx, s);
		return -1;
	}

	s->clifd = clifd;	
	ctx->stat.nconcurrency++;

	_PROXY_FLOW("ssn[%d] client %d accept, %u.%u.%u.%u:%u\n", 
		    s->id, clifd, NIPQUAD(cliaddr.sin_addr.s_addr),
		    ntohs(cliaddr.sin_port));

	return 0;
}

static int 
_proxy_recvfrom_client(proxy_t *ctx, session_t *s)
{
	int clifd;
	packet_t *pkt;
	int n;
	int blen = 0;
	int close = 0;

	assert(ctx);
	assert(s);
	assert(s->clifd >= 0);
	
	clifd = s->clifd;

	/* get packet to restore recved data */
	pkt = s->clicache.pkt;
	if (!pkt) {
		pkt = mempool_get(ctx->pktpool);
		if (!pkt) {
			_PROXY_ERR("can't get packet from memory pool\n");
			return -1;
		}
		s->nalloced++;
		s->clicache.pkt = pkt;
		pkt->len = 0;
		pkt->sendpos = 0;
		pkt->pos = 0;
		pkt->capacity = ctx->pktpool->objsize - sizeof(packet_t);
		_PROXY_FLOW("ssn[%d] client %d get a packet(%p)\n", 
			    s->id, s->clifd, pkt);
	}
	
	blen = pkt->capacity - pkt->len;
	assert(blen > 0);
	n = sock_recv(clifd, pkt->data, blen, &close);
	if (n < 0) {
		s->is_clierror = 1;
		_PROXY_FLOW("ssn[%d] client %d recv error\n", s->id, clifd);
		return -1;
	}

	if (close) {
		s->is_cliclose = 1;
		_PROXY_FLOW("ssn[%d] client %d recv closed\n", s->id, clifd);
	}

	pkt->len += n;

	_PROXY_FLOW("ssn[%d] client %d recv %d bytes\n", s->id, clifd, n);

	if (pkt->len == 0) {
		s->clicache.pkt = NULL;
		s->nalloced--;
		mempool_put(pkt);

		_PROXY_FLOW("ssn[%d] client %d free packet %p\n",
			    s->id, clifd, pkt);
	}

	return 0;
}

static int 
_proxy_recvfrom_server(proxy_t *ctx, session_t *s)
{
	int svrfd;
	packet_t *pkt;
	int n;
	int blen = 0;
	char *ptr;
	int close = 0;

	assert(ctx);
	assert(s);
	assert(s->svrfd >= 0);
	
	svrfd = s->svrfd;
	pkt = s->svrcache.pkt;

	/* get a new packet */
	if (!pkt) {
		pkt = mempool_get(ctx->pktpool);
		if (!pkt) {
			_PROXY_ERR("can't get packet from memory pool\n");
			return -1;
		}
		s->nalloced++;
		s->svrcache.pkt = pkt;
		pkt->len = 0;
		pkt->recvpos = 0;
		pkt->sendpos = 0;
		pkt->pos = 0;
		pkt->capacity = ctx->pktpool->objsize - sizeof(packet_t);
		_PROXY_FLOW("ssn[%d] server %d get a packet(%p)\n", 
			   s->id, svrfd, pkt);
	}
	
	/* recv data */
	ptr = pkt->data + pkt->recvpos;
	blen = pkt->capacity - pkt->recvpos;
	assert(blen > 0);
	n = sock_recv(svrfd, ptr, blen, &close);
	if (n < 0) {
		s->is_svrerror = 1;
		_PROXY_FLOW("ssn[%d] server %d recv error\n", s->id, svrfd);
		return -1;
	}
	else if (close) {
		s->is_svrclose = 1;
		_PROXY_FLOW("ssn[%d] server %d closed\n", s->id, svrfd);
	}

	pkt->len += n;
	if (pkt->len == 0) {
		s->svrcache.pkt = NULL;
		s->nalloced--;
		mempool_put(pkt);
		_PROXY_FLOW("ssn[%d] server %d free packet\n",
			    s->id, svrfd);
	}

	_PROXY_FLOW("ssn[%d] server %d recv %d bytes\n", 
		    s->id, svrfd, n);

	return 0;
}


/**
 *	Parse client data
 *
 *	Return 0 if parse success, -1 on error.
 */
static int 
_proxy_parse_client(proxy_t *ctx, session_t *s)
{
	packet_t *pkt;
	int len;
	void *ptr;

	assert(ctx);
	assert(s);

	/* client is closed */
	if (!s->clicache.pkt)
		return 0;

	pkt = s->clicache.pkt;
	ptr = pkt->data + pkt->recvpos;
	len = pkt->len - pkt->recvpos;

#if 0
	/* do HTTP parse */
	assert(len >= 0);
	if (len > 0) {
		if (http_parse_request(&s->clihttp, ptr, len, 0)) {
			_PROXY_ERR("http request parse error\n");
			return -1;
		}
	}
	/* client is closed */
	else {
		if (s->clihttp.state != HTTP_STE_FIN) {
			_PROXY_ERR("http request is not finished before closed\n");
			return -1;
		}
	}

	_PROXY_FLOW("session[%d] client %d http parse success\n", 
		    s->id, s->clifd);
	
	/* move packet to output queue */
	if (pkt->len == pkt->capacity ||
	    s->clihttp.state == HTTP_STE_FIN) 
	{
		s->clicache.pkt = NULL;
		pktqueue_in(&s->clicache.output, pkt);

		_PROXY_FLOW("session[%d] client %d move current packet to output\n", 
			    s->id, s->clifd);
	}
#endif 
	s->clicache.pkt = NULL;
	pktq_in(&s->clicache.output, pkt);

	return 0;
}

/**
 *	Parse server data.
 *
 *	Return 0 if success, -1 on error.
 */
static int
_proxy_parse_server(proxy_t *ctx, session_t *s)
{
	packet_t *pkt;
	int len;
	void *ptr;

	assert(ctx);
	assert(s);
	
	/* server is closed */
	if (!s->svrcache.pkt)
		return 0;

	pkt = s->svrcache.pkt;
	ptr = pkt->data + pkt->recvpos;
	len = pkt->len - pkt->recvpos;

#if 0
	if (len > 0) {
		if (http_parse_response(&s->svrhttp, ptr, len, 0)) {
			s->flags |= SESSION_SVRERROR;
			_PROXY_ERR("http response parse error\n");
			return -1;
		}
	}
	/* server is closed */
	else {
		if (s->svrhttp.state != HTTP_STE_FIN) {
			_PROXY_ERR("http response not fin before closed\n");
			return -1;
		}
	}
	
	_PROXY_FLOW("ssn %d server %d http parse success\n", 
		    s->id, s->svrfd);
	
	/* move packet to output queue for send */
	if (pkt->len == pkt->capacity ||
	    s->svrhttp.state == HTTP_STE_FIN) 
	{
		s->svrcache.current = NULL;
		pktqueue_in(&s->svrcache.output, pkt);

		_PROXY_FLOW("ssn %d server %d move current packet to output\n", 
			    s->id, s->svrfd);	
	}

#endif
	s->svrcache.pkt = NULL;
	pktq_in(&s->svrcache.output, pkt);

	_PROXY_FLOW("ssn[%d] server %d move packet to output\n", 
		    s->id, s->svrfd);

	return 0;
}



static int 
_proxy_sendto_client(proxy_t *ctx, session_t *s)
{
	packet_t *pkt;
	void *ptr;
	int *pair;
	struct epoll_event event;
	int len;
	int n;
	int fd;
	int len1;

	assert(ctx);
	assert(s);
	assert(s->clifd > -1);

	fd = s->clifd;

	/* check block packet first */
	if (s->is_cliblock) {
		pkt = s->svrcache.blkpkt;
		s->svrcache.blkpkt = NULL;
	}
	else {
		pkt = pktq_out(&s->svrcache.output);
	}

	while (pkt) {
		ptr = pkt->data + pkt->sendpos;
		len = pkt->len - pkt->sendpos;
		if (len > 100)
			len1 = len - 100;
		else 
			len1 = len;
		n = sock_send(fd, ptr, len1);
		if (n < 0) {
			_PROXY_FLOW("ssn[%d] server %d sendto client %d failed\n", 
				    s->id, s->svrfd, fd);
			s->is_clierror = 1;
			s->nalloced--;
			mempool_put(pkt);
			return -1;
		}
		else if (n < len) {
			pkt->sendpos += n;

			if (s->is_cliblock)
				break;
			
			/* add it to send epoll and set block */
			memset(&event, 0, sizeof(event));
			event.events = EPOLLOUT;
			pair = (int *)&event.data.u64;
			pair[0] = fd;
			pair[1] = s->id;
			if (epoll_ctl(ctx->send_epfd, EPOLL_CTL_ADD, fd, &event)) {
				_PROXY_ERR("add client %d to send epoll error: %s\n", 
					   fd, strerror(errno));
				s->nalloced--;
				mempool_put(pkt);
				return -1;
			}
			s->svrcache.blkpkt = pkt;
			s->is_cliblock = 1;
			ctx->nblocked++;
			_PROXY_FLOW("ssn[%d] server %d send to client %d %d:%d blocked\n",
				    s->id, s->svrfd, fd, len, n);
			break;
		}

		if (s->is_cliblock) {
			if (epoll_ctl(ctx->send_epfd, EPOLL_CTL_DEL, fd, NULL)) {
				_PROXY_ERR("Del client %d from send epoll error\n", fd);
				s->nalloced--;
				mempool_put(pkt);
				return -1;
			}
			s->is_cliblock = 0;
			ctx->nblocked--;
		}

		s->nalloced--;
		mempool_put(pkt);

		_PROXY_FLOW("ssn[%d] server %d sendto client %d %d bytes\n", 
			    s->id, s->svrfd, s->clifd, len);
		
		_PROXY_FLOW("ssn[%d] server %d free packet\n", 
			    s->id, s->svrfd);
		
		pkt = pktq_out(&s->svrcache.output);
	}
	
	return 0;

}


static int 
_proxy_get_server(proxy_t *ctx, session_t *s)
{
	int fd;
	int wait;
	struct epoll_event event;
	int *pair;

	if (s->svrfd > 0)
		return 0;

	fd = sock_tcpcli_nb(ctx->rip, htons(ctx->rport), &wait);
	if (fd < 0) {
		s->is_svrerror = 1;
		_PROXY_ERR("get server socket error\n");
		return -1;
	}

	/* add it to send epoll to wait connect success */
	if (wait) {
		pair = (int *)&event.data.u64;	
		pair[0] = fd;
		pair[1] = s->id;
		event.events = EPOLLOUT;			
		if (epoll_ctl(ctx->send_epfd, EPOLL_CTL_ADD, fd, &event)) {
			s->is_svrerror = 1;
			_PROXY_ERR("add server fd %d to send epoll failed\n", fd);
			close(fd);
			return -1;
		}
		ctx->nblocked ++;
		s->is_svrwait = 1;
		_PROXY_FLOW("ssn[%d] client %d connect to server %d wait\n", 
			    s->id, s->clifd, fd);
	}
	/* add to recv epoll */
	else {
		pair = (int *)&event.data.u64;	
		pair[0] = fd;
		pair[1] = s->id;
		event.events = EPOLLIN;			
		if (epoll_ctl(ctx->recv_epfd, EPOLL_CTL_ADD, fd, &event)) {
			s->is_svrerror = 1;
			_PROXY_ERR("add server %d to epoll %d error: %s\n",
				   fd, ctx->recv_epfd, strerror(errno));
			close(fd);
			return -1;
		}
		_PROXY_FLOW("ssn[%d] client %d connect to server %d success\n", 
			    s->id, s->clifd, fd);

	}

	s->svrfd = fd;

	return 0;
}


static int 
_proxy_sendto_server(proxy_t *ctx, session_t *s)
{
	packet_t *pkt;
	int fd;
	void *ptr;
	int len;
	int n;
	struct epoll_event event;
	int *pair;
	int len1;

	assert(ctx);
	assert(s);

	if (s->clicache.output.len < 1 && ! s->clicache.blkpkt)
		return 0;

	if (_proxy_get_server(ctx, s)) 
		return -1;

	if (s->is_svrwait) {
		return 0;
	}

	fd = s->svrfd;

	/* send block packet first */
	if (s->clicache.blkpkt) {
		pkt = s->clicache.blkpkt;
		s->clicache.blkpkt = NULL;
	}
	else {
		pkt = pktq_out(&s->clicache.output);
	}
	while (pkt) {

		/* send packet to server */
		ptr = pkt->data + pkt->sendpos;
		len = pkt->len - pkt->sendpos;
		if (len > 100)
			len1 = len - 100;
		else
			len1 = len;
		n = sock_send(fd, ptr, len1);
		if (n <= 0) {			/* send error */
			s->is_svrerror = 1;
			s->nalloced--;
			mempool_put(pkt);
			_PROXY_FLOW("ssn[%d] client %d sendto server %d error\n", 
				    s->id, s->clifd, fd);
			return -1;
		}
		else if (n < len) {		/* send not finished, block client */
			pkt->sendpos = pkt->sendpos + n;
			
			if (s->is_svrblock)
				break;

			/* add it to send epoll */
			pair = (int *)&event.data.u64;
			pair[0] = fd;
			pair[1] = s->id;
			event.events = EPOLLOUT;			
			if (epoll_ctl(ctx->send_epfd, EPOLL_CTL_ADD, fd, &event)) {
				s->is_svrerror = 1;
				s->nalloced--;
				mempool_put(pkt);
				_PROXY_FLOW("ssn[%d] client %d free a packet %p\n", 
					     s->id, s->clifd, pkt);
				_PROXY_ERR("add server fd %d to send epoll failed %s\n", 
					   fd, strerror(errno));
				return -1;
			}
			s->clicache.blkpkt = pkt;
			s->is_svrblock = 1;
			ctx->nblocked++;
			_PROXY_FLOW("ssn[%d] client %d sendto server %d %d:%d blocked\n", 
				    s->id, s->clifd, fd, len, n);
			break;
		}

		_PROXY_FLOW("ssn[%d] client %d sendto server %d %d bytes\n", 
			    s->id, s->clifd, fd, len);

		/* clear block flags because send success */
		if (s->is_svrblock) {
			if (epoll_ctl(ctx->send_epfd, EPOLL_CTL_DEL, fd, NULL)) {
				s->is_svrerror = 1;
				_PROXY_ERR("del server fd %d from send epoll failed\n", 
					   fd);
				s->nalloced--;
				mempool_put(pkt);
				return -1;
			}
			s->is_svrblock = 0;
			ctx->nblocked--;
		}

		s->nalloced--;
		mempool_put(pkt);

		pkt = pktq_out(&s->clicache.output);
	}

	return 0;
}


/**
 *	Process client request, include recv client data, parse client data, and send
 *	client data to server 
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_proxy_process_client(proxy_t *ctx, session_t *s)
{
	assert(ctx);
	assert(s);
	assert(s->clifd >= 0);

	if (_proxy_recvfrom_client(ctx, s)) {
		_proxy_free_session(ctx, s);
		return -1;
	}
       
	if (_proxy_parse_client(ctx, s)) {
		_proxy_free_session(ctx, s);
		return -1;
	}
		
	if (_proxy_sendto_server(ctx, s)) {
		printf("send to server failed\n");
		_proxy_free_session(ctx, s);
		return -1;
	}

	if (s->is_clierror || s->is_cliclose) {
		_proxy_free_session(ctx, s);
		return -1;
	}

	return 0;
}



/**
 *	Process server data, include recv data data, parse server data, and
 *	send server data to client.
 *
 *	Return 0 if success, -1 on error.
 */
static int
_proxy_process_server(proxy_t *ctx, session_t *s)
{
	assert(ctx);
	assert(s);
	assert(s->svrfd > -1);

	if (_proxy_recvfrom_server(ctx, s)) {
		_proxy_free_session(ctx, s);
		return -1;
	}

	if (_proxy_parse_server(ctx, s)) {
		_proxy_free_session(ctx, s);
		return -1;
	}

	if (_proxy_sendto_client(ctx, s)) {
		_proxy_free_session(ctx, s);
		return -1;
	}

	if (s->is_svrerror || s->is_svrclose) {
		_proxy_free_session(ctx, s);
	}

	return 0;
}


/**
 *	Recv data from socket 
 *
 *	Return 0 if success, -1 on error.
 */
static void 
_proxy_recv_data(proxy_t *ctx)
{
	session_t *s;
	int *pair;
	int fd;
	int id;
	int nfds;
	int i;

	assert(ctx);

	nfds = epoll_wait(ctx->recv_epfd, ctx->events, 
			  ctx->capacity, PROXY_TIMEOUT);
	if (nfds < 0) {
		if (errno == EINTR)
			return;
		_PROXY_ERR("epoll_wait error: %s", strerror(errno));
		return;
	}

	if (nfds == 0)
		return;

	for (i = 0; i < nfds; i++) {
			
		/* only accept when exist connections not 
		 * exceed proxy's capacity 
		 */
		if (ctx->events[i].data.fd == ctx->listenfd) {
			if (ctx->stat.nconcurrency < ctx->capacity) {
				_proxy_accept_client(ctx);
			}
			else {
				_PROXY_FLOW("too many connection %lu, capacity %u\n",
					   ctx->stat.nconcurrency, ctx->capacity);
			}
		}
		/* recv data */
		else {
			pair = (int *)&ctx->events[i].data.u64;
			fd = pair[0];
			id = pair[1];
			s = session_table_find(ctx->sessions, id);
			if (!s) {
				_PROXY_ERR("can't find sesion %d\n", id);
				if (epoll_ctl(ctx->recv_epfd, EPOLL_CTL_DEL, fd, NULL)) {
					_PROXY_ERR("del %d from recv epoll error\n", fd);
				}
				close(fd);
				continue;
			}

			if (s->clifd == fd) {
				if (!s->is_svrblock)
					_proxy_process_client(ctx, s);
			}
			else if (s->svrfd == fd) {
				if (!s->is_cliblock)
					_proxy_process_server(ctx, s);
			}
			else {
				_PROXY_ERR("the fd %d is in session %d", fd, id);
				if (epoll_ctl(ctx->recv_epfd, EPOLL_CTL_DEL, fd, NULL)) {
					_PROXY_ERR("delete %d from recv epoll failed\n", fd);
				}
				close(fd);
			}
		}
	}
}


static int 
_proxy_check_server(proxy_t *ctx, session_t *s)
{
	int ret = 0;
	int fd;
	int *pair;
	struct epoll_event event;

	assert(ctx);
	assert(s);

	fd = s->svrfd;

	ret = sock_is_connected(fd);
	if (ret <= 0) {
		_PROXY_FLOW("ssn[%d] server %d connect failed\n", 
			    s->id, fd);
		return -1;
	}
	s->is_svrwait = 0;
	ctx->nblocked --;

	/* remove it from socket */
	if (epoll_ctl(ctx->send_epfd, EPOLL_CTL_DEL, fd, NULL)) {
		_PROXY_ERR("del server fd %d from send epoll error: %s\n",
			    fd, strerror(errno));
		return -1;
	}

	/* add to recv epoll */
	pair = (int *)&event.data.u64;
	pair[0] = fd;
	pair[1] = s->id;
	event.events = EPOLLIN;
	if (epoll_ctl(ctx->recv_epfd, EPOLL_CTL_ADD, fd, &event)) {
		_PROXY_ERR("add server fd %d to recv epoll error: %s\n",
			   fd, strerror(errno));
		return -1;
	}

	_PROXY_FLOW("ssn[%d] client %d connect to server %d success\n", 
		    s->id, s->clifd, fd);

	return 0;
}


static void  
_proxy_send_data(proxy_t *ctx) 
{
	session_t *s;
	int *pair;
	int fd;
	int id;
	int nfds;
	int i;

	assert(ctx);

	assert(ctx->nblocked >= 0);

	if (ctx->nblocked < 1)
		return;

	nfds = epoll_wait(ctx->send_epfd, ctx->events, 
			  ctx->capacity, PROXY_TIMEOUT);
	if (nfds < 0) {
		if (errno == EINTR)
			return;
		_PROXY_ERR("epoll_wait error: %s", strerror(errno));
		return;
	}

	if (nfds == 0)
		return;

	for (i = 0; i < nfds; i++) {
			
		/* recv data */
		pair = (int *)&ctx->events[i].data.u64;
		fd = pair[0];
		id = pair[1];
		s = session_table_find(ctx->sessions, id);
		if (!s) {
			_PROXY_ERR("can't find sesion %d\n", id);
			if (epoll_ctl(ctx->send_epfd, EPOLL_CTL_DEL, fd, NULL)) {
				_PROXY_ERR("del fd %d from send epoll failed\n", fd);
			}

			close(fd);
			continue;
		}

		if (s->clifd == fd) {
			if (_proxy_sendto_client(ctx, s))
				_proxy_free_session(ctx, s);
		}
		else if (s->svrfd == fd) {
			if (s->is_svrwait && _proxy_check_server(ctx, s))
				_proxy_free_session(ctx, s);

			if (_proxy_sendto_server(ctx, s)) 
				_proxy_free_session(ctx, s);
		}
		else {
			_PROXY_ERR("the fd %d is in session %d", fd, id);
			if (epoll_ctl(ctx->send_epfd, EPOLL_CTL_DEL, fd, NULL)) {
				_PROXY_ERR("del fd %d from send epoll failed\n", fd);
			}
			close(fd);
		}
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
	struct epoll_event event;
	int fd;

	assert(ctx);

	/* add listen fd into epoll event */
	memset(&event, 0, sizeof(event));
	event.data.fd = ctx->listenfd;
	event.events = EPOLLIN;
	fd = ctx->listenfd;
	if (epoll_ctl(ctx->recv_epfd, EPOLL_CTL_ADD, fd, &event)) {
		_PROXY_ERR("add listen fd %d to epoll fd %d error: %s\n",
			   fd, ctx->recv_epfd, strerror(errno));
		return -1;
	}

	/* we only process @ctx->maxconns client connection */
	while (!_g_stop) {
		_proxy_send_data(ctx);
		_proxy_recv_data(ctx);
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
proxy_main(u_int32_t ip, u_int16_t port, 
	   u_int32_t rip, u_int16_t rport, 
	   int capacity)
{
	proxy_t ctx;

	if (!ip || !port || !rip || !rport || capacity < 1)
		return -1;
	
	memset(&ctx, 0, sizeof(ctx));
	ctx.ip = ip;
	ctx.rip = rip;
	ctx.port = port;
	ctx.rport = rport;
	ctx.capacity = capacity;

	if (_proxy_initiate(&ctx)) {
		_proxy_release(&ctx);
		return -1;
	}

	_PROXY_FLOW("start proxy %u.%u.%u.%u:%u, real server %u.%u.%u.%u:%u, capacity %d\n",
		    NIPQUAD(ctx.ip), ctx.port, NIPQUAD(ctx.rip), ctx.rport, ctx.capacity);

	_proxy_loop(&ctx);

	_proxy_release(&ctx);

	return 0;
}


