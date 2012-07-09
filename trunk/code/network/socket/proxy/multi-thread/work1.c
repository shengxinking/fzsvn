/**
 *	@file	work.c
 *
 *	@brief	the work thread, it get packet from recv thread, do 
 *		http parse and put packet to send thread.
 *		
 *	@author	Forrest.zhang	
 *
 *	@date
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

#include "packet.h"
#include "mempool.h"
#include "proxy.h"
#include "work.h"
#include "debug.h"
#include "sesspool.h"
#include "recv.h"
#include "send.h"

static int 
_work_init(thread_t *info)
{
	work_t *winfo;

	assert(info);

	winfo = malloc(sizeof(work_t));
	if (!winfo) {
		ERR("malloc work_t object failed\n");
		return -1;
	}
	memset(winfo, 0, sizeof(work_t));

	winfo->nsess = g_proxy.capacity + 1;
	winfo->sess = malloc(winfo->nsess * sizeof(work_session_t));
	if (!winfo->sess) {
		ERR("malloc memory for parse failed\n");
		free(winfo);
		return -1;
	}
	memset(winfo->sess, 0, winfo->nsess * sizeof(work_session_t));
	DBG("work(%d) alloc %d parse object\n", info->index, winfo->nsess);

	pthread_mutex_init(&winfo->lock, NULL);

	info->priv = winfo;

	return 0;
}


static void 
_work_release(thread_t *info)
{
	work_t *winfo;

	assert(info);

	if (!info->priv)
		return;
	winfo = info->priv;

	if (winfo->sess) {
		free(winfo->sess);
	}
	free(winfo);
	info->priv = NULL;
}


static int 
_work_del_session(thread_t *info, u_int32_t id)
{
	work_t *winfo;
	work_session_t *ws;
	packet_t *pkt;
	int n = 0;

	assert(info);
	assert(info->priv);
	winfo = info->priv;

	ws = &winfo->sess[id];
	if (ws->sid != id) {
		ERR("work session is error\n");
		return -1;
	}

	/* free cache */
	pkt = pktqueue_out(&ws->cli2svr);
	while (pkt) {
		mempool_put1(pkt);
		pkt = pktqueue_out(&ws->cli2svr);
		n++;
	}
//	DBG("work(%d) id %u free %d cache packet\n", info->index, id, n);
	pkt = pktqueue_out(&ws->svr2cli);
	while (pkt) {
		mempool_put1(pkt);
		pkt = pktqueue_out(&ws->svr2cli);
		n++;
	}
//	DBG("work(%d) id %u free %d cache packet\n", info->index, id, n);

	if (ws->http) {
		free(ws->http);
		ws->http = NULL;
	}

	ws->sid = 0;

	DBG("work(%d) id %u free work session\n", info->index, id);

	return n;
}


static int 
_work_free_session(thread_t *info, u_int32_t id, int refcnt)
{
	work_t *winfo;
	session_t *s;
	int n = 0;
	int m = 0;

	assert(info);
	assert(info->priv);
	winfo = info->priv;

	n = _work_del_session(info, id);

//	DBG("id %u free %d cache packet\n", id, n);

	s = sesspool_find(g_proxy.sesspools[info->index], id);
	if (!s) {
		ERR("id %u can't find session\n", id);
		return -1;
	}
	
	if (s->clifd) {
		recv_del_fd(info->index, id, s->clifd);
	}
	if (s->svrfd) {
		recv_del_fd(info->index, id, s->svrfd);
	}
	
	s->refcnt -= refcnt;
	s->refcnt -= n;
	m = s->refcnt;
	assert(s->refcnt >= 0);
	s->flags |= SESSION_DELETE;
	sesspool_put(g_proxy.sesspools[info->index], s);

	DBG("work(%d) id %u delete session, refcnt %d\n", info->index, id, m);

	return 0;
}




/**
 *	Get packet from @inq, add all them to @pq
 */
