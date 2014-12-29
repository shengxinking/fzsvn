/**
 *	file	proxy.c
 *
 *	brief	The proxy main APIs, it accept connections, recv data, 
 *		parse data and send data.
 *
 *	author	Forrest.zhang
 */

#define _GNU_SOURCE
//#define NDEBUG

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
#include "session.h"

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
		printf("work[%d]: policy<%d> ssn(%p) "fmt,	\
		       _g_index, pl->index, s, ##args);	\
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

static int _proxy_accept_client(int fd, int events, void *arg);
static int _proxy_do_handshake(int fd, int events, void *arg);
static int _proxy_recv_data(int fd, int events, void *arg);
static int _proxy_send_data(int fd, int events, void *arg);
static int _proxy_select_server(proxy2_t *py, policy_t *pl, session2_t *s);
static int _proxy_run_task(int fd, fd_task_t task, void *arg);

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
_proxy_init(proxy2_t *py)
{
	struct rlimit rlim;
	policy_t *pl;
	fd_item_t *fi;
	int fdmax;
	int i;

	assert(py);
	
	/* set SIGUSR1 as quit signal*/
	signal(SIGUSR1, _proxy_stop);
	signal(SIGINT, SIG_IGN);

	fdmax = py->max * 2 + 10;

	/* set max file descriptor to 2 * maxsess if maxsess > 512 */
	if (fdmax > 1024) {
		rlim.rlim_cur = fdmax;
		rlim.rlim_max = fdmax;
		
		if (setrlimit(RLIMIT_NOFILE, &rlim)) {
			ERR("setrlimit(RLIMIT_NOFILE) error: %s", ERRSTR);
			return -1;
		}
		
		DBG("set max file descriptor to %d\n", (2 * py->max));
	}

	py->ssnpool = objpool_alloc(sizeof(session2_t), 1024, 0);
	if (!py->ssnpool) {
		ERR("create objpool for session error\n");
		return -1;
	}
	DBG("create memory pool for session2 %p\n", py->ssnpool);
	
	py->pktpool = objpool_alloc(4096, 1024, 0);
	if (!py->ssnpool) {
		ERR("create objpool for packet error\n");
		return -1;
	}

	py->fe = fd_epoll_alloc(fdmax, 1000);
	if (!py->fe) {
		ERR("fd_epoll_alloc failed\n");
		return -1;
	}
	DBG("alloc fd_epoll maxfd is %d\n", py->fe->maxfd);

	for (i = 0; i < py->npolicy; i++) {
		pl = &py->policies[i];
		pl->httpfd = sk_tcp_server(&pl->httpaddr, 1);
		if (pl->httpfd < 0) {
			ERR("create http listen socket failed\n");
			return -1;
		}
		sk_set_nonblock(pl->httpfd, 1);

		fi = fd_epoll_map(py->fe, pl->httpfd);
		assert(fi);
		fd_epoll_init_item(fi);
		fi->state = FD_READY;
		fi->priv = pl;

		if (fd_epoll_alloc_update(py->fe, pl->httpfd, 
					  EPOLLIN, _proxy_accept_client)) 
		{
			ERR("fd_epoll_alloc_update add %d failed\n", pl->httpfd);
			return -1;
		}

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
_proxy_free(proxy2_t *py)
{
	policy_t *pl;
	int i;

	assert(py);

	for (i = 0; i < py->npolicy; i++) {
		pl = &py->policies[i];
		if (pl->httpfd > 0)
			close(pl->httpfd);
	}

	if (py->ssnpool)
		objpool_free(py->ssnpool);
	
	if (py->fe)
		fd_epoll_free(py->fe);

	DBG("all resource is freed\n");
}

/**
 *	Alloc a new session
 *
 *	Return 0 if success, -1 on error.
 */
static session2_t * 
_proxy_alloc_session(proxy2_t *py, policy_t *pl, int fd)
{
	session2_t *s;

	assert(py);
	assert(pl);
	assert(fd > 0);
	
	/* alloc new session */
	s = objpool_get(py->ssnpool);
	if (unlikely(!s)) {
		ERR("get session from ssnpool failed\n");
		goto out_failed;
	}

	/* init session */
	s->cli_fd = fd;
	s->svr_fd = -1;

	s->cli_cache.input = NULL;
	s->cli_cache.is_blocked = 0;
	s->cli_cache.is_shutrd = 0;
	s->cli_cache.is_shutwr = 0;

	s->svr_cache.output = NULL;
	s->svr_cache.is_blocked = 0;
	s->svr_cache.is_shutrd = 0;
	s->svr_cache.is_shutwr = 0;
	
	s->flags = 0;
	s->proxy = py;
	s->policy = pl;

	return s;

out_failed:

	if (s) 
		objpool_put(s);

	return NULL;
}

/**
 *	Free session @s, release all resource used by @s.
 *
 *	No return.
 */
static void 
_proxy_free_session(proxy2_t *py, policy_t *pl, session2_t *s)
{
	int fd;
	fd_item_t *fi;
	
	assert(py);
	assert(pl);
	assert(s);

	/* close client socket */
	if (s->cli_fd > 0) {
		fd = s->cli_fd;

		fi = fd_epoll_map(py->fe, fd);
		assert(fi);
		fd_epoll_init_item(fi);

		close(fd);
		FLOW(1, "client %d closed\n", fd);
	}

	/* close server socket */
	if (s->svr_fd > 0) {
		fd = s->svr_fd;

		fi = fd_epoll_map(py->fe, fd);
		assert(fi);
		fd_epoll_init_item(fi);

		close(fd);
		FLOW(1, "server %d closed\n", fd);
	}

	if (s->cli_cache.input) {
		//ERR("client have data not parse\n");
		objpool_put(s->cli_cache.input);
	}
	if (s->cli_cache.output > 0) {
		//ERR("client have data not send\n");
		objpool_put(s->cli_cache.output);
	}
	if (s->svr_cache.input > 0) {
		//ERR("server have data not parse\n");
		objpool_put(s->svr_cache.input);
	}
	if (s->svr_cache.output > 0) {
		//ERR("server have data not send\n");
		objpool_put(s->svr_cache.output);
	}

	/* delete session from session pool */
	FLOW(1, "deleted\n");

	objpool_put(s);

	py->stat.nhttplive--;
	py->stat.nlive--;
}


/**
 *	Accept client TCP connection, and add it to epoll socket and
 *	session pool.
 *
 *	Return 0 if accept success, -1 on error.
 */
static int
_proxy_accept_client(int fd, int events, void *arg)
{
	proxy2_t *py;
	policy_t *pl;
	ip_port_t cliaddr;
	int clifd;
	session2_t *s = NULL;
	char ipstr1[IP_STR_LEN], ipstr2[IP_STR_LEN];
	int i;
	fd_item_t *fi;

	assert(fd > 0);
	assert(arg);
	
	pl = (policy_t *)arg;
	py = pl->proxy;
	assert(py);
	
	if (unlikely(events & EPOLLERR))
		return -1;

	for (i = 0; i < 8; i++) {
		clifd = sk_tcp_accept(pl->httpfd, &cliaddr);
		if (unlikely(clifd < 0)) {
			if (errno == EAGAIN || errno == EINTR) {
				return 0;
			}

			ERR("accept error: %s\n", ERRSTR);
			return -1;
		}

		/* update statistic data */
		py->stat.naccept++;

		/* set TCP option */
		sk_set_nonblock(clifd, 1);
		sk_set_nodelay(clifd, 1);
		sk_set_quickack(clifd, 1);

		/* check max connection */
		if (unlikely(py->stat.nlive >= py->max)) {
			ERR("too many client %lu max %d\n", 
			    py->stat.nlive, py->max);
			close(clifd);
			return -1;
		}

		/* alloc new session object */
		s = _proxy_alloc_session(py, pl, clifd);
		if (unlikely(!s)) {
			ERR("alloc new session failed\n");
			close(clifd);
			return -1;
		}
		
		FLOW(1, "client %d accepted(%s->%s)\n", clifd, 
		     ip_port_to_str(&cliaddr, ipstr1, IP_STR_LEN),
		     ip_port_to_str(&pl->httpaddr, ipstr2, IP_STR_LEN));

		/* map fd to fd_item, and init it */
		fi = fd_epoll_map(py->fe, clifd);
		assert(fi);
		fd_epoll_init_item(fi);
		fi->state = FD_READY;
		fi->priv = s;

		/* add new client fd to update events */
		if (unlikely(fd_epoll_alloc_update(py->fe, clifd, 
						   EPOLLIN | EPOLLRDHUP,
						   _proxy_recv_data)))
		{
			ERR("%d alloc update failed\n", clifd);
			goto out_failed;
		}
		FLOW(3, "client %d event alloc read|close\n", clifd);

#if 0
		/* try to recv data in task after accept */
		if (unlikely(fd_epoll_alloc_task(py->fe, clifd, FD_TASK_RECV, 
						 _proxy_run_task)) )
		{
			ERR("%d alloc task failed\n", clifd);
			goto out_failed;
		}
		FLOW(1, "client %d task alloc recv\n", clifd);	
#endif

		if (unlikely(_proxy_select_server(py, pl, s))) {
			ERR("select pserver failed\n");
			goto out_failed;
		}

		py->stat.nhttp++;
		py->stat.nhttplive++;
		py->stat.nlive++;

	}

	return 0;

out_failed:
	
	if (unlikely(fd_epoll_alloc_task(py->fe, clifd, FD_TASK_CLOSE, 
					 _proxy_run_task)))
	{
		ERR("alloc task failed\n");
	}

	return -1;
}

/**
 *	Select a real server and connect to it. the select algorithm
 *	is Round-Robin.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_proxy_select_server(proxy2_t *py, policy_t *pl, session2_t *s)
{
	int fd;
	int wait = 0;
	ip_port_t *rs;
	fd_item_t *fi;

	assert(py);
	assert(pl);
	assert(s);
	assert(s->cli_fd > 0);

	if (unlikely(s->svr_fd > 0))
		return 0;
	
	if (pl->pos < 0 || pl->pos >= pl->nrsaddr)
		pl->pos = 0;
	rs = &pl->rsaddrs[pl->pos];
	pl->pos++;

	fd = sk_tcp_client_nb(rs, &wait);
	if (unlikely(fd < 0)) {
		ERR("get server socket error\n");
		return -1;
	}

	/* init fd_item */
	fi = fd_epoll_map(py->fe, fd);
	assert(fi);
	fd_epoll_init_item(fi);
	fi->state = FD_READY;
	fi->priv = s;
	s->svr_fd = fd;

	/* add it to send epoll to wait connect success */
	if (wait) {
		s->svr_cache.is_blocked = 1;

		if (unlikely(fd_epoll_alloc_update(py->fe, fd, EPOLLOUT, 
						   _proxy_do_handshake))) 
		{
			ERR("alloc update failed\n");
			return -1;
		}
		FLOW(3, "server %d event alloc write\n", fd);

		FLOW(1, "server %d connect wait\n", fd);
	}
	/* add to recv epoll */
	else {
		FLOW(1, "client %d connect to server %d success\n", 
		     s->cli_fd, fd);

		sk_set_nodelay(fd, 1);
		sk_set_quickack(fd, 1);

		if (unlikely(fd_epoll_alloc_update(py->fe, fd, EPOLLIN | EPOLLRDHUP,
						   _proxy_recv_data))) 
		{
			ERR("alloc update failed\n");
			return -1;
		}
		FLOW(3, "server %d event alloc read|close\n", fd);

		FLOW(1, "server %d connect success\n", fd);
	}

	return 0;
}

static int 
_proxy_do_handshake(int fd, int events, void *arg)
{
	proxy2_t *py;
	policy_t *pl;
	fd_item_t *fi;
	session2_t *s;
	int ret = 0;

	assert(fd > 0);
	assert(arg);
	
	s = (session2_t *)arg;
	py = s->proxy;
	pl = s->policy;
	assert(py);
	assert(pl);

	fi = fd_epoll_map(py->fe, fd);
	assert(fi);

	if (unlikely(events & EPOLLERR)) {
		ERR("handshake event error\n");
		goto out_failed;
	}

	ret = sk_is_connected(fd);
	if (unlikely(ret < 0)) {
		FLOW(1, "server %d connect failed\n", fd);
		ERR("server %d connect failed\n", fd);
		goto out_failed;
	}

	FLOW(1, "server %d connect success\n", fd);

	sk_set_nodelay(fd, 1);
	sk_set_quickack(fd, 1);

	/* handshake success, change it to read/readclose */
	if (unlikely(fd_epoll_alloc_update(py->fe, fd, EPOLLIN | EPOLLRDHUP, 
					   _proxy_recv_data))) 
	{
		ERR("alloc update failed\n");
		return -1;
	}
	FLOW(3, "server %d event alloc read|close\n", fd);
	
	s->svr_cache.is_blocked = 0;
	if (unlikely(fd_epoll_alloc_task(py->fe, fd, FD_TASK_SEND, 
					 _proxy_run_task)))
	{
		ERR("alloc task failed\n");
		return -1;
	}
	FLOW(3, "server %d task alloc send\n", fd);

	return 0;

out_failed:

	if (unlikely(fd_epoll_alloc_task(py->fe, fd, FD_TASK_CLOSE, 
					 _proxy_run_task)))
	{
		ERR("alloc task failed\n");
		return -1;
	}
	FLOW(3, "server %d task alloc delete\n", fd);

	return -1;
}

static int 
_proxy_recv_data(int fd, int events, void *arg)
{
	proxy2_t *py;
	policy_t *pl;
	session2_t *s;
	session2_cache_t *cache;
	packet_t *pkt;
	int closed = 0;
	int n;
	const char *side;
	int ret = 0;

	assert(fd > 0);
	assert(arg);

	s = (session2_t *)arg;
	py = s->proxy;
	pl = s->policy;
	assert(py);
	assert(pl);

	if (fd == s->cli_fd) {
		cache = &s->cli_cache;
		side = "client";
	}
	else {
		cache = &s->svr_cache;
		side = "server";
	}

	if (!cache->input) {
		pkt = objpool_get(py->pktpool);
		if (unlikely(!pkt)) {
			ERR("alloc packet failed\n");
			goto out_free;
		}

		cache->input = pkt;
		pkt->len = 0;
		pkt->sendpos = 0;
		pkt->pos = 0;
		pkt->capacity = py->pktpool->objsize - sizeof(packet_t);
	}
	else {
		pkt = cache->input;
	}
	
	if (unlikely(events & EPOLLERR)) {
		FLOW(1, "%s %d recv error\n", side, fd);
		//ERR("recv data epoll error\n");
		ret = -1;
		goto out_free;
	}

	n = sk_recv(fd, pkt->data, pkt->capacity, &closed);
	FLOW(1, "%s %d recv %d bytes\n", side, fd, n);

	if (unlikely(n < 0))  {
		FLOW(1, "%s %d recv error\n", side, fd);
		ERR("recv data error\n");
		ret = -1;
		goto out_free;
	}

	pkt->len = n;
	if (closed || events & EPOLLRDHUP) {
		FLOW(1, "%s %d recv read shutdown, events %d\n", side, fd, events);	
		cache->is_shutrd = 1;
		if (unlikely(fd_epoll_alloc_update(py->fe, fd, 0, NULL))) {
			ERR("alloc events failed\n");
			ret = -1;
			goto out_free;
		}
		FLOW(3, "%s %d event alloc delete\n", side, fd);
	}

	if (n == 0) {
		objpool_put(pkt);
		cache->input = NULL;
	}

	/* alloc task when recv data in iocb mode */
	if (events) {
		/* add to task */
		if (unlikely(fd_epoll_alloc_task(py->fe, fd, FD_TASK_PARSE, 
						 _proxy_run_task)))
		{
			ERR("alloc task failed\n");
			ret = -1;
			goto out_free;
		}
		FLOW(3, "%s %d task alloc parse\n", side, fd);
	}

	return n;

out_free:
	
	/* only set task in event iocb */
	if (events) {
		if (unlikely(fd_epoll_alloc_task(py->fe, fd, FD_TASK_CLOSE, 
						 _proxy_run_task)))
		{
			ERR("alloc task failed\n");
			return -1;
		}
		FLOW(3, "%s %d task alloc delete\n", side, fd);
	}

	return ret;
}

static int 
_proxy_send_data(int fd, int events, void *arg)
{
	session2_t *s;
	policy_t *pl;
	proxy2_t *py;
	const char *side;
	session2_cache_t *cache;
	packet_t *pkt;
	int n;
	int flags;

	assert(fd > 0);
	assert(arg);

	s = (session2_t *)arg;
	pl = s->policy;
	py = s->proxy;

	assert(pl);
	assert(py);

	if (fd == s->cli_fd) {
		cache = &s->cli_cache;
		side = "client";
		flags = SESSION2_CLI_CLOSED;
	}
	else {
		cache = &s->svr_cache;
		side = "server";
		flags = SESSION2_SVR_CLOSED;
	}

	if (cache->is_blocked) {
		return 0;
	}
	
	if (cache->output) {
		pkt = cache->output;
		n = sk_send(fd, pkt->data, pkt->len);
		if (unlikely(n < 0)) {
			ERR("send error\n");
			goto out_failed;
		}

		if (unlikely(n != pkt->len)) {
			ERR("send need blocked\n");
			goto out_failed;
		}

		FLOW(1, "%s %d send %d bytes\n", side, fd, n);

		objpool_put(pkt);
		cache->output = NULL;
	}

	if (cache->is_shutwr) {
		shutdown(fd, SHUT_WR);
		s->flags |= flags;
		cache->is_shutwr = 0;
		FLOW(1, "%s %d send SHUT_WR\n", side, fd);
	}

	return 0;

out_failed:

	if (events) {
		if (unlikely(fd_epoll_alloc_task(py->fe, fd, FD_TASK_CLOSE, 
						 _proxy_run_task))) 
		{
			ERR("alloc task failed\n");
		}
	}

	return -1;
}

static int 
_proxy_parse_data(proxy2_t *py, policy_t *pl, session2_t *s, int fd)
{
	if (fd == s->cli_fd) {
		s->svr_cache.is_shutwr = s->cli_cache.is_shutrd;
		if (s->cli_cache.input) {
			s->svr_cache.output = s->cli_cache.input;
			s->cli_cache.input = NULL;
			FLOW(1, "client %d move packet %p to server %d\n", 
			     s->cli_fd, s->svr_cache.output, s->svr_fd);
		}
	}
	else {
		s->cli_cache.is_shutwr = s->svr_cache.is_shutrd;
		if (s->svr_cache.input) {
			s->cli_cache.output = s->svr_cache.input;
			s->svr_cache.input = NULL;
			FLOW(1, "server %d move packet %p to client %d\n", 
			     s->svr_fd, s->cli_cache.output, s->cli_fd);
		}
	}

	return 0;
}

static int 
_proxy_run_task(int fd, fd_task_t task, void *arg)
{
	proxy2_t *py;
	policy_t *pl;
	session2_t *s;
	int peerfd;
	int ret = 0;

	if (unlikely(fd < 0 || !arg))
		return -1;

	if (unlikely(!task)) {
		ERR("invalid task\n");
		return 0;
	}

	s = (session2_t *)arg;
	py = s->proxy;
	pl = s->policy;

	if (fd == s->cli_fd)
		peerfd = s->svr_fd;
	else
		peerfd = s->cli_fd;

	switch (task) {

	case FD_TASK_CLOSE:
		if (s->cli_cache.output) {
			ret = _proxy_send_data(s->cli_fd, 0, s);
		}
		if (s->svr_cache.output) {
			ret = _proxy_send_data(s->svr_fd, 0, s);
		}
		goto out_free;

	case FD_TASK_RECV:
		ret = _proxy_recv_data(fd, 0, s);
		if (unlikely(ret < 0)) goto out_free;

		ret = _proxy_parse_data(py, pl, s, fd);
		if (unlikely(ret < 0)) goto out_free;
		
		if (peerfd > 0) {
			ret = _proxy_send_data(peerfd, 0, s);
			if (unlikely(ret < 0)) goto out_free;
		}
		break;

	case FD_TASK_PARSE:
		ret = _proxy_parse_data(py, pl, s, fd);
		if (unlikely(ret < 0)) goto out_free;
		
		if (peerfd > 0) {
			ret = _proxy_send_data(peerfd, 0, s);
			if (unlikely(ret < 0)) goto out_free;
		}
		break;
	       
	case FD_TASK_SEND:
		ret = _proxy_send_data(fd, 0, s);
		if (unlikely(ret < 0)) goto out_free;
		break;

	default:
		ERR("unknowned task %d\n", task);
	}

	if (s->flags == SESSION2_CLOSED)
		goto out_free;

	return 0;

out_free:

	_proxy_free_session(py, pl, s);
	return ret;
}
		

/**
 *	The main loop of proxy, it use epoll to detect which socket
 *	need process
 *
 *	Return 0 if success, -1 on error.
 */
static int
_proxy_loop(proxy2_t *py)
{
	assert(py);

	/* process all events and cache task */
	while (!_g_stop) {
		fd_epoll_flush_update(py->fe);
		fd_epoll_poll(py->fe);
		fd_epoll_flush_task(py->fe);
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
	proxy2_t py;
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
	
	memset(&py, 0, sizeof(py));
	memcpy(py.policies, arg->policies, arg->npolicy * sizeof(policy_t));
	py.npolicy = arg->npolicy;
	for (i = 0; i < py.npolicy; i++)
		py.policies[i].proxy = &py;

	py.max = arg->max;
	py.use_splice = arg->use_splice;
	py.use_nb_splice = arg->use_nb_splice;
	py.bind_cpu = arg->bind_cpu;
	py.bind_cpu_algo = arg->bind_cpu_algo;
	py.bind_cpu_ht = arg->bind_cpu_ht;
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

	if (py.bind_cpu) {
		cpu = cproc_bind_cpu(getpid(), _g_index, 
				      py.bind_cpu_algo,
				      py.bind_cpu_ht);
		if (cpu < 0) {
			ERR("bind CPU failed");
			exit(0);
		}

		//printf("bind to cpu %d\n", cpu);
	}

	if (_proxy_init(&py)) {
		_proxy_free(&py);
		return -1;
	}
	
	_proxy_loop(&py);

	_proxy_free(&py);

	return 0;
}


