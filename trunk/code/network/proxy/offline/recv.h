/*
 *	@file
 *
 *	@brief
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_RECV_H
#define FZ_RECV_H

#include <pcap.h>

typedef struct recv_info {
	pcap_t		*pcap;
	u_int32_t	ip;		/* the proxy IP */
	u_int16_t	port;		/* the proxy port */
	int		flags;
} recv_info_t;


/**
 *	The recv thread main function.
 */
extern void 
*recv_run(void *arg);


#endif /* end of FZ_RECV_H  */

