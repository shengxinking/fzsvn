/*
 *	queue.c:	queue implement using linked-list
 *
 *	author:		forrest.zhang
 */

#include "queue.h"

queue_t *queue_create(void)
{
	queue_t *queue = NULL;

	queue = malloc(sizeof(queue_t));
	if (!queue)
		return NULL;

	queue->size = 0;
	queue->head = queue->tail = NULL;

	return queue;
}

void queue_free(queue_t *queue)
{
	if (queue)
		free(queue);	
}

int queue_in(queue_t *queue, queue_node_t *node)
{
	if (!queue || !node)
		return -1;

	node->next = queue->head;
	queue->head = node;

	if (!queue->tail)
		queue->tail = node;
	
	queue->size++;

	return 0;
}

queue_node_t *queue_out(queue_t *queue)
{
	queue_node_t *node = NULL;

	if (!queue || queue->size < 1)
		return NULL;

	node = queue->tail;
}
