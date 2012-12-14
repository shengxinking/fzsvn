/*
 *	queue.h:	queue implement using link-list
 *
 *	author:		forrest.zhang
 */

#ifndef _FZ_QUEUE_H
#define _FZ_QUEUE_H

typedef struct queue_node {
	void *			data;
	struct queue_node	*next;
} queue_node_t

typedef struct queue {
	int		size;
	queue_node_t	*head;
	queue_node_t	*tail;
} queue_t;


extern queue_t *queue_create(int size);
extern int queue_in(queue_t *queue, queue_node_t *node);
extern int queue_out(queue_t *queue, queue_node_t *node);


#endif /* end of _FZ_QUEUE_H */