static int 
_work_get_pkt(thread_t *info)
{
	work_t *winfo;
	int n;

	assert(info);
	assert(info->priv);
	winfo = info->priv;

	pthread_mutex_lock(&winfo->lock);
	
	n = winfo->inq.len;
	if (winfo->inq.len > 0) {
		DBG("work(%d) get %d packet from inq\n", 
		    info->index, winfo->inq.len);
		pktqueue_join(&winfo->pq, &winfo->inq);
	}

	pthread_mutex_unlock(&winfo->lock);

	if (n < 1)
		usleep(10);

	return 0;
}

/**
 *	Get delete session, and delete according 
 *	work_session_t object
 */
static int 
_work_del_fd(thread_t *info)
{
	work_t *winfo;
	fd_pair_t pairs[FDPAIR_MAX];
	int size;
	int len;
	int i;
	u_int32_t id;
	session_t *s;
	int n;

	assert(info);
	assert(info->priv);
	winfo = info->priv;

	pthread_mutex_lock(&winfo->delfds.lock);

	size = winfo->delfds.size;
	if (size > 0) {
		len = sizeof(fd_pair_t) * size;
		memcpy(pairs, winfo->delfds.array, len);
		winfo->delfds.size = 0;
	}

	pthread_mutex_unlock(&winfo->delfds.lock);

	if (size < 1)
		return 0;

	for (i = 0; i < size; i++) {
		id = pairs[i].id;
		if (winfo->sess[id].sid == id) {
			n = _work_del_session(info, id);
			if (n < 1)
				continue;

			s = sesspool_find(g_proxy.sesspools[info->index], id);
			if (!s) {
				ERR("id %u can't find session\n", id);
				continue;
			}
			s->refcnt -= n;
			assert(s->refcnt >= 0);
			sesspool_put(g_proxy.sesspools[info->index], s);
		}
		else {
			ERR("id %u not found session\n", id);
		}
	}

	return 0;
}


static int 
_work_cache_pkt(thread_t *info, work_session_t *ws, packet_t *pkt, int svr2cli)
{
	work_t *winfo;

	assert(info);
	assert(info->priv);
	assert(ws);
	assert(pkt);

	winfo = info->priv;

	DBG("work(%d) id %u cache packet\n", info->index, pkt->sid);

	if (!svr2cli) {
		pktqueue_in(&ws->cli2svr, pkt);
	}
	else {
		pktqueue_in(&ws->svr2cli, pkt);
	}

	return 0;
}

static int 
_work_send_cache(thread_t *info, work_session_t *ws, int svr2cli)
{
	work_t *winfo;
	packet_t *pkt;

	assert(info);
	assert(info->priv);
	assert(ws);

	winfo = info->priv;

	if (!svr2cli) {		
		pkt = pktqueue_out(&ws->cli2svr);
		while (pkt) {
			send_add_pkt(info->index, pkt);
			pkt = pktqueue_out(&ws->cli2svr);
		}
	}
	else {
		pkt = pktqueue_out(&ws->svr2cli);
		while (pkt) {
			send_add_pkt(info->index, pkt);
			pkt = pktqueue_out(&ws->svr2cli);
		}
	}

	return 0;
}


