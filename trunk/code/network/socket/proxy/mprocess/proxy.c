/**
 *	file	proxy.c
 *
 *	brief	The proxy main APIs, it accept connections, recv data, 
 *		parse data and send data.
 *
 *	author	Forrest.zhang
 */

#define _GNU_SOURCE

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
#include <fcntl.h>

#include "proxy.h"
#include "sock_util.h"
#include "objpool.h"
#include "nb_splice.h"

/**
 *	Define MACROs for print error message and debug message.
 */

//#define _PROXY_DEBUG	1

#ifdef	_PROXY_DEBUG
#define	DBG(fmt, args...)	printf("proxyd[%d]: "fmt, _g_index, ##args)
#define FLOW(fmt, args...)	printf("proxyd[%d]: "fmt, _g_index, ##args)
#else
#define	DBG(fmt, args...)
#define FLOW(fmt, args...)
#endif

#define	ERR(fmt, args...)	fprintf(stderr, "%s:%d: "fmt,	\
					__FILE__, __LINE__, ##args)
#define ERRSTR			strerror(errno)

static int _g_stop = 0;
static int _g_index = 0;

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
	if (signo != SIGUSR1)
		return;

	DBG("recved stop signal\n");
	_g_stop = 1;
}

/**
 *	Initiate a proxy_t struct @ctx. Create epoll socket and 
 *	listen socket. and alloc some resources
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_proxy_init(proxy_t *ctx)
{
	struct rlimit rlim;

	assert(ctx);
	
	/* create recv epoll socket */
	ctx->recv_epfd = epoll_create(ctx->maxsess);
	if (ctx->recv_epfd < 0) {
		ERR("create epoll fd error\n");
		return -1;
	}
	DBG("create recv epoll fd %d\n", ctx->recv_epfd);

	/* create send epoll socket */
	ctx->send_epfd = epoll_create(ctx->maxsess);
	if (ctx->send_epfd < 0) {
		ERR("create send epoll fd error\n");
		return -1;
	}
	DBG("create send epoll fd %d\n", ctx->send_epfd);

	/* malloc memory for epoll event */
	ctx->events = malloc(ctx->maxsess * sizeof(struct epoll_event));
	if (!ctx->events) {
		ERR("malloc memory for epoll event error %s\n", ERRSTR);
		return -1;
	}
	DBG("create epoll events %p\n", ctx->events);

	/* create memory pool */
	ctx->pktpool = objpool_alloc(4096, 1, 0);
	if (!ctx->pktpool) {
		ERR("create objpool for packet error\n");
		return -1;
	}
	DBG("create memory pool for packet %p\n", ctx->pktpool);
	
	ctx->ssnpool = objpool_alloc(sizeof(session_t), 1, 0);
	if (!ctx->ssnpool) {
		ERR("create objpool for session error\n");
		return -1;
	}
	DBG("create memory pool for session %p\n", ctx->ssnpool);
	
	/* create session table */
	ctx->sessions = session_table_alloc(ctx->maxsess);
	if (!ctx->sessions) {
		ERR("create session table error\n");
		return -1;
	}
	DBG("create session table %p\n", ctx->sessions);

	/* set max file descriptor to 2 * maxsess if maxsess > 512 */
	if (ctx->maxsess > 256) {
		rlim.rlim_cur = 4 * ctx->maxsess + 10;
		rlim.rlim_max = 4 * ctx->maxsess + 10;
		
		if (setrlimit(RLIMIT_NOFILE, &rlim)) {
			ERR("setrlimit(RLIMIT_NOFILE) error: %s", ERRSTR);
			return -1;
		}
		
		DBG("set max file descriptor to %d\n", 
			    (2 * ctx->maxsess));
	}

	/* nb_splice */
	if (ctx->use_nb_splice) {
		ctx->splice_fd = nb_splice_init();
		if (ctx->splice_fd < 0) {
			ERR("nb splice init failed\n");
			return -1;
		}
	}

	/* set SIGUSR1 as quit signal*/
	signal(SIGUSR1, _proxy_stop);
	signal(SIGINT, SIG_IGN);

	return 0;
}

