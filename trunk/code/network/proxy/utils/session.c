/**
 *	@file	session.c
 *
 *	@brief	Session APIs implement.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date
 */

#include "sock_util.h"
#include "session.h"
#include "svrpool.h"
#include "policy.h"
#include "proxy.h"
#include "trapt_util.h"
#include "proxy_debug.h"

#define	SFLOW(level, fmt, args...)		\
	FLOW(level, "%s(%04x) %d "fmt,		\
	     c->side, c->flags, c->fd, ##args)

int 
session_init(session_t *s)
{
	if (!s)
		ERR_RET(-1, "invalid argument\n");

	s->worker = NULL;
	s->thread = NULL;
	s->policy = NULL;
	s->svrdata = NULL;

	conn_init(&s->conns[0], s, 0, "client");
	conn_init(&s->conns[1], s, 1, "server");

	CBLIST_INIT(&s->lfd);
	CBLIST_INIT(&s->svr);

	CBLIST_INIT(&s->request);
	CBLIST_INIT(&s->response);
	
	s->fparse_func = NULL;
	s->getsvr_func = NULL;
	s->parse_func = NULL;
	s->forward_func = NULL;

	return 0;
}

int 
session_free(session_t *s, connection_t *c)
{
	thread_t *ti;
	policy_t *pl;
	connection_t *peer;

	if (!s || !c)
		ERR_RET(-1, "invalid argument\n");

	assert(s->worker);
	assert(s->thread);
	assert(s->policy);

	ti = s->thread;
	pl = s->policy;
	peer = &s->conns[(c->dir + 1) % 2];

	/* free connection */
	conn_free(c);

	/* peer already in task queue, delay to peer delete */
	if (task_in_queue(&peer->task)) {
		peer->task.task = TASK_DELETE;
		FLOW(1, "%s(%04x) %d task in queue, delete later\n", 
		     peer->side, peer->flags, peer->fd);
		return 0;
	}
	else
		conn_free(peer);
	
	assert(s->nalloced == 0);

	/* delete policy */
	policy_free(pl);

	/* delete from list */
	CBLIST_DEL(&s->lfd);
	CBLIST_DEL(&s->svr);

	/* delete svrdata */
	if (s->svrdata)
		server_free_data(s->svrdata);

	/* delete session from session pool */
	FLOW(1, "deleted\n");

	objpool_put(s);

	return 0;
}

int 
session_run_task(task_t *t)
{
	thread_t *ti;
	session_t *s;
	connection_t *c;
	connection_t *peer;

	if (!t)
		ERR_RET(-1, "invalid argument\n");

	assert(t->arg);
	c = t->arg;
	assert(c->s);
	s = c->s;
	assert(s->thread);
	ti = s->thread;
	
	peer = &s->conns[(c->dir + 1) % 2];

	switch (t->task) {

	case TASK_NONE:
		ERR_RET(-1, "invalid TASK_NONE\n");
		
	case TASK_DELETE:
		session_free(s, c);
		return 0;

	case TASK_PARSE:

		/* parse data */
		if (session_fparse(s, c)) {
			session_free(s, c);
			return -1;
		}

		/* forward data */
		if (session_forward(s, peer)) {
			session_free(s, c);
			return -1;
		}

		/* call connection send data */
		if (conn_send_data(peer->fd, 0, peer)) {
			session_free(s, c);
			break;
		}
		
		
		break;

	case TASK_SEND:
		
		/* call connection send data */
		if (conn_send_data(c->fd, 0, c)) {
			session_free(s, c);
			return -1;
		}

		break;
		
	default:
		ERR_RET(-1, "invlaid task type %d\n", t->task);
	}

	if (CONN_IS_CLOSED(c) && CONN_IS_CLOSED(peer)) {
		FLOW(1, "both side closed\n");
		session_free(s, c);
	}

        return 0;
}

int 
session_fparse(session_t *s, connection_t *c)
{
	thread_t *ti;	

	if (!s || !c)
		ERR_RET(-1, "invalid argument\n");

	assert(s->thread);
	ti = s->thread;

	if (c->dir)
		CBLIST_JOIN(&s->response, &c->in);
	else
		CBLIST_JOIN(&s->request, &c->in);

	SFLOW(1, "run fast parse\n");

	return 0;
}

int 
session_parse(session_t *s, connection_t *c)
{
	thread_t *ti;	

	if (!s || !c)
		ERR_RET(-1, "invalid argument\n");

	assert(s->thread);
	ti = s->thread;

	SFLOW(1, "run parse\n");

	return 0;
}

int 
session_get_server(session_t *s, connection_t *c)
{
	thread_t *ti;
	policy_t *pl;
	server_cfg_t *svrcfg;
	server_data_t *svrdata;
	svrpool_data_t *spdata;
	char ipstr[IP_STR_LEN];

	if (!s || !c)
		ERR_RET(-1, "invalid argument\n");

	assert(s->thread);
	assert(s->policy);
	ti = s->thread;
	pl = s->policy;

	/* choose a server */
	spdata = policy_clone_spdata(pl);
	if (!spdata)
		ERR_RET(-1, "get svrpool data failed\n");	

	if (pl->cfg.mode == PL_MODE_REVERSE) {
		svrdata = svrpool_get_rp_server(spdata);
	}
	else if (pl->cfg.mode == PL_MODE_TPROXY) {
		/* set local address as conns[0]->peer address */
		c->local = s->conns[0].peer;
		svrdata = svrpool_get_tp_server(spdata, &s->conns[0].local);
	}
	else {
		svrdata = svrpool_get_tp_server(spdata, &s->conns[0].local);
	}

	svrpool_free_data(spdata);

	if (!svrdata)
		ERR_RET(-1, "svrpool get server failed\n");

	assert(svrdata->server);
	s->svrdata = svrdata;
	svrcfg = &svrdata->server->cfg;
	SFLOW(1, "get server %s\n",  
	     ip_port_to_str(&svrcfg->address, ipstr, IP_STR_LEN));
	c->peer = svrcfg->address;

	/* alloc client-side ssl connect to server */
	if (svrcfg->ssl) {
		c->ssl = ssl_alloc(svrdata->sslctx);
		if (unlikely(!c->ssl))
			ERR_RET(-1, "alloc ssl failed\n");

	}

	return 0;
}

int 
session_forward(session_t *s, connection_t *c)
{
	int e;
	int ret;
	int wait = 0;
	fd_func cb;
	worker_t *wi;
	thread_t *ti;
	policy_t *pl;
	fd_item_t *fi;
	char ipstr1[IP_STR_LEN];
	char ipstr2[IP_STR_LEN];

	if (!s || !c)
		ERR_RET(-1, "invalid argument\n");

	assert(s->thread);
	assert(s->policy);
	assert(s->worker);
	pl = s->policy;
	wi = s->worker;
	ti = s->thread;

	if (c->dir)
		CBLIST_JOIN(&c->out, &s->request);
	else
		CBLIST_JOIN(&c->out, &s->response);

	SFLOW(1, "forward data to output\n");

	/* already connected */
	if (c->fd > 0)
		return 0;

	if (session_get_server(s, c))
		ERR_RET(-1, "session ger server failed\n");

	if (pl->cfg.mode == PL_MODE_REVERSE) {
		c->fd = sk_tcp_client_nb(&c->peer, &c->local, 0, &wait);
	}
	else if (pl->cfg.mode == PL_MODE_TPROXY) {
		c->fd = socket(AF_INET, SOCK_STREAM, 0);
		if (c->fd < 0) 
			ERR_RET(-1, "socket failed: %s\n", ERRSTR);

		sk_set_nonblock(c->fd, 1);
		sk_set_keepalive(c->fd);
		sk_set_nodelay(c->fd, 1);
		sk_set_quickack(c->fd, 1);
		sk_set_mark(c->fd, 100);

		c->fd = sk_tcp_client_nb(&c->peer, &c->local, 1, &wait);
	}
	else {
		tat_addr_t tataddr;
		c->fd = socket(AF_INET, SOCK_STREAM, 0);
		if (c->fd < 0) 
			ERR_RET(-1, "socket failed: %s\n", ERRSTR);

		memset(&tataddr, 0, sizeof(tataddr));
		tataddr.addr = c->peer._addr4.s_addr;
		tataddr.port = c->peer.port;
		if (tat_set_dstaddr(c->fd, &tataddr))
			ERR(-1, "trapt set dstaddr failed\n");

		sk_set_nonblock(c->fd, 1);
		sk_set_keepalive(c->fd);
		sk_set_nodelay(c->fd, 1);
		sk_set_quickack(c->fd, 1);

		if (sk_tcp_connect(c->fd, &c->peer, NULL, 0)) 
			ERR(-1, "trapt set dstaddr failed\n");
	}
	if (c->fd < 0) 
		ERR_RET(-1, "connect to %s failed\n", 
			ip_port_to_str(&c->peer, ipstr1, IP_STR_LEN));

	SFLOW(1, "connecting %s->%s\n", 
	      ip_port_to_str(&c->local, ipstr1, IP_STR_LEN),
	      ip_port_to_str(&c->peer, ipstr2, IP_STR_LEN));

	fi = fd_epoll_map(wi->fe, c->fd);
	assert(fi);
	memset(fi, 0, sizeof(*fi));
	fi->state = FD_READY;
	fi->arg = c;
	cb = conn_handshake;

	/* TCP connect not success */
	if (wait) {
		c->flags |= CONN_F_HSK;
		ret = fd_epoll_add_event(wi->fe, c->fd, FD_OUT, cb);
		SFLOW(3, "add event(write)\n");
	}
	/* TCP connect success */
	else {
		/* do SSL handshake */
		if (c->ssl) {
			if (ssl_set_fd(c->ssl, c->fd))
				ERR_RET(-1, "ssl_set_fd failed\n");
			c->flags |= CONN_F_SSLHSK;
			e = FD_OUT;
		}
		else {
			e = FD_IN;
			cb = conn_recv_data;
		}
		ret = fd_epoll_add_event(wi->fe, c->fd, e, cb);
		SFLOW(3, "add event(%s)\n", e == FD_IN ? "read" : "write");
	}
	if (ret) {
		ERR_RET(-1, "add event failed\n");
	}

	return 0;
}