static int 
_work_parse_pkt(thread_t *info, packet_t *pkt)
{
	work_t *winfo;
	work_session_t *ws;
	int svr2cli = 0;
	u_int32_t id = 0;
	int act;

	assert(info);
	assert(info->priv);
	assert(pkt);

	winfo = info->priv;
	id = pkt->sid;
	ws = &winfo->sess[pkt->sid];

	if (ws->sid != id) {
		
		/* create a new work session */
		if (pkt->pos == 0) {
			DBG("work(%d) id %u alloc new work session\n", 
			    info->index, pkt->sid);
			ws->sid = id;
			ws->http = malloc(sizeof(http_info_t));
			memset(ws->http, 0, sizeof(http_info_t));
		}
		else {
			ERR("id %u is deleted\n", id);
			mempool_put1(pkt);
			return -1;
		}
	}

	if (PACKET_IS_CLI2SVR(pkt->flags)) {
		act = http_parse_request(ws->http, pkt->data, 
					 pkt->len, 0);
	}
	else {
		act = http_parse_response(ws->http, pkt->data, 
					  pkt->len, 0);
		svr2cli = 1;
	}
	
	/* check parse results */
	switch (act) {
	case ACTION_ACCEPT:
		send_add_pkt(info->index, pkt);
		break;
		
	case ACTION_DENY:
		mempool_put1(pkt);
		_work_free_session(info, id, 1);
		break;
		
	case ACTION_CACHE:
		if (ws->http->state != HTTP_STE_FIN) {
			_work_cache_pkt(info, ws, pkt, svr2cli);
		}
		else {
			/* add all cache packet to send */
			_work_send_cache(info, ws, svr2cli);
			send_add_pkt(info->index, pkt);
		}
		
		break;
		
	default:
		ERR("Unkowned action %d\n", act);
		send_add_pkt(info->index, pkt);
		break;
	}

	return 0;
}


/**
 *	Parse the packet in @pq, and send it to send thread.
 */
static int 
_work_parse_pkts(thread_t *info)
{
	work_t *winfo;
	packet_t *pkt;

	assert(info);
	assert(info->priv);
	winfo = info->priv;

	if (winfo->pq.len < 1)
		return 0;

	DBG("work(%d) have %d packet\n", info->index, winfo->pq.len);

	pkt = pktqueue_out(&winfo->pq);
	while (pkt) {		
		_work_parse_pkt(info, pkt);
		pkt = pktqueue_out(&winfo->pq);
	}

	return 0;
}


static int 
_work_loop(thread_t *info)
{
	
	while (!g_proxy.stop) {

		_work_get_pkt(info);

		_work_del_fd(info);

		_work_parse_pkts(info);

	}
	
	return 0;
}


/**
 *	The work thread main function.
 *
 *	No return.
 */
void * 
work_run(void *arg)
{
	thread_t	*info;

	usleep(200);

	info = arg;
	assert(info);

	DBG("work(%d) thread start\n", info->index);
	
	if (_work_init(info))
		return NULL;

	_work_loop(info);

	_work_release(info);

	DBG("work(%d) thread exited\n", info->index);

	pthread_exit(0);
}


/**
 *	Add a new packet to work thread @index.
 *
 *	Return 0 if success, -1 on error.
 */
int 
work_add_pkt(int index, packet_t *pkt)
{
	work_t *winfo;

	if (!pkt)
		return -1;

	if (index < 0 || index >= g_proxy.nworks) {
		ERR("invalid packet index: %d\n", index);
		return -1;
	}

	winfo = g_proxy.works[index].priv;
	if (!winfo) {
		ERR("work %d didn't have priv\n", index);
		return -1;
	}

	pthread_mutex_lock(&winfo->lock);

	pktqueue_in(&winfo->inq, pkt);

	pthread_mutex_unlock(&winfo->lock);

	return 0;
}


/**
 *	Delete a session from work thread @index.
 *
 *	Return 0 if success, -1 on error.
 */
int 
work_del_id(int index, u_int32_t id)
{
	work_t *winfo;
	int ret = 0;
	int size = 0;

	if (index < 0 || index >= g_proxy.nworks) {
		ERR("invalid packet index: %d\n", index);
		return -1;
	}

	winfo = g_proxy.works[index].priv;
	if (!winfo) {
		ERR("work %d didn't have priv\n", index);
		return -1;
	}

	pthread_mutex_lock(&winfo->delfds.lock);

	if (winfo->delfds.size >= FDPAIR_MAX) {
		ERR("too many delete fd in work %d\n", winfo->delfds.size);
		ret = -1;
	}
	else {
		size = winfo->delfds.size;
		winfo->delfds.array[size].id = id;
		winfo->delfds.array[size].fd = -1;
		winfo->delfds.size++;
	}	

	pthread_mutex_unlock(&winfo->delfds.lock);

	return ret;
}