/**
 *	Free resource alloced by _proxy_initiate().
 *
 *	No return.
 */
static void 
_proxy_free(proxy_t *ctx)
{
	assert(ctx);

	if (ctx->httpfd >= 0)
		close(ctx->httpfd);

	if (ctx-> recv_epfd >= 0)
		close(ctx-> recv_epfd);

	if (ctx-> send_epfd >= 0)
		close(ctx-> send_epfd);

	if (ctx->events)
		free(ctx->events);

	if (ctx->pktpool)
		objpool_free(ctx->pktpool);

	if (ctx->ssnpool)
		objpool_free(ctx->ssnpool);
	
	if (ctx->sessions)
		session_table_free(ctx->sessions);

	if (ctx->splice_fd >= 0)
		close(ctx->splice_fd);

	DBG("all resource is freed\n");
}

/**
 *	Get a packet from packet pool and put it to session queue @q
 *
 */
static packet_t * 
_proxy_get_packet(proxy_t *ctx, session_t *s, session_queue_t *q)
{
	packet_t *pkt;

	assert(ctx);
	assert(s);
	assert(q);

	/* get packet to restore recved data */
	pkt = s->cliq.pkt;
	if (pkt) {
		return pkt;
	}
	
	pkt = objpool_get(ctx->pktpool);
	if (!pkt) {
		ERR("can't get packet from memory pool\n");
		return NULL;
	}
	s->nalloced++;
	q->pkt = pkt;
	pkt->len = 0;
	pkt->sendpos = 0;
	pkt->pos = 0;
	pkt->capacity = ctx->pktpool->objsize - sizeof(packet_t);
	if (q == &s->cliq)
		FLOW("ssn[%d] client %d get a packet(%p)\n", 
		     s->id, s->clifd, pkt);
	else
		FLOW("ssn[%d] server %d get a packet(%p)\n",
		     s->id, s->clifd, pkt);

	return pkt;
}

/**
 *	Check sever is connect success or failed.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_proxy_check_server(proxy_t *ctx, session_t *s)
{
	struct epoll_event e;
	int ret = 0;
	int fd;
	int *pair;

	assert(ctx);
	assert(s);

	fd = s->svrfd;

	ret = sk_is_connected(fd);
	if (ret <= 0) {
		FLOW("ssn[%d] server %d connect failed\n", s->id, fd);
		return -1;
	}

	s->is_svrwait = 0;
	ctx->nblocked --;

	/* remove it from socket */
	if (epoll_ctl(ctx->send_epfd, EPOLL_CTL_DEL, fd, NULL)) {
		ERR("epoll del %d error: %s\n", fd, ERRSTR);
		return -1;
	}

	/* add to recv epoll */
	memset(&e, 0, sizeof(e));
	e.events = EPOLLIN;
	pair = (int *)&e.data.u64;
	pair[0] = fd;
	pair[1] = s->id;
	if (epoll_ctl(ctx->recv_epfd, EPOLL_CTL_ADD, fd, &e)) {
		ERR("epoll_add %d error: %s\n", fd, ERRSTR);
		return -1;
	}
	
	FLOW("ssn[%d] client %d connect to server %d success\n", 
	     s->id, s->clifd, fd);

	return 0;
}

