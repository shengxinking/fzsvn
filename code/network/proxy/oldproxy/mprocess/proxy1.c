/**
 *	file	proxy.c
 *
 *	brief	The proxy main APIs, it accept connections, recv data, 
 *		parse data and send data.
 *
 *	author	Forrest.zhang
 */

#define _GNU_SOURCE
#define NDEBUG

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
#include "childproc.h"

/**
 *	Define MACROs for print error message and debug message.
 */

static int _g_dbglvl = 0;
static int _g_stop = 0;
static int _g_index = 0;

#define	DBG(fmt, args...)			\
{						\
	if (unlikely(_g_dbglvl > 0))			\
		printf("work[%d]: "fmt,	\
			_g_index, ##args);	\
}

#define FLOW(lvl, fmt, args...)			\
{						\
	if (unlikely(_g_dbglvl > lvl))			\
		printf("work[%d]: ssn<%d> "fmt,	\
			_g_index, s->id, ##args);\
}

#define	ERR(fmt, args...)			\
	fprintf(stderr, "%s:%d: work[%d] "fmt,	\
		__FILE__, __LINE__, \
		_g_index, ##args)
#define ERRSTR			strerror(errno)


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

static int _proxy_process_client(proxy_t *ctx, session_t *s);

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
	signal(SIGINT, SIG_IGN);
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
	int *pair;
	struct epoll_event e;
	policy_t *pl;
	int i;

	assert(ctx);
	
	/* set SIGUSR1 as quit signal*/
	signal(SIGUSR1, _proxy_stop);
	signal(SIGINT, SIG_IGN);

	/* set max file descriptor to 2 * maxsess if maxsess > 512 */
	if (ctx->max > 256) {
		rlim.rlim_cur = 4 * ctx->max + 20;
		rlim.rlim_max = 4 * ctx->max + 20;
		
		if (setrlimit(RLIMIT_NOFILE, &rlim)) {
			ERR("setrlimit(RLIMIT_NOFILE) error: %s", ERRSTR);
			return -1;
		}
		
		DBG("set max file descriptor to %d\n", (2 * ctx->max));
	}

	/* malloc memory for epoll event */
	ctx->events = malloc(ctx->max * sizeof(struct epoll_event));
	if (!ctx->events) {
		ERR("malloc memory for epoll event error %s\n", ERRSTR);
		return -1;
	}
	DBG("create epoll events %p\n", ctx->events);

	/* create memory pool */
	ctx->pktpool = objpool_alloc(4096, 1024, 0);
	if (!ctx->pktpool) {
		ERR("create objpool for packet error\n");
		return -1;
	}
	DBG("create memory pool for packet %p\n", ctx->pktpool);
	
	ctx->ssnpool = objpool_alloc(sizeof(session_t), 1024, 0);
	if (!ctx->ssnpool) {
		ERR("create objpool for session error\n");
		return -1;
	}
	DBG("create memory pool for session %p\n", ctx->ssnpool);
	
	/* create session table */
	ctx->sessions = session_table_alloc(ctx->max, 0);
	if (!ctx->sessions) {
		ERR("create session table error\n");
		return -1;
	}
	DBG("create session table %p\n", ctx->sessions);

	/* nb_splice */
	if (ctx->use_nb_splice) {
		ctx->splice_fd = nb_splice_init();
		if (ctx->splice_fd < 0) {
			ERR("nb splice init failed\n");
			return -1;
		}
	}

	/* create send epoll socket */
	ctx->epfd = epoll_create(ctx->max);
	if (ctx->epfd < 0) {
		ERR("create epoll fd error\n");
		return -1;
	}
	DBG("create epoll fd %d\n", ctx->epfd);

	for (i = 0; i < ctx->npolicy; i++) {
		pl = &ctx->policies[i];
		pl->httpfd = sk_tcp_server(&pl->httpaddr, 1);
		if (pl->httpfd < 0) {
			ERR("create http listen socket failed\n");
			return -1;
		}
		sk_set_nonblock(pl->httpfd, 1);
	
		/* add listen fd to recv epoll */
		e.events = EPOLLIN;
		pair = (int *)&e.data.u64;
		pair[0] = pl->httpfd;
		pair[1] = -(i + 1);
		if (epoll_ctl(ctx->epfd, EPOLL_CTL_ADD, pl->httpfd, &e)) {
			ERR("epoll add %d error %s\n", pl->httpfd, ERRSTR);
			return -1;
		}
		ctx->nepollevt++;
		DBG("create http listen fd %d\n", pl->httpfd);
	}

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
	policy_t *pl;
	int i;

	assert(ctx);

	for (i = 0; i < ctx->npolicy; i++) {
		pl = &ctx->policies[i];
		if (pl->httpfd > 0) {
			close(pl->httpfd);
		}
	}

	if (ctx-> epfd > 0) 
		close(ctx->epfd);

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
		FLOW(2, "client %d get a packet(%p)\n", s->clifd, pkt)
	else
		FLOW(2, "server %d get a packet(%p)\n", s->clifd, pkt)

	return pkt;
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
	int wait = 0;
	struct epoll_event e;
	int *pair;
	policy_t *pl;
	ip_port_t *rs;

	if (s->svrfd > 0)
		return 0;
	
	assert(s->policy_id >= 0 && s->policy_id < ctx->npolicy);

	pl = &ctx->policies[s->policy_id];
	if (pl->pos < 0 || pl->pos >= pl->nrsaddr)
		pl->pos = 0;
	rs = &pl->rsaddrs[pl->pos];
	pl->pos++;

	fd = sk_tcp_client_nb(rs, &wait);
	if (fd < 0) {
		s->is_svrerror = 1;
		ERR("get server socket error\n");
		return -1;
	}

	/* add it to send epoll to wait connect success */
	if (wait) {
		/* add to epoll for write */
		e.events = EPOLLOUT;
		pair = (int *)&e.data.u64;	
		pair[0] = fd;
		pair[1] = s->id;
		if (epoll_ctl(ctx->epfd, EPOLL_CTL_ADD, fd, &e)) {
			s->is_svrerror = 1;
			ERR("epoll_ctl(add) %d failed %s\n", fd, ERRSTR);
			close(fd);
			return -1;
		}
		ctx->nepollevt++;
		s->is_svrep = 1;
		FLOW(3, "server %d epoll add write\n", fd);
		
#if 0
		/* remove client from epoll */
		if (epoll_ctl(ctx->epfd, EPOLL_CTL_DEL, s->clifd, NULL)) {
			s->is_clierror = 1;
			ERR("epoll del %d error %s\n", fd, ERRSTR);
			return -1;
		}
		s->is_cliep = 0;
		ctx->nepollevt--;
		FLOW(3, "client %d epoll del read\n", s->clifd);
#endif

		s->is_svrwait = 1;
		ctx->nblocked++;
		FLOW(1, "client %d connect to server %d wait\n", s->clifd, fd);
	}
	/* add to recv epoll */
	else {
		sk_set_nodelay(fd, 1);
		sk_set_quickack(fd, 1);

		e.events = EPOLLIN | EPOLLRDHUP;			
		pair = (int *)&e.data.u64;	
		pair[0] = fd;
		pair[1] = s->id;
		if (epoll_ctl(ctx->epfd, EPOLL_CTL_ADD, fd, &e)) {
			s->is_svrerror = 1;
			ERR("epoll_ctl(add) %d error: %s\n", fd, ERRSTR);
			close(fd);
			return -1;
		}
		ctx->nepollevt++;
		s->is_svrep = 1;
		FLOW(3, "server %d epoll add write\n", fd);

		FLOW(1, "client %d connect to server %d success\n", 
		     s->clifd, fd);
	}

	s->svrfd = fd;

	return 0;
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
	int fd;
	int *pair;

	assert(ctx);
	assert(s);

	fd = s->svrfd;

	int ret = 0;
	ret = sk_is_connected(fd);
	if (ret <= 0) {
		FLOW(1, "server %d connect failed\n", fd);
		return -1;
	}

	sk_set_nodelay(fd, 1);
	sk_set_quickack(fd, 1);

	/* mod it in epoll(write->read) */
	pair = (int *)&e.data.u64;
	pair[0] = fd;
	pair[1] = s->id;
	e.events = EPOLLIN;
	if (epoll_ctl(ctx->epfd, EPOLL_CTL_MOD, fd, &e)) {
		ERR("epoll mod %d error: %s\n", fd, ERRSTR);
		return -1;
	}
	s->is_svrwait = 0;
	ctx->nblocked--;
	FLOW(3, "server %d epoll mod write->read\n", fd);

#if 0
	/* add client to epoll(read) */
	memset(&e, 0, sizeof(e));
	e.events = EPOLLIN | EPOLLRDHUP;
	pair = (int *)&e.data.u64;
	pair[0] = s->clifd;
	pair[1] = s->id;
	if (epoll_ctl(ctx->epfd, EPOLL_CTL_ADD, s->clifd, &e)) {
		ERR("epoll add %d error: %s\n", s->clifd, ERRSTR);
		return -1;
	}
	s->is_cliep = 1;
	ctx->nepollevt++;
	FLOW(3, "client %d epoll add read\n", s->clifd);
#endif

	FLOW(1, "client %d connect to server %d success\n", s->clifd, fd);

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
		FLOW(2, "free a current packet %p\n", pkt);
		objpool_put(pkt);
	}

	pkt = q->blkpkt;
	if (pkt) {
		s->nalloced--;
		FLOW(2, "free a block packet %p\n", pkt);
		objpool_put(pkt);
	}

	pkt = pktq_out(&q->inq);
	while(pkt) {
		s->nalloced--;
		FLOW(2, "free a input queue packet %p\n", pkt);
		objpool_put(pkt);
		pkt = pktq_out(&q->inq);
	}

	pkt = pktq_out(&q->outq);
	while (pkt) {
		s->nalloced--;
		FLOW(2, "free a output queue packet %p\n", pkt);
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
	int is_cliclose;
	int is_clierror;
	int is_svrclose;
	int is_svrerror;

	assert(ctx);
	assert(s);

	if (ctx->use_splice) {
		if (s->read_pipe >= 0)
			close(s->read_pipe);
		if (s->write_pipe >= 0)
			close(s->write_pipe);
	}

	/* close client socket */
	if (s->clifd > 0) {
		fd = s->clifd;
		epfd = ctx->epfd;

		if (s->is_cliep && epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL))
			ERR("epoll_ctl(del) %d error %s\n", fd, ERRSTR);
		ctx->nepollevt--;
		close(s->clifd);
		FLOW(1, "client %d closed\n", fd);
	}

	/* close server socket */
	if (s->svrfd > -1) {

		fd = s->svrfd;
		epfd = ctx->epfd;

		if (s->is_svrep && epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL)) 
			ERR("epoll_ctl(del) %d error %s\n", fd, ERRSTR);
		ctx->nepollevt--;
		close(s->svrfd);
		FLOW(1, "server %d closed\n", fd);
	}

	_proxy_clean_session(ctx, s, &s->cliq);

	_proxy_clean_session(ctx, s, &s->svrq);

	assert(s->nalloced == 0);

	/* delete session from session pool */
	FLOW(1, "deleted\n");

	session_table_del(ctx->sessions, s->id);

	is_cliclose = s->is_cliclose;
	is_svrclose = s->is_svrclose;
	is_clierror = s->is_clierror;
	is_svrerror = s->is_svrerror;

	objpool_put(s);

	ctx->stat.nhttplive--;
	ctx->stat.nlive--;
	if (is_cliclose)
		ctx->stat.ncliclose++;
	if (is_svrclose)
		ctx->stat.nsvrclose++;
	if (is_clierror)
		ctx->stat.nclierror++;
	if (is_svrerror)
		ctx->stat.nsvrerror++;
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
_proxy_accept_client(proxy_t *ctx, policy_t *pl)
{
	ip_port_t cliaddr;
	int clifd;
	struct epoll_event e;
	session_t *s;
	int *pair;
	int pipefd[2];
	char ipstr1[IP_STR_LEN], ipstr2[IP_STR_LEN];
	int i;

	assert(ctx);
	
	for (i = 0; i < 8; i++) {
	clifd = sk_tcp_accept(pl->httpfd, &cliaddr);
	if (clifd < 0) {
		if (errno == EAGAIN || errno == EINTR)
			return 0;

		ERR("accept error: %s\n", ERRSTR);
		return -1;
	}

	/* update statistic data */
	ctx->stat.naccept++;

	/* set TCP option */
	sk_set_nonblock(clifd, 1);
	sk_set_nodelay(clifd, 1);
	sk_set_quickack(clifd, 1);

	/* check max connection */
	if (unlikely(ctx->stat.nlive >= ctx->max)) {
		close(clifd);
		ERR("too many client %lu max %d\n", ctx->stat.nlive, ctx->max);
		return -1;
	}

	/* add client socket to session pool */
	s = _proxy_alloc_session(ctx);
	if (!s) {
		close(clifd);
		return -1;
	}

	/* add client socket to epoll socket */
	memset(&e, 0, sizeof(e));
	e.events = EPOLLIN | EPOLLRDHUP;
	pair = (int *)&e.data.u64;
	pair[0] = clifd;
	pair[1] = s->id;
	if (epoll_ctl(ctx->epfd, EPOLL_CTL_ADD, clifd, &e)) {
		ERR("epoll add %d error: %s\n", clifd, ERRSTR);
		close(clifd);
		s->clifd = -1;
		_proxy_free_session(ctx, s);
		return -1;
	}
	s->is_cliep = 1;
	ctx->nepollevt++;

	FLOW(3, "ssn<%d> client %d epoll add read\n", 
	     s->id, clifd);

	s->clifd = clifd;
	s->policy_id = pl->index;

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

		FLOW(1, "create splice pipe %d:%d\n",
				s->read_pipe, s->write_pipe);
	}

	FLOW(1, "policy %d accept client %d %s->%s\n", pl->index, clifd, 
			ip_port_to_str(&cliaddr, ipstr1, IP_STR_LEN),
			ip_port_to_str(&pl->httpaddr, ipstr2, IP_STR_LEN));

	ctx->stat.nhttp++;
	ctx->stat.nhttplive++;
	ctx->stat.nlive++;

	/* try to connect server */
	if (_proxy_get_server(ctx, s))
		_proxy_free_session(ctx, s);

	/* try process client right now */
	if (_proxy_process_client(ctx, s))
		_proxy_free_session(ctx, s);
	}

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
	n = sk_recv(clifd, pkt->data, blen, &close);
	if (n < 0) {
		s->is_clierror = 1;
		FLOW(1, "client %d recv error\n", clifd);
		return -1;
	}

	pkt->len += n;
	ctx->stat.nclirecv += n;

	if (pkt->len == 0) {
		s->cliq.pkt = NULL;
		s->nalloced--;
		objpool_put(pkt);

		FLOW(2, "client %d free packet %p\n", clifd, pkt);
	}
	
	if (close) {
		s->is_cliclose = 1;
		FLOW(1, "client %d recv closed\n", clifd);
		return 0;
	}

	FLOW(1, "client %d recv %d bytes\n", clifd, n);

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
	n = sk_recv(svrfd, ptr, blen, &close);
	if (n < 0) {
		s->is_svrerror = 1;
		FLOW(1, "ssn[%d] server %d recv error\n", s->id, svrfd);
		return -1;
	}

	pkt->len += n;
	ctx->stat.nsvrrecv += n;

	if (pkt->len == 0) {
		s->svrq.pkt = NULL;
		s->nalloced--;
		objpool_put(pkt);
		
		FLOW(2, "ssn[%d] server %d free packet %p\n",
			    s->id, svrfd, pkt);
	}

	if (close) {
		s->is_svrclose = 1;
		FLOW(1, "server %d closed\n", svrfd);
		return 0;
	}

	FLOW(1, "server %d recv %d bytes\n", svrfd, n);

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

	FLOW(2, "client %d move current packet to output\n", s->clifd);

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

	FLOW(2, "server %d move current packet to output\n", s->svrfd);

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
		//if (len > 100) len1 = len - 100;
		
		n = sk_send(fd, ptr, len1);
		if (n < 0) {
			FLOW(1, "ssn[%d] server %d sendto client %d failed\n", 
			     s->id, s->svrfd, fd);
			s->is_clierror = 1;
			s->nalloced--;
			objpool_put(pkt);
			return -1;
		}
		
		ctx->stat.nclisend += n;

		if (n < len) {
			pkt->sendpos += n;
			if (s->is_cliblock)
				break;
			
			/* add it to send epoll and set block */
			memset(&e, 0, sizeof(e));
			e.events = EPOLLOUT;
			pair = (int *)&e.data.u64;
			pair[0] = fd;
			pair[1] = s->id;
			if (epoll_ctl(ctx->epfd, EPOLL_CTL_MOD, fd, &e)) {
				s->nalloced--;
				objpool_put(pkt);
				FLOW(2, "server %d free a packet %p", s->svrfd, pkt);
				ERR("epoll add %d error %s\n", fd, ERRSTR);
				return -1;
			}
			FLOW(3, "ssn<%d> client %d epoll mod read->write\n", 
			     s->id, fd);

#if 1
			/* remove server fd from epoll */
			if (epoll_ctl(ctx->epfd, EPOLL_CTL_DEL, s->svrfd, NULL)) {
				ERR("epoll del %d error %s\n", fd, ERRSTR);
				objpool_put(pkt);
				s->nalloced--;
				FLOW(2, "server %d free a packet %p\n", s->svrfd, pkt);
				ERR("epoll del %d error %s\n", s->svrfd, ERRSTR);
				return -1;
			}
			s->is_svrep = 0;
			ctx->nepollevt--;
			FLOW(3, "server %d epoll del read(blocked)\n", s->svrfd);
#endif

			s->svrq.blkpkt = pkt;
			s->is_cliblock = 1;
			ctx->nblocked++;
			FLOW(1, "server %d send to client %d %d:%d blocked\n",
			     s->svrfd, fd, len, n);
			break;
		}

		if (s->is_cliblock) {
			e.events = EPOLLIN;
			pair = (int *)&e.data.u64;
			pair[0] = fd;
			pair[1] = s->id;
			if (epoll_ctl(ctx->epfd, EPOLL_CTL_MOD, fd, &e)) {
				objpool_put(pkt);
				s->nalloced--;
				FLOW(2, "server %d free a packet %p\n", s->svrfd, pkt);
				ERR("epoll mod %d error %s\n", fd, ERRSTR);
				return -1;
			}
			FLOW(3, "client %d epoll mod read(not blocked)\n", fd);

#if 0
			/* add server fd to epoll for read */
			e.events = EPOLLIN | EPOLLRDHUP;
			pair = (int *)&e.data.u64;
			pair[0] = s->svrfd;
			pair[1] = s->id;
			if (epoll_ctl(ctx->epfd, EPOLL_CTL_ADD, s->svrfd, &e)) {
				objpool_put(pkt);
				s->nalloced--;
				FLOW(2, "server %d free a packet %p\n",
					s->svrfd, pkt);
				ERR("epoll add %d error %s\n", s->svrfd, ERRSTR);
				return -1;
			}
			s->is_svrep = 1;
			ctx->nepollevt++;
			FLOW(3, "server %d epoll add read(not blocked)\n", fd);
#endif

			s->is_cliblock = 0;
			ctx->nblocked--;
		}

		s->nalloced--;
		objpool_put(pkt);

		FLOW(1, "server %d sendto client %d %d bytes\n", 
		     s->svrfd, s->clifd, len);
		
		FLOW(2, "server %d free packet %p\n", s->svrfd, pkt);
		
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

	if (_proxy_get_server(ctx, s)) 
		return -1;

	if (s->is_svrwait) 
		return 0;

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

		n = sk_send(fd, ptr, len1);
		/* send error */
		if (n <= 0) {
			s->is_svrerror = 1;
			s->nalloced--;
			objpool_put(pkt);
			FLOW(2, "ssn[%d] client %d free a packet %p\n", 
				s->id, s->clifd, pkt);
			FLOW(1, "ssn[%d] client %d sendto server %d error\n", 
			     s->id, s->clifd, fd);
			return -1;
		}
		/* send not finished, block client */
		else if (n < len) { 
			pkt->sendpos = pkt->sendpos + n;			
			if (s->is_svrblock)
				break;

			/* mod server epoll read->write */
			pair = (int *)&e.data.u64;
			pair[0] = fd;
			pair[1] = s->id;
			e.events = EPOLLOUT;
			if (epoll_ctl(ctx->epfd, EPOLL_CTL_MOD, fd, &e)) {
				s->is_svrerror = 1;
				objpool_put(pkt);
				s->nalloced--;
				FLOW(2, "client %d free a packet %p\n", s->clifd, pkt);
				ERR("epoll mod %d error %s\n", fd, ERRSTR);
				return -1;
			}
			FLOW(3, "server %d epoll mod read->write(blocked)\n", fd);

#if 1
			/* remove client fd from epoll */
			if (epoll_ctl(ctx->epfd, EPOLL_CTL_DEL, s->clifd, NULL)) {
				ERR("epoll del %d error %s\n", s->clifd, ERRSTR);
				return -1;
			}
			s->is_cliep = 0;
			ctx->nepollevt--;
			FLOW(3, "client %d epoll del read(blocked)\n", s->clifd);
#endif

			s->cliq.blkpkt = pkt;
			s->is_svrblock = 1;
			ctx->nblocked++;
			FLOW(1, "client %d sendto server %d %d:%d blocked\n", 
			     s->clifd, fd, len, n);
			break;
		}

		ctx->stat.nsvrsend += n;

		/* clear block flags because send success */
		if (s->is_svrblock) {
			pair = (int *)&e.data.u64;
			pair[0] = fd;
			pair[1] = s->id;
			e.events = EPOLLIN;
			if (epoll_ctl(ctx->epfd, EPOLL_CTL_MOD, fd, &e)) {
				s->is_svrerror = 1;
				objpool_put(pkt);
				s->nalloced--;
				FLOW(2, "client %d free a packet(%p)\n", s->clifd, pkt);
				ERR("epoll mod %d error %s\n", fd, ERRSTR);
				return -1;
			}
			FLOW(3, "server %d epoll mod write->read\n", fd);
			s->is_svrblock = 0;
			ctx->nblocked--;

#if 1
			/* add client to read epoll */
			e.events = EPOLLIN | EPOLLRDHUP;
			pair = (int *)&e.data.u64;
			pair[0] = s->clifd;
			pair[1] = s->id;
			if (epoll_ctl(ctx->epfd, EPOLL_CTL_ADD, s->clifd, &e)) {
				s->is_clierror = 1;
				objpool_put(pkt);
				s->nalloced--;
				FLOW(2, "client %d free a packet(%p)\n", s->clifd, pkt);
				ERR("epoll mod %d error %s\n", s->clifd, ERRSTR);
				return -1;
			}
			s->is_cliep = 1;
			ctx->nepollevt++;
			FLOW(3, "client %d epoll add read(not blocked)\n", s->clifd);
#endif
			s->is_svrblock = 0;
			ctx->nblocked--;
		}

		s->nalloced--;
		objpool_put(pkt);

		FLOW(1, "client %d sendto server %d %d bytes\n", 
		     s->clifd, fd, len);

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
		FLOW(1, "client %d nb_splice server %d failed/closed\n",
		     s->svrfd, s->clifd);
		return n;
	}
	FLOW(1, "client %d nb_splice server %d %d bytes\n", 
	     s->clifd, s->svrfd, n);

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
	FLOW(1, "client %d splice(1) server %d %d bytes\n", 
	     s->clifd, s->svrfd, n);
	if (n <= 0){
		if (n < 0)
			ERR("splice failed: %s\n", ERRSTR);
		return -1;
	}

	m = splice(s->read_pipe, NULL, s->svrfd, NULL, n, SPLICE_F_MOVE|SPLICE_F_NONBLOCK);
	FLOW(1, " client %d splice(2) server %d %d bytes\n", 
	     s->clifd, s->svrfd, m);

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
	assert(ctx);
	assert(s);
	assert(s->clifd >= 0);

	if (ctx->use_nb_splice) 
		return _proxy_nb_splice_cli2svr(ctx, s);

	if (ctx->use_splice) 
		return _proxy_splice_cli2svr(ctx, s);

	if (_proxy_recvfrom_client(ctx, s)) 
		return -1;
       
	if (_proxy_parse_client(ctx, s)) 
		return -1;
		
	if (_proxy_sendto_server(ctx, s)) 
		return -1;

	if (s->is_clierror || s->is_cliclose) 
		return -1;

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
		FLOW(1, "server %d nb_splice client %d failed/closed\n",
		    s->svrfd, s->clifd);
		return n;
	}
	FLOW(1, "server %d nb_splice client %d %d bytes\n",
	     s->svrfd, s->clifd, n);

	return 0;
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
	FLOW(1, "server %d splice(1) client %d %d bytes\n", 
	     s->svrfd, s->clifd, n);
	if (n <= 0) {
		if (n < 0)
			ERR("splice failed %s\n", ERRSTR);
		return -1;
	}

	m = splice(s->read_pipe, NULL, s->clifd, NULL, n, SPLICE_F_MOVE|SPLICE_F_NONBLOCK);
	FLOW(1, "server %d splice(2) client %d %d bytes\n", 
	     s->svrfd, s->clifd, m);

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

	if (ctx->use_nb_splice) 
		return  _proxy_nb_splice_svr2cli(ctx, s);

	if (ctx->use_splice) 
		return  _proxy_splice_svr2cli(ctx, s);

	if (_proxy_recvfrom_server(ctx, s)) 
		return -1;

	if (_proxy_parse_server(ctx, s)) 
		return -1;

	if (_proxy_sendto_client(ctx, s)) 
		return -1;

	if (s->is_svrerror || s->is_svrclose) 
		return -1;

	return 0;
}


