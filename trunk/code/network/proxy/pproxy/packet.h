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

#include "cblist.h"

/**
 *	Packet struct, each packet is used to recv data
 */
typedef struct packet {
	cblist_t	list;		/* the packet in cblist */
	u_int32_t	pos;		/* position in stream */
	u_int16_t	capacity;	/* capacity of packet */
	u_int16_t	len;		/* bytes in packet */
	u_int16_t	sendpos;	/* the start send position */
	u_int16_t	recvpos;	/* teh start recv position */
	char		data[0];	/* ata in packet */
} packet_t;

static inline 
void packet_init(packet_t *pkt, u_int16_t capacity)
{
	pkt->pos = 0;
	pkt->len = 0;
	pkt->sendpos = 0;
	pkt->recvpos = 0;

	CBLIST_INIT(&pkt->list);

	pkt->capacity = capacity;
}

#endif /* end of FZ_PACKET_H  */