/**
 *	Select a real server and connect to it. the select algorithm
 *	is Round-Robin.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_proxy_get_server(proxy_t *ctx, session_t *s)
{
	int fd;
	int wait;
	struct epoll_event e;
	int *pair;
	static int i = 0;

	if (s->svrfd > 0)
		return 0;

	fd = sk_tcp_client_nb(&ctx->rsaddrs[i], &wait);
	if (fd < 0) {
		s->is_svrerror = 1;
		ERR("get server socket error\n");
		return -1;
	}

	/* to next real server */
	i = (i + 1) % ctx->nrsaddr;

	/* add it to send epoll to wait connect success */
	if (wait) {
		pair = (int *)&e.data.u64;	
		pair[0] = fd;
		pair[1] = s->id;
		e.events = EPOLLOUT;			
		if (epoll_ctl(ctx->send_epfd, EPOLL_CTL_ADD, fd, &e)) {
			s->is_svrerror = 1;
			ERR("epoll_ctl(add) %d failed %s\n", fd, ERRSTR);
			close(fd);
			return -1;
		}
		ctx->nblocked ++;
		s->is_svrwait = 1;
		FLOW("ssn[%d] client %d connect to server %d wait\n", 
		     s->id, s->clifd, fd);
	}
	/* add to recv epoll */
	else {
		pair = (int *)&e.data.u64;	
		pair[0] = fd;
		pair[1] = s->id;
		e.events = EPOLLIN;			
		if (epoll_ctl(ctx->recv_epfd, EPOLL_CTL_ADD, fd, &e)) {
			s->is_svrerror = 1;
			ERR("epoll_ctl(add) %d error: %s\n", fd, ERRSTR);
			close(fd);
			return -1;
		}
		FLOW("ssn[%d] client %d connect to server %d success\n", 
		     s->id, s->clifd, fd);

	}

	s->svrfd = fd;

	return 0;
}

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
		FLOW("1 ssn[%d] free a packet %p\n", s->id, pkt);
		objpool_put(pkt);
	}

	pkt = q->blkpkt;
	if (pkt) {
		s->nalloced--;
		FLOW("2 ssn[%d] free a packet %p\n", s->id, pkt);
		objpool_put(pkt);
	}

	pkt = pktq_out(&q->inq);
	while(pkt) {
		s->nalloced--;
		FLOW("3 ssn[%d] free a packet %p\n", s->id, pkt);
		objpool_put(pkt);
		pkt = pktq_out(&q->inq);
	}

	pkt = pktq_out(&q->outq);
	while (pkt) {
		s->nalloced--;
		FLOW("4 ssn[%d] free a packet %p\n", s->id, pkt);
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
_proxy_free_session(proxy_t *ctx, session_t *s)
{
	int fd;
	int epfd;

	assert(ctx);
	assert(s);

	if (ctx->use_splice) {
		if (s->read_pipe >= 0)
			close(s->read_pipe);
		if (s->write_pipe >= 0)
			close(s->write_pipe);
	}

	/* close client socket */
	if (s->clifd > -1) {

		fd = s->clifd;

		epfd = ctx->send_epfd;
		if (s->is_cliblock && epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL))
			ERR("epoll_ctl(del) %d error %s\n", fd, ERRSTR);

		epfd = ctx->recv_epfd;
		if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL)) 
			ERR("epoll_ctl(del) %d error %s\n", fd, ERRSTR);
		
		FLOW("ssn[%d] client %d closed\n", s->id, fd);

		close(s->clifd);
	}

	/* close server socket */
	if (s->svrfd > -1) {

		fd = s->svrfd;

		epfd = ctx->send_epfd;
		if (s->is_svrwait) {
			if (epoll_ctl(ctx->send_epfd, EPOLL_CTL_DEL, fd, NULL))
				ERR("epoll del %d error %s\n", fd, ERRSTR);
		}
		else {
			epfd = ctx->send_epfd;
			if (s->is_svrblock && 
			    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL)) 
			{
				ERR("epoll del %d error %s\n", fd, ERRSTR);
			}

			epfd = ctx->recv_epfd;
			if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL))
				ERR("epoll del %d error %s\n", fd, ERRSTR);
		}

		FLOW("ssn[%d] server %d closed\n", s->id, fd);
		close(s->svrfd);
	}

	_proxy_clean_session(ctx, s, &s->cliq);

	_proxy_clean_session(ctx, s, &s->svrq);

	assert(s->nalloced == 0);

	/* delete session from session pool */
	FLOW("ssn[%d] deleted\n", s->id);

	session_table_del(ctx->sessions, s->id);
	objpool_put(s);

	ctx->stat.nliveconn--;
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
		ERR("get session from ssnpool failed\n");
		return NULL;
	}

	memset(s, 0, sizeof(session_t));

	if (session_table_add(ctx->sessions, s)) {
		ERR("add session to session table failed\n");
		objpool_put(s);
		return NULL;
	}

	s->clifd = -1;
	s->svrfd = -1;
	s->read_pipe = -1;
	s->write_pipe = -1;

	return s;
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
	ip_port_t cliaddr;
	int clifd;
	struct epoll_event e;
	session_t *s;
	int *pair;
	int pipefd[2];
	char buf[512], buf1[512];

	assert(ctx);
	
	clifd = sk_accept(ctx->httpfd, &cliaddr);
	if (clifd < 0) {
		if (errno == EAGAIN || errno == EINTR)
			return 0;

		ERR("accept error: %s\n", ERRSTR);
		return -1;
	}

	/* add client socket to session pool */
	s = _proxy_alloc_session(ctx);
	if (!s) {
		close(clifd);
		return -1;
	}
	s->clifd = clifd;	

	/* add client socket to epoll socket */
	memset(&e, 0, sizeof(e));
	e.events = EPOLLIN;
	pair = (int *)&e.data.u64;
	pair[0] = clifd;
	pair[1] = s->id;
	if (epoll_ctl(ctx->recv_epfd, EPOLL_CTL_ADD, clifd, &e)) {
		ERR("epoll add %d error: %s\n", clifd, ERRSTR);
		close(clifd);
		s->clifd = -1;
		_proxy_free_session(ctx, s);
		return -1;
	}

	if (ctx->use_splice || ctx->use_nb_splice) {
		if (_proxy_get_server(ctx, s)) {
			_proxy_free_session(ctx, s);
		}
	}

	if (ctx->use_splice) {
		if (pipe2(pipefd, O_NONBLOCK))
			_proxy_free_session(ctx, s);
		
		s->read_pipe = pipefd[0];
		s->write_pipe = pipefd[1];

		FLOW("ssn[%d] create splice pipe %d:%d\n",
			    s->id, s->read_pipe, s->write_pipe);
	}

	ctx->stat.nliveconn++;

	FLOW("ssn[%d] accept client %d %s->%s\n", s->id, clifd, 
	     ip_port_to_str(&cliaddr, buf, sizeof(buf)),
	     ip_port_to_str(&ctx->svraddr, buf1, sizeof(buf1)));

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

	pkt = _proxy_get_packet(ctx, s, &s->cliq);
	if (!pkt)
		return -1;
	
	blen = pkt->capacity - pkt->len;
	assert(blen > 0);
	n = sk_recv(clifd, pkt->data, blen, &close);
	if (n < 0) {
		s->is_clierror = 1;
		FLOW("ssn[%d] client %d recv error\n", s->id, clifd);
		return -1;
	}

	pkt->len += n;

	if (pkt->len == 0) {
		s->cliq.pkt = NULL;
		s->nalloced--;
		objpool_put(pkt);

		FLOW("ssn[%d] client %d free packet %p\n",
			    s->id, clifd, pkt);
	}
	
	if (close) {
		s->is_cliclose = 1;
		FLOW("ssn[%d] client %d recv closed\n", s->id, clifd);
		return 0;
	}

	FLOW("ssn[%d] client %d recv %d bytes\n", s->id, clifd, n);

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

	pkt = _proxy_get_packet(ctx, s, &s->svrq);
	if (!pkt)
		return -1;

	/* recv data */
	ptr = pkt->data + pkt->recvpos;
	blen = pkt->capacity - pkt->recvpos;
	assert(blen > 0);
	n = sk_recv(svrfd, ptr, blen, &close);
	if (n < 0) {
		s->is_svrerror = 1;
		FLOW("ssn[%d] server %d recv error\n", s->id, svrfd);
		return -1;
	}

	pkt->len += n;

	if (pkt->len == 0) {
		s->svrq.pkt = NULL;
		s->nalloced--;
		objpool_put(pkt);
		
		FLOW("ssn[%d] server %d free packet\n",
			    s->id, svrfd);
	}

	if (close) {
		s->is_svrclose = 1;
		FLOW("ssn[%d] server %d closed\n", s->id, svrfd);
		return 0;
	}

	FLOW("ssn[%d] server %d recv %d bytes\n", s->id, svrfd, n);

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
//	int len;
//	void *ptr;

	assert(ctx);
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
			ERR("http request parse error\n");
			return -1;
		}
	}
	/* client is closed */
	else {
		if (s->clihttp.state != HTTP_STE_FIN) {
			ERR("http request is not finished before closed\n");
			return -1;
		}
	}

	FLOW("session[%d] client %d http parse success\n", s->id, s->clifd);
	
	/* move packet to output queue */
	if (pkt->len == pkt->maxsess ||
	    s->clihttp.state == HTTP_STE_FIN) 
	{
		s->clicache.pkt = NULL;
		pktqueue_in(&s->clicache.output, pkt);

		FLOW("session[%d] client %d move current packet to output\n", 
			    s->id, s->clifd);
	}

