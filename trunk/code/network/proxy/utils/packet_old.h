/*
 *	@file	packet.h
 *
 *	@brief	The packet APIs
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_PACKET_H
#define FZ_PACKET_H

#include <sys/types.h>

/**
 *	Packet struct, each packet is used to recv data
 */
typedef struct packet {
	struct packet	*next, *prev;	/* double link in queue */
	u_int32_t	sid;		/* the session id of packet */
	u_int32_t	pos;		/* position in stream */
	u_int16_t	capacity;	/* capacity of packet */
	u_int16_t	len;		/* bytes in packet */
	u_int16_t	sendpos;	/* the start send position */
	u_int16_t	recvpos;	/* teh start recv position */

	u_int16_t	is_request:1;	/* it's the request message */
	
	char		data[0];	/* ata in packet */
} packet_t;

/**
 * 	The queue of packet.
 */
typedef struct pktq {
	u_int32_t	len;
	packet_t	*head;
	packet_t	*tail;
} pktq_t;


/**
 *	Put packet @p into tail of packet queue @q.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
pktq_in(pktq_t *q, packet_t *p);


/**
 *	Get a packet from head of packet queue @q.
 *
 *	Return a pointer to packet if success, NULL mean @q if empty or error
 */
extern packet_t *
pktq_out(pktq_t *q);


/**
 *	merge packet queue @sq into packet queue @dq
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
pktq_join(pktq_t *dq, pktq_t *sq);



#endif /* end of FZ_PACKET_H  */