static void  
_proxy_epoll(proxy_t *ctx) 
{
	session_t *s;
	struct epoll_event *e;
	int *pair;
	int fd;
	int id;
	int nfds;
	int i;
	policy_t *pl;

	assert(ctx);
	assert(ctx->nepollevt > 0);

	nfds = epoll_wait(ctx->epfd, ctx->events, 
			  ctx->nepollevt, PROXY_TIMEOUT);
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

		if (id < 0) {
			if (e->events & EPOLLERR) {
				ERR("work[%d] listen fd %d epoll error: %s\n",
				    _g_index, fd, ERRSTR);
				continue;
			}

			assert((-id) > 0 && (-id) <= ctx->npolicy);
			pl = &ctx->policies[(-id) - 1];

			_proxy_accept_client(ctx, pl);
		}

		s = session_table_find(ctx->sessions, id);
		if (!s) {
			continue;
		}

		if (e->events & EPOLLERR) {
			FLOW(1, "fd %d epoll error\n", fd);
			_proxy_free_session(ctx, s);
			continue;
		}

		if (e->events & EPOLLRDHUP) {
			_proxy_free_session(ctx, s);
			continue;
		}

		if (s->clifd == fd) {
			if (e->events & EPOLLIN) {
				FLOW(3, "client %d have read event\n", fd);
				
				if (_proxy_process_client(ctx, s))
					_proxy_free_session(ctx, s);
			}
			else if (e->events & EPOLLOUT) {
				FLOW(3, "client %d have write event\n", fd);
				if (_proxy_sendto_client(ctx, s))
					_proxy_free_session(ctx, s);
			}
			else {
				ERR("client %d unknowed event %d\n", fd, e->events);
			}
		}
		else if (s->svrfd == fd) {
			if (e->events & EPOLLIN) {
				FLOW(3, "server %d have read event\n", fd);
				if (_proxy_process_server(ctx, s))
					_proxy_free_session(ctx, s);
			
			}
			else if (e->events & EPOLLOUT) {
				FLOW(3, "server %d have write event\n", fd);
				if (s->is_svrwait && _proxy_check_server(ctx, s))
				{
					_proxy_free_session(ctx, s);
					continue;
				}

				if (_proxy_sendto_server(ctx, s)) 
					_proxy_free_session(ctx, s);
			}
			else {
				ERR("server %d unknowed event %d\n", fd, e->events);
			}
		}
		else {
			ERR("the fd %d is not in session %d", fd, id);
			if (epoll_ctl(ctx->epfd, EPOLL_CTL_DEL, fd, NULL)) {
				ERR("del fd %d from send epoll failed\n", fd);
			}
			ctx->nepollevt--;
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
	assert(ctx);

	/* we only process @ctx->maxconns client connection */
	while (!_g_stop) {
		_proxy_epoll(ctx);
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
	int cpu;
	char ipstr[IP_STR_LEN];
	char plstr[4096];
	policy_t *pl;
	int i;
	int j;

	if (!arg) {
		ERR("invalid arguments\n");
		return -1;
	}
	
	memset(&ctx, 0, sizeof(ctx));
	memcpy(ctx.policies, arg->policies, arg->npolicy * sizeof(policy_t));
	ctx.npolicy = arg->npolicy;
	ctx.max = arg->max;
	ctx.use_splice = arg->use_splice;
	ctx.use_nb_splice = arg->use_nb_splice;
	ctx.bind_cpu = arg->bind_cpu;
	ctx.bind_cpu_algo = arg->bind_cpu_algo;
	ctx.bind_cpu_ht = arg->bind_cpu_ht;
	_g_index = arg->index;
	_g_dbglvl = arg->dbglvl;

	DBG("started\n");

	for (i = 0; i < arg->npolicy; i++) {
		pl = &arg->policies[i];
		memset(plstr, 0, sizeof(plstr));
		snprintf(plstr, sizeof(plstr) - 1, 
			 "work[%d]: policy<%d>: %s", _g_index, pl->index, 
			 ip_port_to_str(&pl->httpaddr, ipstr, IP_STR_LEN));
		for (j = 0; j < pl->nrsaddr; j++) {
			strcat(plstr, ",");
			strcat(plstr, 
			       ip_port_to_str(&pl->rsaddrs[j], ipstr, IP_STR_LEN));
		}
		//printf("%s\n", plstr);
	}

	if (ctx.bind_cpu) {
		cpu = cproc_bind_cpu(getpid(), _g_index, 
				      ctx.bind_cpu_algo,
				      ctx.bind_cpu_ht);
		if (cpu < 0) {
			ERR("bind CPU failed");
			exit(0);
		}

		//printf("bind to cpu %d\n", cpu);
	}

	if (_proxy_init(&ctx)) {
		_proxy_free(&ctx);
		return -1;
	}
	
	_proxy_loop(&ctx);

	_proxy_free(&ctx);

	return 0;
}


