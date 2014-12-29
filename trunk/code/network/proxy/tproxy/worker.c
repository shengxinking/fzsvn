/**
 *	@file	work5.c
 *
 *	@brief	work thread which accept client, many policy in
 *		one work thread.
 *		and recv client request data, parse request data, 
 *		connect to server, send request to server and recv 
 *		reply from server, parse reply, and send reply to 
 *		client. 
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2008-07-31
 */

#define	_GNU_SOURCE

//#define NDEBUG          1

#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <assert.h>
#include <syscall.h>
#include <sys/resource.h>

#include "proxy_debug.h"
#include "thread.h"
#include "worker.h"
#include "proxy.h"
#include "packet.h"
#include "task.h"
#include "session.h"
#include "gcc_common.h"
#include "policy.h"

/**
 *	Init work thread, alloc resources.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_worker_init(thread_t *ti)
{	
	proxy_t *py;
	worker_t *wi;

	assert(ti);
	assert(ti->cfg);

	py = ti->cfg;

	/* alloc work_info */
	wi = calloc(1, sizeof(*wi));
	if (!wi) 
		ERR_RET(-1, "calloc memory for work_t failed\n");

	/* alloc object pool */
	wi->pktpool = objpool_alloc(MAX_PKTSIZE, 5000, 0);
	if (!wi->pktpool) {
		ERR("objpool_alloc for pktpool failed\n");
		goto err_free;
	}
	DBG(2, "worker[%d] alloc packet pool(%p)\n", ti->index, wi->pktpool);

	/* alloc session pool */
	wi->ssnpool = objpool_alloc(sizeof(session_t), 10000, 0);
	if (!wi->ssnpool) {
		ERR("objpool_alloc for ssnpool failed\n");
		goto err_free;
	}
	DBG(2, "worker[%d] alloc session pool(%p)\n", ti->index, wi->ssnpool);

	/* alloc task queue */
	wi->taskq = task_alloc_queue();
	if (!wi->taskq) {
		ERR("alloc task queue failed\n");
		goto err_free;
	}
	DBG(2, "worker[%d] alloc task queue(%p)\n", 
	    ti->index, wi->taskq);

	/* alloc fd_epoll */
	wi->fe = fd_epoll_alloc(py->data.maxfd, 1);
	if (!wi->fe) {
		ERR("alloc fd_epoll failed\n");
		goto err_free;
	}
	DBG(2, "worker[%d] alloc fd_epoll(%p)\n", 
	    ti->index, wi->fe);

	/* init list */
	CBLIST_INIT(&wi->lfdlist);
	CBLIST_INIT(&wi->cmdlist);

	wi->naccept = py->cfg.naccept;
	DBG(2, "worker[%d] naccept is %d\n", 
	    ti->index, wi->naccept);

	/* init lock */
	pthread_mutex_init(&wi->lock, NULL);

	ti->priv = wi;

	return 0;

err_free:
		
	if (wi->pktpool)
		objpool_free(wi->pktpool);

	if (wi->ssnpool)
		objpool_free(wi->ssnpool);

	if (wi->taskq)
		task_free_queue(wi->taskq);

	if (wi->fe)
		fd_epoll_free(wi->fe);

	free(wi);
	return -1;
}

/**
 *	Release work thread resource which alloced by _work_init()
 *
 *	No return.
 */
static void 
_worker_free(thread_t *ti)
{
	worker_t *wi;
	listener_fd_t *lfd, *bk;

	assert(ti);

	wi = ti->priv;
	if (!wi)
		return;
	
	/* free listener_fd */
	CBLIST_FOR_EACH_SAFE(&wi->lfdlist, lfd, bk, list) {
		CBLIST_DEL(&lfd->list);
		listener_free_fd(lfd);
		DBG(2, "worker[%d] free listener(%p)\n", 
		    ti->index, lfd);
	}

	if (wi->pktpool) {
		objpool_free(wi->pktpool);
		DBG(2, "worker[%d] free packet pool(%p)\n", 
		    ti->index, wi->pktpool);
	}
	
	if (wi->taskq) {
		task_free_queue(wi->taskq);
		DBG(2, "worker[%d] free taskq(%p)\n",
		    ti->index, wi->taskq);
	}

	if (wi->ssnpool) {
		objpool_free(wi->ssnpool);
		DBG(2, "worker[%d] free session pool(%p)\n", 
		    ti->index, wi->ssnpool);
	}

	if (wi->fe) {
		fd_epoll_free(wi->fe);
		DBG(2, "worker[%d] free fd_epoll(%p)\n", 
		    ti->index, wi->fe);
	}

	ti->priv = NULL;
	free(wi);
}

