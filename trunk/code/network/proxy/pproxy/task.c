/**
 *	@file	task.c
 *
 *	@brief	the task function implement.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "task.h"

task_queue_t * 
task_alloc_queue(void)
{
	task_queue_t *tq;

	tq = calloc(1, sizeof(task_queue_t));
	if (!tq)
		return NULL;

	CBLIST_INIT(&tq->list);

	return tq;
}

void 
task_free_queue(task_queue_t *tq)
{
	task_t *t, *b;

	if (!tq)
		return;

	CBLIST_FOR_EACH_SAFE(&tq->list, t, b, list) {
		CBLIST_DEL(&t->list);
	}

	free(tq);
}

