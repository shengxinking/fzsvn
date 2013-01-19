/**
 *	@file	accept.c
 *
 *	@brief	it's accept thread, it accept client connection,
 *		and dispatch the client fd to recv thread(recv.c).
 *
 *	@author	Forrest.zhang	
 *
 *	@date	2008-07-30
 */

#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/poll.h>

#include "debug.h"
#include "thread.h"
#include "sock_util.h"
#include "proxy.h"

#define	NDEBUG		1

/**
 *	Alloc and initate accept thread global resource
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_accept_init(thread_t*info)
{
	accept_t *ai;

	if (!info) {
		ERR("invalid param\n");
		return -1;
	}

	ai = malloc(sizeof(accept_t));
	if (!ai) {
		ERR("malloc error %s\n", ERRSTR);
		return -1;
	}
	memset(ai, 0, sizeof(accept_t));

	ai->httpfd = sk_tcp_server(&g_proxy.httpaddr);
	if (ai->httpfd < 0) {
		ERR("can't creat HTTP listen socket\n");
		return -1;
	}
	sk_set_nonblock(ai->httpfd, 1);
	DBG("accept(%d) create HTTP socket %d\n", info->index, ai->httpfd);

#if 0
	ai->httpsfd = sk_tcp_server(&g_proxy.httpsaddr);
	if (ai->httpsfd < 0) {
		ERR("can't create HTTPS listen socket\n");
		return -1;
	}
	sk_set_nonblock(ai->httpsfd, 1);
	DBG("accept(%d) create HTTPS socket %d\n", info->index, ai->httpsfd);
#endif

	info->priv = ai;

	return 0;
}

/**
 *	Release resource alloced by @_accept_initiate.
 *
 *	No return.
 */
static void 
_accept_release(thread_t *info)
{
	accept_t *ai;

	if (!info || !info->priv) {
		return;
	}

	ai = info->priv;
	
	if (ai->httpfd > 0) {
		close(ai->httpfd);
		DBG("accept close HTTP socket %d\n", ai->httpfd);
	}

	if (ai->httpsfd > 0) {
		close(ai->httpsfd);
		DBG("accept close HTTPS socket %d\n", ai->httpsfd);
	}

	free(ai);
	info->priv = NULL;
}

/**
 *	Accept a client connect, and assign it to a work thread.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_accept_client(thread_t *info, int fd, int ssl, ip_port_t *dip)
{
	accept_t *ai;
	ip_port_t ip;
	int clifd;
	workfd_t wfd;
	char buf[512], buf1[512];

	assert(info);
	assert(info->priv);
	assert(dip);

	ai = info->priv;

	clifd = sk_accept(fd, &ip); 
	if (clifd < 0) {
		ERR("accept client error: %s\n", ERRSTR);
		return -1;
	}

	/* check concurrency is exceed or not */
	if (g_proxy.stat.nlive >= g_proxy.max) {
		close(clifd);
		return -1;
	}
	
	g_proxy.stat.naccept++;
	if (ssl) g_proxy.stat.nhttps++;
	else g_proxy.stat.nhttp++;
	
	FLOW(1, "accept[%d]: client %d accept (%s->%s)\n",  
	     info->index, clifd, 
	     ip_port_to_str(&ip, buf, sizeof(buf)), 
	     ip_port_to_str(dip, buf1, sizeof(buf1)));
	
	/* assigned client to recv thread */
	wfd.fd = clifd;
	wfd.is_ssl = ssl ? 1 : 0;
	if (work_add_session(ai->index, &wfd)) {
		ERR("accept[%d] client %d add to work %d failed\n", 
		    info->index, clifd, ai->index);
		close(clifd);
		return -1;
	}
	
	FLOW(1, "accept[%d]: ssn[%d] assign to work %d\n", 
	     info->index, s->id, ai->index);

	/* update statistic data */
	proxy_stat_lock();

	g_proxy.stat.nlive++;
	if (ssl) g_proxy.stat.nhttpslive++;
	else g_proxy.stat.nhttplive++;
	
	proxy_stat_unlock();

	/* rotate work index */
	ai->index++;
	if (ai->index >= g_proxy.nwork)
		ai->index = 0;
		
	return 0;
}



/**
 *	Accept connection from client, and assign the client 
 *	fd to work thread.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_accept_loop(thread_t*info)
{
	accept_t *ai;
	struct pollfd pfds[2];
	int npfd = 0;
	int n;
	int i;

	assert(info);
	assert(info->priv);

	ai = info->priv;

	/* add http fd */
	if (ai->httpfd > 0) {
		pfds[npfd].fd = ai->httpfd;
		pfds[npfd].events = POLLIN;
		npfd++;
	}

	/* add https fd */
	if (ai->httpsfd > 0) {
		pfds[npfd].fd = ai->httpsfd;
		pfds[npfd].events = POLLIN;
		npfd++;
	}

	while (!g_proxy.stop) {
		
		n = poll(pfds, npfd, ACCEPT_TIMEOUT);
		
		if (n < 0) {
			ERR("poll error: %s\n", ERRSTR);
			continue;
		}
		if (n == 0) {
			continue;
		}

		for (i = 0; i < n; i++) {
			if (pfds[i].fd == ai->httpfd) {
				_accept_client(info, ai->httpfd, 0, 
					       &g_proxy.httpaddr);
			}
			else if (pfds[i].fd == ai->httpsfd) {
				_accept_client(info, ai->httpsfd, 1, 
					       &g_proxy.httpsaddr);
			}
			else {
				ERR("unknowed fd %d in poll\n", pfds[i].fd);
			}
		}
	}
	
	return 0;
}


void * 
accept_run(void *args)
{
	thread_t *info = NULL;

	usleep(200);
	
	info = args;
	assert(info);

	DBG("accept(%d) thread start\n", info->index);

	if (_accept_init(info)) {
		if (g_proxy.main_tid > 0) {
			pthread_kill(g_proxy.main_tid, SIGINT);
		}
		pthread_exit(0);
	}

	_accept_loop(info);

	_accept_release(info);

	DBG("accept(%d) thread exited\n", info->index);

	pthread_exit(0);
}




