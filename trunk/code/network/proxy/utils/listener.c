/**
 *	@file	listener.c
 *
 *	@brief	listener implement.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date
 */

#include <unistd.h>
#include <errno.h>

#define	NDEBUG

#include "sock_util.h"
#include "session.h"
#include "worker.h"
#include "listener.h"
#include "policy.h"
#include "proxy.h"
#include "trapt_util.h"
#include "tproxy_util.h"
#include "proxy_debug.h"

static int 
_ltn_accept(int fd, listener_fd_t *lfd, listener_data_t *ltndata)
{
	int ret;
	int flags = 0;
	SSL *ssl = NULL;
	int clifd = -1;
	fd_func cb;
	fd_item_t *fi;
	session_t *s = NULL;
	policy_t *pl;
	worker_t *wi;
	thread_t *ti;
	ip_port_t cliaddr;
	ip_port_t svraddr;
	char ipstr1[IP_STR_LEN];
	char ipstr2[IP_STR_LEN];

	if (fd < 1 || !lfd)
		ERR_RET(-1, "invalid argument\n");

	assert(lfd->worker);
	assert(lfd->thread);
	assert(fd == lfd->fd);

	wi = lfd->worker;
	ti = lfd->thread;
	pl = lfd->policy;

	clifd = sk_tcp_accept_nb(fd, &cliaddr, &svraddr);
	if (clifd < 0) {
		if (unlikely(errno != EAGAIN))
			ERR_RET(-1, "accept %d failed: %s\n", fd, ERRSTR);
		else
			return 0;
	}
	
	/* set socket options */
	sk_set_nodelay(clifd, 1);
	sk_set_quickack(clifd, 1);
	sk_set_keepalive(clifd);
	//sk_set_usertimeout(clifd, 5);
	
	/* alloc session */
	s = objpool_get(wi->ssnpool);
	if (unlikely(!s)) {
		ERR("alloc session failed\n");
		goto err_free;
	}

	/* init session */
	ret = session_init(s);
	if (unlikely(ret)) {
		ERR("init session(%p) failed\n", s);
		goto err_free;
	}
	s->sid = wi->next_sid;
	wi->next_sid++;

	FLOW(1, "client(%04x) %d accepted %s->%s\n", 
	     flags, clifd,
	     ip_port_to_str(&cliaddr, ipstr1, IP_STR_LEN),
	     ip_port_to_str(&svraddr, ipstr2, IP_STR_LEN));

	/* alloc ssl object */
	if (ltndata->sslctx) {
		ssl = ssl_alloc(ltndata->sslctx);
		if (unlikely(!ssl)) {
			ERR("ssl_alloc failed\n");
			goto err_free;
		}
		if (unlikely(ssl_set_fd(ssl, clifd))) {
			ERR("ssl_set_fd failed\n");
			goto err_free;
		}
		cb = conn_handshake;
		flags = CONN_F_SSLHSK;
		FLOW(1, "client(%04x) %d alloc ssl(%p)\n", 
		     flags, clifd, ssl);		
	}
	else 
		cb = conn_recv_data;

	/* set fd_item */
	fi = fd_epoll_map(wi->fe, clifd);
	assert(fi);
	fi->state = FD_READY;
	fi->arg = &s->conns[0];

	/* add to fd_epoll event */
	ret = fd_epoll_add_event(wi->fe, clifd, FD_IN, cb);
	if (unlikely(ret)) {
		ERR("add fd event failed\n");
		goto err_free;
	}
	FLOW(3, "client(%04x) %d add event read\n", flags, clifd);
	
	/* set connection[0] */
	s->conns[0].fd = clifd;
	s->conns[0].ssl = ssl;
	s->conns[0].peer = cliaddr;
	if (pl->cfg.mode == PL_MODE_TRAPT) {
		tat_addr_t tataddr;
		if (tat_get_dstaddr(clifd, &tataddr))
			goto err_free;
		IP_ADDR_SET_V4((ip_addr_t *)&s->conns[0].local, tataddr.addr);
		s->conns[0].local.port = tataddr.port;
	}
	else 
		s->conns[0].local = svraddr;
	s->conns[0].flags |= flags;

	s->worker = wi;
	s->thread = ti;
	s->policy = policy_clone(lfd->policy);
	CBLIST_ADD_TAIL(&lfd->ssnlist, &s->lfd);

	return 0;

	
err_free:
	
	if (s)
		objpool_put(s);

	if (ssl)
		ssl_free(ssl);

	if (clifd > 0)
		close(clifd);

	return -1;
}


listener_t * 
listener_alloc(void)
{
	listener_t *ltn;

	ltn = calloc(1, sizeof(*ltn));
	if (!ltn)
		ERR_RET(NULL, "calloc memory for listener_t failed\n");

	CBLIST_INIT(&ltn->list);

	return ltn;
}

listener_t * 
listener_clone(listener_t *ltn)
{
	if (!ltn)
		ERR_RET(NULL, "invalid argument\n");

	__sync_fetch_and_add(&ltn->refcnt, 1);

	return ltn;
}

