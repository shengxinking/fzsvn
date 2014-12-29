/**
 *	@file	task.h
 *
 *	@brief	The proxy task APIs
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_TASK_H
#define FZ_TASK_H

#include <unistd.h>
#include <assert.h>

#include "cblist.h"
#include "gcc_common.h"

struct task;

typedef	int	(*task_cb)(struct task *t);

/* task type */
enum {
	TASK_NONE,
	TASK_PARSE,
	TASK_SEND,
	TASK_DELETE,
};

typedef struct task {
	cblist_t	list;		/* task in list */
	int		task;		/* task type */
	void		*arg1;		/* session of task running */
	void		*arg2;		/* session_data of task running */
	task_cb		cb;		/* callback function */
	struct task	*peer;		/* the other side task */
} task_t;

typedef struct task_queue {
	cblist_t	list;		/* task list, all tasks link into it */
	int		ntask;		/* number of task in list */
} task_queue_t;

extern task_queue_t * 
task_alloc_queue(void);

extern void 
task_free_queue(task_queue_t *tq);

static inline
int task_in_queue(task_t *t)
{
	return !(CBLIST_IS_EMPTY(&t->list));
}

static inline 
void task_init(task_t *t)
{
	CBLIST_INIT(&t->list);
	t->task = TASK_NONE;
}

static inline 
void task_add_queue(task_queue_t *tq, task_t *t)
{
	CBLIST_ADD_TAIL(&tq->list, &t->list);
	tq->ntask++;
}

static inline 
void task_del_queue(task_queue_t *tq, task_t *t)
{
	CBLIST_DEL(&t->list);
	tq->ntask--;
}

static inline 
void task_run_queue(task_queue_t *tq)
{
	task_t *t, *b;

	CBLIST_FOR_EACH_SAFE(&tq->list, t, b, list) {

		if (unlikely(!t->arg1))
			continue;

		if (unlikely(!t->arg2))
			continue;

		if (unlikely(!t->cb))
			continue;
		
		task_del_queue(tq, t);

		t->cb(t);
	}
	//assert(tq->ntask == 0);
}

#endif /* end of FZ_TASK_H  */


