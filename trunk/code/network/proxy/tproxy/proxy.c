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

#include "packet.h"
#include "worker.h"
#include "listener.h"
#include "policy.h"
#include "svrpool.h"
#include "proxy.h"
#include "proxy_debug.h"
//#include "nb_splice.h"

/**
 *	Init proxy running data.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_py_init_data(proxy_t *py)
{
	policy_t *pl;
	struct rlimit rlim;

	if (!py)
		ERR_RET(-1, "invalid argument\n");

	g_dbglvl = py->cfg.debug;
	g_flowlvl = py->cfg.flow;
	g_httplvl = py->cfg.http;
	g_timestamp = py->cfg.timestamp;

	py->data.maxfd = py->cfg.maxconn * 2 + 100;
	DBG(1, "proxy maxfd is %d\n", py->data.maxfd);

	/* set file descriptor limit */
	if (py->data.maxfd > 1024) {
		rlim.rlim_cur = py->data.maxfd;
		rlim.rlim_max = py->data.maxfd;
		if (setrlimit(RLIMIT_NOFILE, &rlim))
			ERR_RET(-1, "setrlimit error: %s\n", ERRSTR);
		DBG(1, "proxy set rlimit(RLIMIT_NOFILE) %d\n", py->data.maxfd);
	}

	/* init policy running data */
	CBLIST_FOR_EACH(&py->pllist, pl, list) {
		if (unlikely(policy_init_data(pl)))
			ERR_RET(-1, "policy(%s) init data failed\n", pl->cfg.name);
		DBG(1, "proxy(%s) init data\n", pl->cfg.name);
	}

#if 0
	if (pycfg->nb_splice) {
		py->nb_splice_fd = nb_splice_init();
		if (py->nb_splice_fd < 0) {
			ERR("nb splice init failed\n");
			return -1;
		}
		DBG("create nb_splice fd %d\n", py->nb_splice_fd);
	}
#endif


	/* add trapt policy to kernel */
	tat_add_policies(py);

	/* add tproxy policy to kernel */
	tp_add_policies(py);

	return 0;
}

/**
 *	Free proxy running data.
 *
 */
static int 
_py_free_data(proxy_t *py)
{
	policy_t *pl;

	if (!py)
		ERR_RET(-1, "invalid data\n");
	
	/* free policy running data */
	CBLIST_FOR_EACH(&py->pllist, pl, list) {
		policy_free_data(pl);
		DBG(1, "proxy free policy_data(%s)\n", pl->cfg.name);
	}

	tat_flush_policies();

	tp_flush_policies();

	return 0;
}

/**
 *	output the proxy's statistic data.
 *
 *	No return.
 */
void 
_py_print_stat(proxy_t *py)
{
	printf("\n-------proxy statistic---------\n");
	printf("nhttplive:    %lu\n", py->stat.nhttplive);
	printf("nhttpslive:   %lu\n", py->stat.nhttpslive);
	printf("nlive:        %lu\n", py->stat.nlive);
	printf("\n-------------------------------\n");
}

/**
 *	Main loop function until @g_stop is set.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_py_loop(proxy_t *py)
{
	int i;
	policy_t *pl;
	worker_cmd_t *cmd;
	
	/* add policy to each worker */
	CBLIST_FOR_EACH(&py->pllist, pl, list) {
		assert(pl->cfg.listener);
		assert(pl->cfg.svrpool);
		/* add policy to worker */
		for (i = 0; i < py->cfg.nworker; i++) {
			cmd = calloc(1, sizeof(*cmd));
			if (!cmd)
				ERR_RET(-1, "calloc memory for worker_cmd failed\n");
			cmd->arg = policy_clone(pl);
			cmd->cmd = WORKER_CMD_ADD_POLICY;
			CBLIST_INIT(&cmd->list);
			if (worker_add_cmd(&py->data.workers[i], cmd)) {
				policy_free(pl);
				free(cmd);
				ERR_RET(-1, "add cmd to worker failed\n");
			}
		}
	}

	while (!g_stop) {
		sleep(1);
	}

	return 0;
}