int 
listener_free(listener_t *ltn)
{
	int refcnt;

	if (!ltn)
		ERR_RET(-1, "invalid argument\n");

	refcnt = __sync_fetch_and_sub(&ltn->refcnt, 1);

	if (refcnt == 0) {
		free(ltn);
	}

	return 0;
}

int 
listener_print(const listener_t *ltn, const char *prefix)
{
	char ipstr[IP_STR_LEN];
	const listener_cfg_t *ltncfg;

	if (!ltn)
		ERR_RET(-1, "invalid argument\n");

	ltncfg = &ltn->cfg;

	printf("%slistener(%p) <%s>:\n", prefix, ltn, ltncfg->name);
	printf("%s\taddress:        %s\n", prefix, 
	       ip_port_to_str(&ltncfg->address, ipstr, IP_STR_LEN));
	printf("%s\tssl:            %d\n", prefix, ltncfg->ssl);
	printf("%s\tcertset:        %p\n", prefix, ltncfg->cert);

	return 0;
}

listener_data_t * 
listener_alloc_data(listener_t *ltn)
{
	listener_data_t *ltndata;

	if (!ltn)
		ERR_RET(NULL, "invalid argument\n");

	ltndata = calloc(1, sizeof(*ltndata));
	if (!ltndata) 
		ERR_RET(NULL, "calloc memory for listener_data failed\n");

	if (ltn->cfg.ssl && ltn->cfg.cert) {
		ltndata->sslctx = certset_alloc_ctx(ltn->cfg.cert, SSL_SD_SERVER);
		if (!ltndata->sslctx) {
			free(ltndata);
			ERR_RET(NULL, "alloc ssl context failed\n");
		}
	}

	return ltndata;
}

listener_data_t * 
listener_clone_data(listener_data_t *ltndata)
{
	if (!ltndata)
		ERR_RET(NULL, "invalid argument\n");

	__sync_fetch_and_add(&ltndata->refcnt, 1);

	return ltndata;
}

int 
listener_free_data(listener_data_t *ltndata)
{
	int refcnt;

	if (!ltndata)
		ERR_RET(-1, "invalid argument\n");

	refcnt = __sync_fetch_and_sub(&ltndata->refcnt, 1);

	if (refcnt == 0) {
		if (ltndata->sslctx)
			ssl_ctx_free(ltndata->sslctx);
		free(ltndata);
	}

	return 0;
}

listener_fd_t * 
listener_alloc_fd(listener_t *ltn, int mode)
{
	listener_fd_t *lfd;

	if (!ltn)
		ERR_RET(NULL, "invalid argument\n");	

	lfd = calloc(1, sizeof(*lfd));
	if (!lfd)
		ERR_RET(NULL, "calloc memory for listener_fd failed\n");

	lfd->address = ltn->cfg.address;
	CBLIST_INIT(&lfd->list);
	CBLIST_INIT(&lfd->ssnlist);

	if (mode == PL_MODE_REVERSE || mode == PL_MODE_TRAPT)
		lfd->fd = sk_tcp_server(&lfd->address, 1, 0);
	else 
		lfd->fd = sk_tcp_server(&lfd->address, 1, 1);
	if (lfd->fd < 0) {
		free(lfd);
		ERR_RET(NULL, "create server socket failed\n");
	}
	sk_set_nonblock(lfd->fd, 1);
	sk_set_quickack(lfd->fd, 1);
	sk_set_nodelay(lfd->fd, 1);
	sk_set_keepalive(lfd->fd);

	if (mode == PL_MODE_TPROXY)
		sk_set_mark(lfd->fd, 100);

	return lfd;
}

int 
listener_free_fd(listener_fd_t *lfd)
{
	session_t *s, *bk;

	if (!lfd)
		ERR_RET(-1, "invalid argument\n");

	CBLIST_FOR_EACH_SAFE(&lfd->ssnlist, s, bk, lfd) {
		CBLIST_DEL(&s->lfd);
		session_free(s, &s->conns[0]);
	}

	if (lfd->policy) 
		policy_free(lfd->policy);

	if (lfd->fd > 0)
		close(lfd->fd);
	
	free(lfd);

	return 0;
}

int 
listener_accept(int fd, int e, void *arg)
{
	int i;
	int ret = 0;
	worker_t *wi;
	policy_t *pl;
	listener_fd_t *lfd;
	listener_data_t *ltndata;

	if (unlikely(fd < 0 || !e || !arg))
		ERR_RET(-1, "invalid argument\n");

	lfd = arg;
	assert(lfd->policy);
	assert(lfd->worker);

	pl = lfd->policy;
	wi = lfd->worker;

	ltndata = policy_clone_ltndata(pl);

	/* accept client */
	for (i = 0; i < wi->naccept; i++) {
		ret = _ltn_accept(fd, lfd, ltndata);
		if (unlikely(ret))
			break;
	}

	listener_free_data(ltndata);

	return ret;
}





