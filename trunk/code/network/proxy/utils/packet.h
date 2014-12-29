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
	cblist_t	list;		/* packet in list */
	u_int16_t	max;		/* max bytes stored */
	u_int16_t	recvpos;	/* recv position */
	u_int32_t	sendpos;	/* send position */
	u_int16_t	len;		/* bytes in packet */
	char		data[0];	/* ata in packet */
} packet_t;

#define	PKT_INIT(p)			\
({					\
	CBLIST_INIT(&((p)->list));	\
	(p)->max = MAX_PKTLEN;		\
	(p)->sendpos = 0;		\
	(p)->recvpos = 0;		\
	(p)->len = 0;			\
})


#endif /* end of FZ_PACKET_H  */