proxy_t * 
proxy_alloc(void)
{
	proxy_t *py;

	py = calloc(1, sizeof(*py));
	if (!py)
		ERR_RET(NULL, "calloc memory for proxy_t failed\n");

	CBLIST_INIT(&py->certlist);
	CBLIST_INIT(&py->ltnlist);
	CBLIST_INIT(&py->splist);
	CBLIST_INIT(&py->pllist);
	
	py->cfg.nworker = 1;
	py->cfg.naccept = 1;

	return py;
}

int 
proxy_free(proxy_t *py)
{
	listener_t *ltn, *ltnbk;
	svrpool_t *sp, *spbk;
	policy_t *pl, *plbk;

	if (!py)
		ERR_RET(-1, "invalid argument\n");

	/* free listener */
	CBLIST_FOR_EACH_SAFE(&py->ltnlist, ltn, ltnbk, list) {
		CBLIST_DEL(&ltn->list);
		listener_free(ltn);
	}

	/* free svrpool */
	CBLIST_FOR_EACH_SAFE(&py->splist, sp, spbk, list) {
		CBLIST_DEL(&sp->list);
		svrpool_free(sp);
	}

	/* free policy */
	CBLIST_FOR_EACH_SAFE(&py->pllist, pl, plbk, list) {
		CBLIST_DEL(&pl->list);
		policy_free(pl);
	}

	free(py);
	return 0;
}

int 
proxy_print(const proxy_t *py)
{
	const policy_t *pl;
	const svrpool_t *sp;
	const certset_t *cert;
	const listener_t *ltn;
	const proxy_cfg_t *pycfg;

	if (!py)
		ERR_RET(-1, "invalid argument\n");

	pycfg = &py->cfg;

	printf("--------proxy(%p) begin--------\n", py);
	printf("\tworker:         %d\n", pycfg->nworker);
	printf("\tnice:           %d\n", pycfg->nice);
	printf("\tnaccept:        %d\n", pycfg->naccept);
	printf("\tuse_splice:     %d\n", pycfg->use_splice);
	printf("\tuse_nbsplice:   %d\n", pycfg->use_nbsplice);
	printf("\tmaxconn:        %d\n", pycfg->maxconn);
	printf("\tbind_cpu:       %d\n", pycfg->bind_cpu);
	printf("\tbind_cpu_algo:  %d\n", pycfg->bind_cpu_algo);
	printf("\tbind_cpu_ht:    %d\n", pycfg->bind_cpu_ht);
	printf("\tdebug:          %d\n", pycfg->debug);
	printf("\tflow:           %d\n", pycfg->flow);
	printf("\thttp:           %d\n", pycfg->http);
	printf("\ttimestamp:      %d\n", pycfg->timestamp);
	printf("\tcertset:        %d\n", py->ncert);
	printf("\tlistener number:%d\n", py->nlistener);
	printf("\tsvpool number:  %d\n", py->nsvrpool);
	printf("\tpolicy number:  %d\n", py->npolicy);
	CBLIST_FOR_EACH(&py->certlist, cert, list) {
		printf("\n");
		certset_print(cert, "");
	}

	CBLIST_FOR_EACH(&py->ltnlist, ltn, list) {
		printf("\n");
		listener_print(ltn, "");
	}
	CBLIST_FOR_EACH(&py->splist, sp, list) {
		printf("\n");
		svrpool_print(sp, "");
	}
	CBLIST_FOR_EACH(&py->pllist, pl, list) {
		printf("\n");
		policy_print(pl, "");
	}
	printf("--------proxy(%p) end  --------\n", py);

	return 0;
}

