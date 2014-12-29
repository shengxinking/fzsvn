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

#include "debug.h"
#include "proxy.h"
#include "packet.h"
#include "worker.h"
#include "session.h"
//#include "nb_splice.h"

/**
 *	The global proxy variable.
 */
proxy_t		g_proxy;
int		g_dbglvl;

/**
 *	The stop signal of proxy, it interrupt accept thread, and
 *	set @g_proxy.stop as 1, all other thread will check 
 *	@g_proxy.stop to decide stop running.
 *
 *	No return.
 */
static void 
_sig_int(int signo)
{
	if (signo != SIGINT)
		return;

	printf("\n\n\n");
	DBG("worker[%d]: recv stop signal\n", g_proxy.index);
	g_proxy.stop = 1;
}


static int 
_proxy_init(proxy_t *py)
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

	/* set file descriptor limit */
	if (py->maxfd > 1024) {
		rlim.rlim_cur = py->maxfd;
		rlim.rlim_max = py->maxfd;
		if (setrlimit(RLIMIT_NOFILE, &rlim)) {
			ERR("setrlimit error: %s\n", ERRSTR);
			return -1;
		}
		DBG("set rlimit to %d\n", py->maxfd);
	}

#if 0
	/* alloc packet pool */
	py->pktpool = objpool_alloc(MAX_PKTLEN, 1000, 1);
	if (!py->pktpool) {
		ERR("objpool_alloc failed\n");
		return -1;
	}

	DBG("alloc pktpool %p\n", py->pktpool);

	/* alloc session pool */
	py->ssnpool = objpool_alloc(sizeof(session_t), 1000, 1);
	if (!py->ssnpool) {
		ERR("objpool_alloc failed\n");
		return -1;
	}

	DBG("alloc ssnpool %p\n", py->ssnpool);
#endif

#if 0
	if (py->nb_splice) {
		py->nb_splice_fd = nb_splice_init();
		if (py->nb_splice_fd < 0) {
			ERR("nb splice init failed\n");
			return -1;
		}
		DBG("create nb_splice fd %d\n", py->nb_splice_fd);
	}
#endif

	return 0;
}


static int 
_proxy_free(proxy_t *py)
{
	if (!py)
		return -1;

#if 0
	if (py->pktpool) {
		DBG("pktpool: freed %d nalloced %d\n", 
		    py->pktpool->nfreed, py->pktpool->nalloced); 
		objpool_free(py->pktpool);
		py->pktpool = NULL;
	}

	if (py->ssnpool) {
		DBG("ssnpool: freed %d nalloced %d\n", 
		    py->ssnpool->nfreed, py->ssnpool->nalloced); 
		objpool_free(py->ssnpool);
		py->ssnpool = NULL;
	}
#endif

#if 0
	if (py->nb_splice_fd > 0) {
		DBG("close nb_splice fd %d\n", py->nb_splice_fd);
		close(py->nb_splice_fd);
		py->nb_splice_fd = -1;
	}
#endif

	return 0;
}


/**
 *	output the proxy's statistic data.
 */
static void 
_proxy_stat(proxy_t *py)
{

	printf("\n-------proxy statistic---------\n");
	printf("nhttp:        %lu\n", py->stat.nhttp);
	printf("nhttps:       %lu\n", py->stat.nhttps);
	printf("naccept:      %lu\n", py->stat.naccept);
	printf("nhttplive:    %lu\n", py->stat.nhttplive);
	printf("nhttpslive:   %lu\n", py->stat.nhttpslive);
	printf("nlive:        %lu\n", py->stat.nlive);
	printf("--------------------------------------\n");
	printf("nclirecvs:    %lu\n", py->stat.nclirecv);
	printf("nclisends:    %lu\n", py->stat.nclisend);
	printf("nsvrrecvs:    %lu\n", py->stat.nsvrrecv);
	printf("nsvrsends:    %lu\n", py->stat.nsvrsend);
	printf("--------------------------------------\n");
	printf("nclicloses:   %lu\n", py->stat.ncliclose);
	printf("nsvrcloses:   %lu\n", py->stat.nsvrclose);
	printf("nclierrors:   %lu\n", py->stat.nclierror);
	printf("nsvrerrors:   %lu\n", py->stat.nsvrerror);
	printf("-------------------------------\n");
}

/**
 *	The main entry of proxy.
 *
 *	Return 0 if success, -1 on error.
 */
int 
proxy_main(proxy_arg_t *arg)
{
	proxy_t *py;
	policy_t *pl;
	char plstr[4096];
	char ipstr[IP_STR_LEN];
	int i, j;

	if (!arg)
		return -1;
	
	py = &g_proxy;
	memset(py, 0, sizeof(proxy_t));
	memcpy(py->policies, arg->policies, arg->npolicy * sizeof(policy_t));
	py->npolicy = arg->npolicy;

	py->max = arg->max;
	py->maxfd = arg->maxfd;
	py->use_splice = arg->use_splice;
	py->use_nb_splice = arg->use_nb_splice;
	py->bind_cpu = arg->bind_cpu;
	py->bind_cpu_algo = arg->bind_cpu_algo;
	py->bind_cpu_ht = arg->bind_cpu_ht;
	g_dbglvl = arg->dbglvl;

	for (i = 0; i < arg->npolicy; i++) {
		pl = &arg->policies[i];
		memset(plstr, 0, sizeof(plstr));
		snprintf(plstr, sizeof(plstr) - 1,
			 "policy<%d>: %s", pl->index,
			 ip_port_to_str(&pl->httpaddr, ipstr, IP_STR_LEN));
		for (j = 0; j < pl->nrsaddr; j++) {
			strcat(plstr, ",");
			strcat(plstr, ip_port_to_str(&pl->rsaddrs[j], 
						     ipstr, IP_STR_LEN));
		}
		DBG("%s\n", plstr);
	}

	/* init signal */
	if (_proxy_init(py)) {
		ERR("proxy init failed\n");
		_proxy_free(py);
		return -1;
	}

	worker_run(py);
	
	_proxy_stat(py);

	_proxy_free(py);

	return 0;
}






