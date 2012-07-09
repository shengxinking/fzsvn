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

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <assert.h>
#include <signal.h>

#include "accept.h"
#include "debug.h"
#include "thread.h"
#include "sock.h"
#include "proxy.h"
#include "work.h"


/**
 *	Alloc and initate accept thread global resource
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_accept_init(thread_t*info)
{
	accept_t *ainfo;

	if (!info) {
		ERR("invalid param\n");
		return -1;
	}

	ainfo = malloc(sizeof(accept_t));
	if (!ainfo) {
		ERR("can't alloc memory for accept info\n");
		return -1;
	}
	memset(ainfo, 0, sizeof(accept_t));

	ainfo->listen_fd = sock_tcpsvr(g_proxy.ip, g_proxy.port);
	if (ainfo->listen_fd < 0) {
		ERR("can't creat listen socket\n");
		return -1;
	}

	DBG("accept create listen fd %d\n", ainfo->listen_fd);

	info->priv = ainfo;

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
	accept_t *ainfo;

	if (!info)
		return;

	if (info->priv) {

		ainfo = info->priv;

		if (ainfo->listen_fd > 0) {
			close(ainfo->listen_fd);
			DBG("accept close listen socket %d\n", 
			    ainfo->listen_fd);
		}

		free(info->priv);
		info->priv = NULL;
	}
}


/**
 *	Accept a client connect, and assign it to a work thread.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_accept_client(thread_t *info, int fd, int ssl)
{
	clifd = sock_accept(ainfo->listen_fd, &sip, &sport); 
	if (clifd == -2) {
		ERR("accept client error: %s\n", strerror(errno));
		continue;
	}
	if (clifd == -1) {
		sched_yield();
		continue;
	}
	
	g_proxy.stat.naccepts++;
	
	DBG("accept new TCP session: %u.%u.%u.%u:%u->%u.%u.%u.%u:%u\n",
	    sid, NIPQUAD(s->sip), ntohs(s->sport), 
	    NIPQUAD(s->dip), ntohs(s->dport));
	
	/* assigned client to recv thread */
	if (work_epoll_add_fd(sid, clifd)) {
		ERR("id %u add clifd %d to recv failed\n", sid, clifd);
		close(clifd);
	}
	
	/* rotate work index */
	ainfo->index++;
	if (ainfo->index >= g_proxy.nworks)
		ainfo->index = 0;
		
	return 0;
}


/**
 *	Accept connection from client, and assign the client fd to work thread.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_accept_loop(thread_t*info)
{
	accept_t *ainfo;
	struct pollfd pfds[2];
	int npfd;

	assert(info);
	assert(info->priv);

	ainfo = info->priv;

	while (!g_proxy.stop) {

		npfd = 0;
		if (ainfo->http_fd) {
			pfds[npfd].fd = ainfo->http_fd;
			pfds[npfd].events = POLLIN;
			npfd++;
		}

		if (ainfo->https_fd) {
			pfds[npfd].fd = ainfo->https_fd;
			pfds[npfd].events = POLLIN;
			npfd++;
		}

		n = poll(pfds, npfd, ACCEPT_TIMEOUT);

		if (n < 0) {
			ERR("poll error: %s\n", strerror(errno));
			continue;
		}
		if (n == 0) {
			continue;
		}

		for (i = 0; i < n; i++) {
			if (pfds[i].fd == ainfo->http_fd) {
				_accept_client(info, ainfo->http_fd, 0);
			}
			else if (pfds[i].fd == ainfo->https_fd) {
				_accept_client(info, ainfo->https_fd, 1);
			}
			else {
				ERR("unknowed fd %d in accept poll\n", pfds[i].fd);
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

	DBG("accept thread start\n");

	if (_accept_init(info)) {
		if (g_proxy.main_tid > 0) {
			pthread_kill(g_proxy.main_tid, SIGINT);
		}
		pthread_exit(0);
	}

	_accept_loop(info);

	_accept_release(info);

	DBG("accept thread exited\n");

	pthread_exit(0);
}