int 
proxy_add_certset(proxy_t *py, certset_t *cert)
{
	if (!py || !cert)
		ERR_RET(-1, "invalid argument\n");

	if (py->ncert > MAX_CERTSET)
		ERR_RET(-1, "too many certset in proxy\n");

	CBLIST_ADD_TAIL(&py->certlist, &cert->list);
	py->ncert++;

	return 0;
}

int 
proxy_add_listener(proxy_t *py, listener_t *ltn)
{
	if (!py || !ltn)
		ERR_RET(-1, "invalid argument\n");

	if (py->nlistener > MAX_LISTENER)
		ERR_RET(-1, "too many listener in proxy\n");

	CBLIST_ADD_TAIL(&py->ltnlist, &ltn->list);
	py->nlistener++;

	return 0;
}

int 
proxy_add_svrpool(proxy_t *py, svrpool_t *sp)
{
	if (!py || !sp)
		ERR_RET(-1, "invalid argument\n");

	if (py->nsvrpool > MAX_SVRPOOL)
		ERR_RET(-1, "too many svrpool in proxy\n");

	CBLIST_ADD_TAIL(&py->splist, &sp->list);
	py->nsvrpool++;

	return 0;
}

int 
proxy_add_policy(proxy_t *py, policy_t *pl)
{
	if (!py || !pl)
		ERR_RET(-1, "invalid argument\n");

	if (py->npolicy > MAX_POLICY)
		ERR_RET(-1, "too many svrpool in proxy\n");

	CBLIST_ADD_TAIL(&py->pllist, &pl->list);
	py->npolicy++;

	return 0;
}

certset_t *
proxy_find_certset(proxy_t *py, const char *name)
{
	certset_t *cert, *cert1;

	if (!py || !name)
		ERR_RET(NULL, "invalid name\n");

	cert1 = NULL;
	CBLIST_FOR_EACH(&py->certlist, cert, list) {
		if (strcmp(cert->name, name) == 0) {
			cert1 = cert;
			break;
		}
	}

	return cert1;
}

listener_t * 
proxy_find_listener(proxy_t *py, const char *name)
{
	listener_t *ltn, *ltn1;

	if (!py || !name)
		ERR_RET(NULL, "invalid argument\n");

	ltn1 = NULL;
	CBLIST_FOR_EACH(&py->ltnlist, ltn, list) {
		if (strcmp(ltn->cfg.name, name) == 0)
			ltn1 = ltn;
	}

	return ltn1;
}

svrpool_t * 
proxy_find_svrpool(proxy_t *py, const char *name)
{
	svrpool_t *sp, *sp1;

	if (!py || !name)
		ERR_RET(NULL, "invalid argument\n");

	sp1 = NULL;
	CBLIST_FOR_EACH(&py->splist, sp, list) {
		if (strcmp(sp->cfg.name, name) == 0)
			sp1 = sp;
	}

	return sp1;
}

policy_t * 
proxy_find_policy(proxy_t *py, const char *name)
{
	policy_t *pl, *pl1;

	if (!py || !name)
		ERR_RET(NULL, "invalid argument\n");

	pl1 = NULL;
	CBLIST_FOR_EACH(&py->pllist, pl, list) {
		if (strcmp(pl->cfg.name, name) == 0)
			pl1 = pl;
	}

	return pl1;
}

int 
proxy_main(proxy_t *py)
{
	int i;

	if (!py)
		ERR_RET(-1, "invalid argument\n");
	
	if (_py_init_data(py))
		ERR_RET(-1, "proxy init data failed\n");

	/* create work threads */
	for (i = 0; i < py->cfg.nworker; i++) {
		py->data.workers[i].cfg = py;
		thread_create(&py->data.workers[i], i, worker_run);
	}

	/* wait worker start */
	sleep(1);

	_py_loop(py);
	
	/* join work threads */
	for (i = 0; i < py->cfg.nworker; i++) {
		thread_join(&py->data.workers[i]);
	}

	_py_free_data(py);

	return 0;
}