#endif 

	pktq_in(&s->cliq.outq, pkt);
	s->cliq.pkt = NULL;

	FLOW("ssn[%d] client %d move current packet to output\n", 
	     s->id, s->clifd);

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
//	int len;
//	void *ptr;

	assert(ctx);
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
	
	FLOW("ssn %d server %d http parse success\n", 
		    s->id, s->svrfd);
	
	/* move packet to output queue for send */
	if (pkt->len == pkt->maxsess ||
	    s->svrhttp.state == HTTP_STE_FIN) 
	{
		s->svrcache.current = NULL;
		pktqueue_in(&s->svrcache.output, pkt);

		FLOW("ssn %d server %d move current packet to output\n", 
			    s->id, s->svrfd);	
	}

#endif

	pktq_in(&s->svrq.outq, pkt);
	s->svrq.pkt = NULL;

	FLOW("ssn[%d] server %d move packet to output\n", 
	     s->id, s->svrfd);

	return 0;
}

/**
 *	Send server data to client.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_proxy_sendto_client(proxy_t *ctx, session_t *s)
{
	packet_t *pkt;
	void *ptr;
	int *pair;
	struct epoll_event e;
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
		if (len > 100)
			len1 = len - 100;

		n = sk_send(fd, ptr, len1);
		if (n < 0) {
			FLOW("ssn[%d] server %d sendto client %d failed\n", 
			     s->id, s->svrfd, fd);
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
			if (epoll_ctl(ctx->send_epfd, EPOLL_CTL_ADD, fd, &e)) {
				ERR("epoll add %d error %s\n", fd, ERRSTR);
				s->nalloced--;
				objpool_put(pkt);
				return -1;
			}
			s->svrq.blkpkt = pkt;
			s->is_cliblock = 1;
			ctx->nblocked++;
			FLOW("ssn[%d] server %d send to client %d %d:%d blocked\n",
			     s->id, s->svrfd, fd, len, n);
			break;
		}

		if (s->is_cliblock) {
			if (epoll_ctl(ctx->send_epfd, EPOLL_CTL_DEL, fd, NULL)) {
				ERR("epoll del %d error %s\n", fd, ERRSTR);
				s->nalloced--;
				objpool_put(pkt);
				return -1;
			}
			s->is_cliblock = 0;
			ctx->nblocked--;
		}

		s->nalloced--;
		objpool_put(pkt);

		FLOW("ssn[%d] server %d sendto client %d %d bytes\n", 
		     s->id, s->svrfd, s->clifd, len);
		
		FLOW("ssn[%d] server %d free packet %p\n", 
		     s->id, s->svrfd, pkt);
		
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
_proxy_sendto_server(proxy_t *ctx, session_t *s)
{
	packet_t *pkt;
	struct epoll_event e;
	int fd;
	void *ptr;
	int len;
	int n;
	int *pair;
	int len1;

	assert(ctx);
	assert(s);

	if (s->cliq.outq.len < 1 && ! s->cliq.blkpkt)
		return 0;

	if (_proxy_get_server(ctx, s)) 
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
		if (len > 100) len1 = len - 100;
		n = sk_send(fd, ptr, len1);
		/* send error */
		if (n <= 0) {
			s->is_svrerror = 1;
			s->nalloced--;
			objpool_put(pkt);
			FLOW("ssn[%d] client %d sendto server %d error\n", 
			     s->id, s->clifd, fd);
			return -1;
		}
		/* send not finished, block client */
		else if (n < len) { 
			pkt->sendpos = pkt->sendpos + n;			
			if (s->is_svrblock)
				break;

			/* add it to send epoll */
			pair = (int *)&e.data.u64;
			pair[0] = fd;
			pair[1] = s->id;
			e.events = EPOLLOUT;
			if (epoll_ctl(ctx->send_epfd, EPOLL_CTL_ADD, fd, &e)) {
				s->is_svrerror = 1;
				objpool_put(pkt);
				s->nalloced--;
				FLOW("ssn[%d] client %d free a packet %p\n", 
				     s->id, s->clifd, pkt);
				ERR("epoll add %d error %s\n", fd, ERRSTR);
				return -1;
			}
			s->cliq.blkpkt = pkt;
			s->is_svrblock = 1;
			ctx->nblocked++;
			FLOW("ssn[%d] client %d sendto server %d %d:%d blocked\n", 
			     s->id, s->clifd, fd, len, n);
			break;
		}

		FLOW("ssn[%d] client %d sendto server %d %d bytes\n", 
		     s->id, s->clifd, fd, len);

		/* clear block flags because send success */
		if (s->is_svrblock) {
			if (epoll_ctl(ctx->send_epfd, EPOLL_CTL_DEL, fd, NULL)) {
				s->is_svrerror = 1;
				ERR("epoll del %d error %s\n", fd, ERRSTR);
				objpool_put(pkt);
				s->nalloced--;
				return -1;
			}
			s->is_svrblock = 0;
			ctx->nblocked--;
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
_proxy_nb_splice_cli2svr(proxy_t *ctx, session_t *s)
{
	int n;

	assert(ctx);
	assert(s);
	assert(s->clifd > 0);
	assert(s->svrfd > 0);

	if (s->is_svrwait)
		return 0;

	n = nb_splice(ctx->splice_fd, s->clifd, s->svrfd, 16384, 0);
	if (n < 0) {
		FLOW("ssn[%d] client %d nb_splice server %d failed/closed\n",
		    s->id, s->svrfd, s->clifd);
		return n;
	}
	FLOW("ssn[%d] client %d nb_splice server %d %d bytes\n", 
	     s->id, s->clifd, s->svrfd, n);

	return n;
}

/**
 *	splice(linux) client data to server
 *
 *	Return splice bytes if success, -1 on error.
 */
static int 
_proxy_splice_cli2svr(proxy_t *ctx, session_t *s)
{
	int n, m;
	
	assert(ctx);
	assert(s);
	assert(s->clifd > 0);
	assert(s->svrfd > 0);
	
	if (s->is_svrwait)
		return 0;

	n = splice(s->clifd, NULL, s->write_pipe, NULL, 16384, SPLICE_F_MOVE|SPLICE_F_NONBLOCK);
	FLOW("ssn[%d] client %d splice(1) server %d %d bytes\n", 
	     s->id, s->clifd, s->svrfd, n);
	if (n <= 0){
		if (n < 0)
			ERR("splice failed: %s\n", ERRSTR);
		return -1;
	}

	m = splice(s->read_pipe, NULL, s->svrfd, NULL, n, SPLICE_F_MOVE|SPLICE_F_NONBLOCK);
	FLOW("ssn[%d] client %d splice(2) server %d %d bytes\n", 
		    s->id, s->clifd, s->svrfd, m);

	return m;
}

/**
 *	Process client request, include recv client data, parse client data, 
 *	and send client data to server 
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_proxy_process_client(proxy_t *ctx, session_t *s)
{
	int ret = 0;

	assert(ctx);
	assert(s);
	assert(s->clifd >= 0);

	if (ctx->use_nb_splice) {
		ret = _proxy_nb_splice_cli2svr(ctx, s);
		if (ret < 0) {
			_proxy_free_session(ctx, s);		
			return -1;
		}
		return ret;
	}

	if (ctx->use_splice) {
		ret = _proxy_splice_cli2svr(ctx, s);
		if (ret < 0) {
			_proxy_free_session(ctx, s);		
			return -1;
		}
		return ret;
	}

	if (_proxy_recvfrom_client(ctx, s)) {
		_proxy_free_session(ctx, s);
		return -1;
	}
       
	if (_proxy_parse_client(ctx, s)) {
		_proxy_free_session(ctx, s);
		return -1;
	}
		
	if (_proxy_sendto_server(ctx, s)) {
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
 *	nb_splice server data to client.	
 *
 *	Return splice bytes if success, -1 on error.
 */
static int 
_proxy_nb_splice_svr2cli(proxy_t *ctx, session_t *s)
{
	int n;

	assert(ctx);
	assert(s);
	assert(s->clifd > 0);
	assert(s->svrfd > 0);

	n = nb_splice(ctx->splice_fd, s->svrfd, s->clifd, 16384, 0);
	if (n < 0) {
		FLOW("ssn[%d] server %d nb_splice client %d failed/closed\n",
		    s->id, s->svrfd, s->clifd);
		return n;
	}
	FLOW("ssn[%d] server %d nb_splice client %d %d bytes\n",
	     s->id, s->svrfd, s->clifd, n);

	return n;
}

/**
 *	Splice server data to client.
 *
 *	Return splice bytes is success, -1 on error.
 */
static int 
_proxy_splice_svr2cli(proxy_t *ctx, session_t *s)
{
	int n, m;
	
	assert(ctx);
	assert(s);
	assert(s->clifd > 0);
	assert(s->svrfd > 0);
	
	n = splice(s->svrfd, NULL, s->write_pipe, NULL, 4096, SPLICE_F_MOVE|SPLICE_F_NONBLOCK);
	FLOW("ssn[%d] server %d splice(1) client %d %d bytes\n", 
	     s->id, s->svrfd, s->clifd, n);
	if (n <= 0) {
		if (n < 0)
			ERR("splice failed %s\n", ERRSTR);
		return -1;
	}

	m = splice(s->read_pipe, NULL, s->clifd, NULL, n, SPLICE_F_MOVE|SPLICE_F_NONBLOCK);
	FLOW("ssn[%d] server %d splice(2) client %d %d bytes\n", 
	     s->id, s->svrfd, s->clifd, m);
	return m;
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
	int ret = 0;

	assert(ctx);
	assert(s);
	assert(s->svrfd > -1);

	if (ctx->use_nb_splice) {
		ret = _proxy_nb_splice_svr2cli(ctx, s);
		if (ret < 0) {
			_proxy_free_session(ctx, s);
			return -1;
		}
		
		return ret;
	}

	if (ctx->use_splice) {
		ret = _proxy_splice_svr2cli(ctx, s);
		if (ret < 0) {
			_proxy_free_session(ctx, s);
			return -1;
		}
		return ret;
	}

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
	struct epoll_event *e;
	int *pair;
	int fd;
	int id;
	int nfds;
	int i;

	assert(ctx);

	nfds = epoll_wait(ctx->recv_epfd, ctx->events, 
			  ctx->maxsess, PROXY_TIMEOUT);
	if (nfds < 0) {
		if (errno == EINTR)
			return;
		ERR("epoll_wait error: %s", ERRSTR);
		return;
	}

	if (nfds == 0)
		return;

	for (i = 0; i < nfds; i++) {
		
		e = &ctx->events[i];

		/* only accept when exist connections not 
		 * exceed proxy's maxsess 
		 */
		if (e->data.fd == ctx->httpfd) {
			if (ctx->stat.nliveconn < ctx->maxsess) {
				_proxy_accept_client(ctx);
			}
			else {
				FLOW("too many connection %lu, max %u\n",
				     ctx->stat.nliveconn, ctx->maxsess);
			}
		}
		/* recv data */
		else {
			pair = (int *)&ctx->events[i].data.u64;
			fd = pair[0];
			id = pair[1];
			s = session_table_find(ctx->sessions, id);
			if (!s) {
				continue;
			}

			if (e->events & EPOLLERR) {
				_proxy_free_session(ctx, s);
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
				ERR("the fd %d is in session %d", fd, id);
			}
		}
	}
}




static void  
_proxy_send_data(proxy_t *ctx) 
{
	session_t *s;
	struct epoll_event *e;
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
			  ctx->maxsess, PROXY_TIMEOUT);
	if (nfds < 0) {
		if (errno == EINTR)
			return;
		ERR("epoll_wait error: %s", ERRSTR);
		return;
	}

	if (nfds == 0)
		return;

	for (i = 0; i < nfds; i++) {
		
		/* recv data */
		e = &ctx->events[i];
		pair = (int *)&e->data.u64;
		fd = pair[0];
		id = pair[1];
		s = session_table_find(ctx->sessions, id);
		if (!s) {
			continue;
		}

		if (e->events & EPOLLERR) {
			_proxy_free_session(ctx, s);
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
			ERR("the fd %d is not in session %d", fd, id);
			if (epoll_ctl(ctx->send_epfd, EPOLL_CTL_DEL, fd, NULL)) {
				ERR("del fd %d from send epoll failed\n", fd);
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
	event.data.fd = ctx->httpfd;
	event.events = EPOLLIN;
	fd = ctx->httpfd;
	if (epoll_ctl(ctx->recv_epfd, EPOLL_CTL_ADD, fd, &event)) {
		ERR("epoll add %d error %s\n", fd, ERRSTR);
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
 *	is @maxsess, and proxy will exit after process @maxconns client 
 *	connections.
 *
 *	return 0 if success, -1 on error.
 */
int 
proxy_main(proxy_arg_t *arg)
{
	proxy_t ctx;
	char buf[512] = {0};
	int i;

	if (!arg) {
		ERR("invalid arguments\n");
		return -1;
	}
	
	memset(&ctx, 0, sizeof(ctx));
	ctx.svraddr = arg->svraddr;
	memcpy(ctx.rsaddrs, arg->rsaddrs, arg->nrsaddr * sizeof(ip_port_t));
	ctx.nrsaddr = arg->nrsaddr;
	ctx.maxsess = arg->maxsess;
	ctx.use_splice = arg->use_splice;
	ctx.use_nb_splice = arg->use_nb_splice;
	ctx.index = arg->index;
	_g_index = arg->index;
	ctx.httpfd = arg->httpfd;

	DBG("started, listen fd %d,  maxsess %d\n", 
	    ctx.httpfd, ctx.maxsess);
	DBG("real servers(%d):\n", ctx.nrsaddr);
	for (i = 0; i < ctx.nrsaddr; i++) {
		DBG("\t%d: %s\n", i, 
		       ip_port_to_str(&ctx.rsaddrs[i], buf, sizeof(buf)));
	}

	if (_proxy_init(&ctx)) {
		_proxy_free(&ctx);
		return -1;
	}
	
	_proxy_loop(&ctx);

	_proxy_free(&ctx);

	return 0;
}


