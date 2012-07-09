/**
 *	@file	proxy.c
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
#include "mempool.h"
#include "sesspool.h"
#include "packet.h"

extern void *accept_run(void *);
extern void *recv_run(void *);
extern void *send_run(void *);
extern void *work_run(void *);

/**
 *	The global proxy variable.
 */
proxy_t		g_proxy;

/**
 *	The stop signal of proxy, it interrupt accept thread, and
 *	set @g_proxy.stop as 1, all other thread will check @g_proxy.stop
 *	to decide stop running.
 *
 *	No return.
 */
static void 
_sig_int(int signo)
{
	if (signo == SIGINT) {
		if (pthread_self() == g_proxy.main_tid) {
			DBG("recved stop signal SIGINT\n");
			g_proxy.stop = 1;
		}
		else {
			if (g_proxy.stop == 0)
				pthread_kill(g_proxy.main_tid, SIGINT);
		}
	}
}


static int 
_proxy_init(proxy_t *ctx)
{
	struct sigaction act;
	struct rlimit rlim;
	int i;

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
		rlim.rlim_cur = ctx->capacity * 2 + 10;
		rlim.rlim_max = ctx->capacity * 2 + 10;
		if (setrlimit(RLIMIT_NOFILE, &rlim)) {
			ERR("setrlimit error: %s\n", strerror(errno));
			return -1;
		}
	}

	ctx->main_tid = pthread_self();
	ctx->pktpool = mempool_alloc(PKT_SIZE, ctx->capacity, 1);
	for (i = 0; i < g_proxy.nworks; i++) {
		ctx->sesspools[i] = sesspool_alloc(ctx->capacity);
		if (!ctx->sesspools[i]) {
			ERR("alloc session pool failed\n");
		}
	}

	return 0;
}


static int 
_proxy_release(proxy_t *ctx)
{
	int i;

	if (!ctx)
		return -1;

	if (ctx->pktpool) {
		DBG("pktpool: freed %d capacity %d\n", 
		    ctx->pktpool->nfreed, ctx->pktpool->size); 
		mempool_free(ctx->pktpool);
		ctx->pktpool = NULL;
	}
	for (i = 0; i < g_proxy.nworks; i++) {
		if (ctx->sesspools[i]) {
		printf("sesspool: size %d capacity %d\n", 
		       ctx->sesspools[i]->size, ctx->sesspools[i]->capacity);
		sesspool_free(ctx->sesspools[i]);
		ctx->sesspools[i] = NULL;
		}
	}

	return 0;
}


static int 
_proxy_loop(proxy_t *ctx)
{
	while (!ctx->stop) {
		sleep(1);
	}

	return 0;
}

/**
 *	output the proxy's statistic data.
 */
static void 
_proxy_stat(proxy_t *ctx)
{
	int i;
	unsigned long long nsess = 0, nexist = 0;

	printf("-------proxy statistic---------\n");
	printf("naccepts:   %llu\n", ctx->stat.naccepts);
	for (i = 0; i < g_proxy.nworks; i++) {
		if (ctx->sesspools[i]) {
			nsess += ctx->sesspools[i]->total;
			nexist += ctx->sesspools[i]->size;
		}
	}

	printf("nsessions:  %llu\n", nsess);
	printf("nexist:     %llu\n", nexist);
	printf("\n");

	printf("nclirecvs:  %llu\n", ctx->stat.nclirecvs);
	printf("nclisends:  %llu\n", ctx->stat.nclisends);
	printf("nsvrrecvs:  %llu\n", ctx->stat.nsvrrecvs);
	printf("nsvrsends:  %llu\n", ctx->stat.nsvrsends);
	printf("ncliparses: %llu\n", ctx->stat.ncliparses);
	printf("nsvrparses: %llu\n", ctx->stat.nsvrparses);

	printf("nclicloses: %llu\n", ctx->stat.nclicloses);
	printf("nsvrcloses: %llu\n", ctx->stat.nsvrcloses);
	printf("nclierrors: %llu\n", ctx->stat.nclierrors);
	printf("nsvrerrors: %llu\n", ctx->stat.nsvrerrors);
	printf("-------------------------------\n");
}

/**
 *	The main entry of proxy.
 *
 *	Return 0 if success, -1 on error.
 */
int 
proxy_main(u_int32_t ip, u_int16_t port, u_int32_t rip, 
	   u_int16_t rport, int capacity, int nworks)
{
	int i;

	if (!port || !rip || !rport)
		return -1;

	g_proxy.ip = ip;
	g_proxy.port = port;
	g_proxy.rip = rip;
	g_proxy.rport = rport;
	g_proxy.capacity = capacity;
	g_proxy.nworks = nworks;

	if (_proxy_init(&g_proxy))
		return -1;

	thread_create(&g_proxy.accept, THREAD_ACCEPT, accept_run, 0);

	for (i = 0; i < nworks; i++) {
		thread_create(&g_proxy.works[i], THREAD_PARSE, 
			      work_run, i);
		thread_create(&g_proxy.recvs[i], THREAD_RECV, recv_run, i);
		thread_create(&g_proxy.sends[i], THREAD_SEND, send_run, i);
	}

	_proxy_loop(&g_proxy);

	thread_join(&g_proxy.accept);
	for (i = 0; i < nworks; i++) {
		thread_join(&g_proxy.works[i]);
		thread_join(&g_proxy.sends[i]);
		thread_join(&g_proxy.recvs[i]);
	}

	_proxy_stat(&g_proxy);

	_proxy_release(&g_proxy);

	return 0;
}










