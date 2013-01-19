/**
 *	@file	proxy_main.c
 *
 *	@brief	tcp proxy main API.
 *
 *	@author	Forrest.zhang	
 */

#include <sys/time.h>
#include <sys/resource.h>
#include <signal.h>
#include <sys/epoll.h>
#include <errno.h>
#include <unistd.h>

#include "proxy.h"
#include "debug.h"

proxy_t		g_proxy;

/**
 *	The stop signal of proxy, it interrupt accept thread, 
 *	and set @g_proxy.stop as 1, all other thread will 
 *	check @g_proxy.stop
 *	to decide stop running.
 *
 *	No return.
 */
static void 
_sig_int(int signo)
{
	if (signo != SIGINT)
		return;

	if (pthread_self() == g_proxy.main_tid) {
		DBG("recved stop signal SIGINT\n");
		g_proxy.stop = 1;
		if (g_proxy.accept.tid) {
			pthread_kill(g_proxy.accept.tid, SIGINT);
		}
	}
	else {
		if (g_proxy.stop == 0)
			pthread_kill(g_proxy.main_tid, SIGINT);
	}
}

/**
 *	Alloc resource for proxy.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_proxy_init(proxy_t *ctx)
{
	struct sigaction act;
	struct rlimit rlim;

	ctx->main_tid = pthread_self();

	/* using SIGINT as stop signal */ 
	act.sa_handler = _sig_int;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_restorer = NULL;
	if (sigaction(SIGINT, &act, NULL)) {
		ERR("sigaction error: %s\n", strerror(errno));
		return -1;
	}

	/* ignore SIGPIPE signal */
	act.sa_handler = SIG_IGN;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_restorer = NULL;
	if (sigaction(SIGPIPE, &act, NULL)) {
		ERR("sigaction error: %s\n", strerror(errno));
		return -1;
	}

	/* set file descriptor limit */
	if (ctx->capacity > 512) {
		rlim.rlim_cur = ctx->capacity * 2;
		rlim.rlim_max = ctx->capacity * 2;
		if (setrlimit(RLIMIT_NOFILE, &rlim)) {
			ERR("setrlimit error: %s\n", 
			    strerror(errno));
			return -1;
		}
	}

	
	ctx->pktpool = objpool_alloc(PKT_SIZE, ctx->capacity, 1);
	if (!ctx->pktpool) {
		ERR("alloc packet pool failed\n");
		return -1;
	}

	ctx->sesstbl = session_table_alloc(ctx->capacity);
	if (!ctx->sesstbl) {
		ERR("alloc session table failed\n");
		return -1;
	}

	return 0;
}


/**
 *	Release resource for proxy.
 *
 *	No return.
 */
static void 
_proxy_release(proxy_t *ctx)
{
	if (!ctx)
		return;

	if (ctx->pktpool) {
		objpool_free(ctx->pktpool);
		ctx->pktpool = NULL;
	}

	if (ctx->sesstbl) {
		session_table_free(ctx->sesstbl);
		ctx->sesstbl = NULL;
	}
}


/**
 *	The main loop function of proxy main thread.
 * 
 * 	Return 0 if success, -1 on error.
 */
static int 
_proxy_loop(proxy_t *ctx)
{
	while (!ctx->stop) {
		sleep(1);
	}

	return 0;
}


void 
proxy_print_stat(proxy_t *ctx)
{
	printf("-------proxy statistic---------\n");
	printf("nhttp:      %lu\n", ctx->stat.nhttp);
	printf("nhttps:     %lu\n", ctx->stat.nhttps);
	printf("naccept:    %lu\n", ctx->stat.naccept);
	printf("nhttplive:  %lu\n", ctx->stat.nhttplive);
	printf("nhttpslive: %lu\n", ctx->stat.nhttpslive);
	printf("nlive:      %lu\n", ctx->stat.nlive);
	printf("nclirecv:   %lu\n", ctx->stat.nclirecv);
	printf("nclisend:   %lu\n", ctx->stat.nclisend);
	printf("nsvrrecv:   %lu\n", ctx->stat.nsvrrecv);
	printf("nsvrsend:   %lu\n", ctx->stat.nsvrsend);
	printf("ncliclose:  %lu\n", ctx->stat.ncliclose);
	printf("nclierror:  %lu\n", ctx->stat.nclierror);
	printf("nsvrclose:  %lu\n", ctx->stat.nsvrclose);
	printf("nsvrerror:  %lu\n", ctx->stat.nsvrerror);
	printf("-------------------------------\n");
}


int 
proxy_main(ip_port_t *svraddr, ip_port_t *psvraddr, 
	   size_t max, size_t nworks)
{
	int i;

	if (!svraddr || !psvraddr)
		return -1;

	g_proxy.svraddr = *svraddr;
	g_proxy.rsvraddr = *rsvraddr;
	g_proxy.max = max;
	g_proxy.nworks = nworks;

	if (_proxy_init(&g_proxy))
		return -1;

	thread_create(&g_proxy.accept, THREAD_ACCEPT, 
		      accept_run, 0);

	for (i = 0; i < nworks; i++) {
		thread_create(&g_proxy.works[i], THREAD_WORK, 
			      work_run, i);
	}

	_proxy_loop(&g_proxy);

	thread_join(&g_proxy.accept);
	for (i = 0; i < nworks; i++) {
		thread_join(&g_proxy.works[i]);
	}

	proxy_print_stat(&g_proxy);

	_proxy_release(&g_proxy);

	return 0;
}