static int 
_worker_lock(thread_t *ti)
{
	worker_t *wi;
	
	assert(ti);
	assert(ti->priv);
	wi = ti->priv;

	if (pthread_mutex_lock(&wi->lock))
		ERR_RET(-1, "worker[%d] lock failed\n", ti->index);

	return 0;
}

static int 
_worker_unlock(thread_t *ti)
{
	worker_t *wi;

	assert(ti);
	assert(ti->priv);
	wi = ti->priv;

	if (pthread_mutex_unlock(&wi->lock))
		ERR_RET(-1, "worker%d] unlock failed\n", ti->index);

	return 0;
}

static int 
_worker_add_policy(thread_t *ti, policy_t *pl)
{
	int ret;
	worker_t *wi;
	fd_item_t *fi;
	listener_t *ltn;
	listener_fd_t *lfd;

	assert(ti);
	assert(ti->priv);
	assert(pl);
	assert(pl->cfg.listener);
	wi = ti->priv;
	ltn = pl->cfg.listener;

	DBG(2, "worker[%d] add policy %s\n", ti->index, pl->cfg.name);

	/* need locked protect listener pointer */
	lfd = listener_alloc_fd(ltn, pl->cfg.mode);
	if (!lfd) 
		ERR_RET(-1, "alloc listener fd failed\n");

	lfd->worker = wi;
	lfd->thread = ti;
	lfd->policy = pl;
	fi = fd_epoll_map(wi->fe, lfd->fd);
	assert(fi);
	memset(fi, 0, sizeof(*fi));
	fi->arg = lfd;
	fi->state = FD_READY;

	/* add fd to fd_epoll */
	ret = fd_epoll_add_event(wi->fe, lfd->fd, FD_IN, listener_accept);
	if (unlikely(ret)) {
		ERR("add fd %d to fd_epoll failed\n", lfd->fd);
		listener_free_fd(lfd);
		return -1;
	}

	CBLIST_ADD_TAIL(&wi->lfdlist, &lfd->list);	

	DBG(3, "worker[%d] add listener(%p) fd %d event read\n", 
	    ti->index, lfd, lfd->fd);

	return 0;
}

static int 
_worker_del_policy(thread_t *ti, policy_t *pl)
{
	worker_t *wi;
	listener_fd_t *lfd, *lfd1, *bk;

	assert(pl);
	assert(ti);
	assert(ti->priv);
	wi = ti->priv;

	DBG(2, "work[%d] del policy %s\n", ti->index, pl->cfg.name);

	lfd1 = NULL;
	CBLIST_FOR_EACH_SAFE(&wi->lfdlist, lfd, bk, list) {
		if (lfd->policy == pl) {
			CBLIST_DEL(&lfd->list);
			lfd1 = lfd;
			break;
		}
	}

	/* free policy */
	policy_free(pl);
	
	if (!lfd1)
		ERR_RET(-1, "worker[%d] not found policy %s\n", 
			ti->index, pl->cfg.name);

	listener_free_fd(lfd);

	return 0;
}

static int 
_worker_run_cmd(thread_t *ti, worker_cmd_t *cmd)
{
	policy_t *pl;

	pl = cmd->arg;

	switch(cmd->cmd) {

	case WORKER_CMD_ADD_POLICY:
		_worker_add_policy(ti, pl);
		break;

	case WORKER_CMD_DEL_POLICY:
		_worker_del_policy(ti, pl);
		break;

	default:
		free(cmd);
		ERR_RET(-1, "invalid cmd %d\n", cmd->cmd);
	}

	free(cmd);

	return 0;
}

