/**
 *	@file	send.h
 *
 *	@brief	the send thread header file
 *	
 *	@author	Forrest.zhang
 *
 *	@date	2010-07-06
 */

#ifndef FZ_SEND_H
#define FZ_SEND_H

#define SEND_TIMEOUT	10

#include "thread.h"
#include "packet.h"

typedef struct _send {
	pktqueue_t	inq;	/* the incoming packet need send */
	pktqueue_t	outq;	/* the packet need send out */
	int		epfd;	/* the epoll fd for server connect */
	pthread_mutex_t	lock;	/* the lock for packet queue */
} send_t;


/**
 *	The send thread main function.
 *
 *	No return.
 */
extern void * 
send_run(void *arg);


/**
 *	Add the packet to send thread.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
send_add_pkt(int index, packet_t *pkt);


#endif /* end of FZ_SEND_H  */

