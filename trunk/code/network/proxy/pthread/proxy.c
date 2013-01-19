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
#include "packet.h"
#include "nb_splice.h"

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
	if (signo != SIGINT)
		return;

	if (pthread_self() == g_proxy.main_tid) {
		printf("\n\n\nrecved stop signal SIGINT\n");
		g_proxy.stop = 1;
	}
	else {
		if (g_proxy.stop == 0)
			pthread_kill(g_proxy.main_tid, SIGINT);
	}
}


static int 
_proxy_init(proxy_t *ctx)
{
	struct sigaction act;
	struct rlimit rlim;

	/* using SIGINT as stop signal */ 
	act.sa_handler = _sig_int;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_restorer = NULL;
	if (sigaction(SIGINT, &act, NULL)) {
		ERR("sigaction error: %s\n", ERRSTR);
		return -1;
	}

	/* ignore SIGPIPE signal */
	act.sa_handler = SIG_IGN;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_restorer = NULL;
	if (sigaction(SIGPIPE, &act, NULL)) {
		ERR("sigaction error: %s\n", ERRSTR);
		return -1;
	}

	rlim.rlim_cur = (1024*1024*100);
	rlim.rlim_max = (1024*1024*100);
	if (setrlimit(RLIMIT_CORE, &rlim)) {
		ERR("setrlimit error %s\n", ERRSTR);
		return -1;
	}

	/* set file descriptor limit */
	if (ctx->max > 256) {
		rlim.rlim_cur = ctx->max * 4 + 10;
		rlim.rlim_max = ctx->max * 4 + 10;
		if (setrlimit(RLIMIT_NOFILE, &rlim)) {
			ERR("setrlimit error: %s\n", ERRSTR);
			return -1;
		}
		DBG("set rlimit to %d\n", ctx->max * 4 + 10);
	}

	/* alloc packet pool */
	ctx->pktpool = objpool_alloc(MAX_PKTLEN, 1000, 1);
	if (!ctx->pktpool) {
		ERR("objpool_alloc failed\n");
		return -1;
	}

	DBG("alloc pktpool %p\n", ctx->pktpool);

	/* alloc session pool */
	ctx->ssnpool = objpool_alloc(sizeof(session_t), 1000, 1);
	if (!ctx->ssnpool) {
		ERR("objpool_alloc failed\n");
		return -1;
	}

	DBG("alloc ssnpool %p\n", ctx->ssnpool);

	if (ctx->use_nb_splice) {
		ctx->nb_splice_fd = nb_splice_init();
		if (ctx->nb_splice_fd < 0) {
			ERR("nb splice init failed\n");
			return -1;
		}
		DBG("create nb_splice fd %d\n", ctx->nb_splice_fd);
	}

	printf("\n\n");

	ctx->main_tid = pthread_self();

	return 0;
}


static int 
_proxy_free(proxy_t *ctx)
{
	if (!ctx)
		return -1;

	if (ctx->pktpool) {
		DBG("pktpool: freed %d nalloced %d\n", 
		    ctx->pktpool->nfreed, ctx->pktpool->nalloced); 
		objpool_free(ctx->pktpool);
		ctx->pktpool = NULL;
	}

	if (ctx->ssnpool) {
		DBG("ssnpool: freed %d nalloced %d\n", 
		    ctx->ssnpool->nfreed, ctx->ssnpool->nalloced); 
		objpool_free(ctx->ssnpool);
		ctx->ssnpool = NULL;
	}

	if (ctx->nb_splice_fd > 0) {
		DBG("close nb_splice fd %d\n", ctx->nb_splice_fd);
		close(ctx->nb_splice_fd);
		ctx->nb_splice_fd = -1;
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

	printf("\n-------proxy statistic---------\n");
	printf("naccepts:   %lu\n", ctx->stat.naccept);
	printf("nclirecvs:  %lu\n", ctx->stat.nclirecv);
	printf("nclisends:  %lu\n", ctx->stat.nclisend);
	printf("nsvrrecvs:  %lu\n", ctx->stat.nsvrrecv);
	printf("nsvrsends:  %lu\n", ctx->stat.nsvrsend);
	printf("nclicloses: %lu\n", ctx->stat.ncliclose);
	printf("nsvrcloses: %lu\n", ctx->stat.nsvrclose);
	printf("nclierrors: %lu\n", ctx->stat.nclierror);
	printf("nsvrerrors: %lu\n", ctx->stat.nsvrerror);
	printf("-------------------------------\n");
}

/**
 *	The main entry of proxy.
 *
 *	Return 0 if success, -1 on error.
 */
int 
proxy_main(ip_port_t *svraddr, ip_port_t *rsaddrs, int nrsaddr, 
	   int max, int nwork, int use_splice, int use_nb_splice)
{
	int i;
	char buf[512];

	if (!svraddr || !rsaddrs || nrsaddr < 1 ||
	    max < 1 || nwork < 1)
		return -1;

	g_proxy.httpaddr = *svraddr;
	memcpy(g_proxy.rsaddrs, rsaddrs, sizeof(ip_port_t) * nrsaddr);
	g_proxy.nrsaddr = nrsaddr;
	g_proxy.max = max;
	g_proxy.nwork = nwork;
	g_proxy.dbglvl = 1;
	g_proxy.use_splice = use_splice;
	g_proxy.use_nb_splice = use_nb_splice;

	printf("\nproxyd: listen %s, work %d, max %d, real servers %d\n", 
	       ip_port_to_str(svraddr, buf, sizeof(buf)), 
	       g_proxy.nwork, g_proxy.max, g_proxy.nrsaddr);
	for (i = 0; i < nrsaddr; i++) {
		printf("\t[real server %d]: %s\n", i, 
		       ip_port_to_str(&rsaddrs[i], buf, sizeof(buf)));
	}
	printf("\n");

	if (_proxy_init(&g_proxy)) {
		_proxy_free(&g_proxy);
		return -1;
	}

	for (i = 0; i < nwork; i++) {
		thread_create(&g_proxy.works[i], THREAD_WORK, 
			      work_run, i);
		usleep(50);
	}
	/* create threads */
	thread_create(&g_proxy.accept, THREAD_ACCEPT, accept_run, 0);

	_proxy_loop(&g_proxy);

	thread_join(&g_proxy.accept);
	for (i = 0; i < nwork; i++) {
		thread_join(&g_proxy.works[i]);
	}

	_proxy_stat(&g_proxy);

	_proxy_free(&g_proxy);

	return 0;
}

void 
proxy_stat_lock(void)
{
	pthread_mutex_lock(&g_proxy.lock);
}

void 
proxy_stat_unlock(void)
{
	pthread_mutex_unlock(&g_proxy.lock);
}