static int 
_worker_get_cmd(thread_t *ti)
{
	worker_t *wi;
	cblist_t cmdlist;
	worker_cmd_t *cmd, *bak;
	
	wi = ti->priv;
	assert(wi);
	CBLIST_INIT(&cmdlist);

	/* get all commands */
	_worker_lock(ti);
	if (wi->ncmd > 0) {
		CBLIST_JOIN(&cmdlist, &wi->cmdlist);
		wi->ncmd = 0;
	}
	_worker_unlock(ti);

	/* run each commands */
	CBLIST_FOR_EACH_SAFE(&cmdlist, cmd, bak, list) {
		CBLIST_DEL(&cmd->list);	
		_worker_run_cmd(ti, cmd);		
	}
	
	return 0;
}

/**
 *	work thread main loop
 *	
 *	Always return 0.
 */
static int 
_worker_loop(thread_t *ti)
{
	worker_t *wi;
	
	assert(ti);
	assert(ti->priv);

	wi = ti->priv;

	while (!g_stop) {
		_worker_get_cmd(ti);
		fd_epoll_flush_events(wi->fe);
		fd_epoll_poll(wi->fe);
		task_run_queue(wi->taskq);
	}

	return 0;
}

void *
worker_run(void *arg)
{
	int cpu;
	proxy_t *py;
	thread_t *ti;
	pid_t tid;
	int nice;
	
	assert(arg);
	ti = arg;
	assert(ti->cfg);
	py = ti->cfg;

	DBG(1, "worker[%d] started\n", ti->index);

	/* unshare the file descriptor table avoid locked in threads */
	unshare(CLONE_FILES);
	DBG(1, "worker[%d] unshare file descriptor table\n", ti->index);

	/* set priority of this thread */
	if (py->cfg.nice != 0) {
		tid = syscall(SYS_gettid);
		if (setpriority(PRIO_PROCESS, tid, py->cfg.nice)) {
			ERR("set priority %d failed\n", py->cfg.nice);
			pthread_kill(g_maintid, SIGINT);
			pthread_exit(0);
		}
		nice = getpriority(PRIO_PROCESS, tid);
		DBG(1, "worker[%d] get priority is %d\n", ti->index, nice);
	}

	/* bind to cpu */
	if (py->cfg.bind_cpu) {
		cpu = thread_bind_cpu(pthread_self(), 
				      ti->index, 
				      py->cfg.bind_cpu_algo, 
				      py->cfg.bind_cpu_ht);
		if (cpu < 0) {
			ERR("worker[%d] bind CPU failed", ti->index);
			pthread_kill(g_maintid, SIGINT);
			pthread_exit(0);
		}

		DBG(1, "worker[%d] bind to cpu %d\n", ti->index, cpu);
	}

	if (_worker_init(ti)) {
		ERR("work[%d] init failed\n", ti->index);
		pthread_kill(g_maintid, SIGINT);
		pthread_exit(0);
	}

	_worker_loop(ti);

	_worker_free(ti);

	DBG(1, "worker[%d] stopped\n", ti->index);

	pthread_exit(0);
}

int 
worker_add_cmd(thread_t *ti, worker_cmd_t *cmd)
{
	int cmdtype;
	worker_t *wi;

	if (!ti || !cmd)
		ERR_RET(-1, "invalid argument\n");

	cmdtype = cmd->cmd;
	wi = ti->priv;
	if (!wi)
		ERR_RET(-1, "worker[%d] not running\n", ti->index);

	_worker_lock(ti);
	CBLIST_ADD_TAIL(&wi->cmdlist, &cmd->list);
	wi->ncmd++;
	_worker_unlock(ti);

	DBG(1, "proxy add cmd(%d) to worker[%d]\n", cmdtype, ti->index);

	return 0;
}
