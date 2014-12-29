/**
 *	@file	packet.c
 *
 *	@brief	Define a set of API to manipulate packet which 
 *		is used to store data recved from client or server.
 *	
 *	@author	Forrest.zhang
 *
 *	@date	2008-07-24
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "packet.h"

/**
 *	Put a packet @p to end of packet queue @q
 *
 *	Return 0 if success, -1 on error.
 */
int 
pktq_in(pktq_t *q, packet_t *pkt)
{
	if (!q || !pkt)
		return -1;

	pkt->next = NULL;
	if (q->len)
		q->tail->next = pkt;
	else
		q->head = pkt;

	q->tail = pkt;
	q->len++;

	return 0;
}


/**
 *	Get a packet from head of packet queue @q.
 *
 *	Return pointer to packet if success, NULL on error.
 */
packet_t *
pktq_out(pktq_t *q)
{
	packet_t *pkt;

	if (!q || !q->len)
		return NULL;
	
	pkt = q->head;
	q->head = pkt->next;
	if (q->len == 1)
		q->tail = NULL;

	q->len--;
	pkt->next = NULL;
	
	return pkt;
}


/**
 *	merge packet queue @sq into packet queue @dq
 *
 *	Return 0 if success, -1 on error.
 */
int 
pktq_join(pktq_t *dq, pktq_t *sq)
{
	if (!dq || !sq)
		return -1;

	if (sq->len < 1)
		return 0;

	if (dq->len > 0)
		dq->tail->next = sq->head;
	else
		dq->head = sq->head;

	dq->tail = sq->tail;
	dq->len += sq->len;
	sq->len = 0;
	sq->head = NULL;
	sq->tail = NULL;

	return 0;
}


