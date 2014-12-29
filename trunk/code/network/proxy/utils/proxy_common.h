/**
 *	@file	proxy_common.h
 *
 *	@brief	proxy common macros.
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_PROXY_COMMON_H
#define FZ_PROXY_COMMON_H

#define	MAX_NAME	32
#define	MAX_PKTSIZE	(4096 - 40)
#define	MAX_PKTLEN	(MAX_PKTSIZE - sizeof(packet_t))
#define	MAX_WORKER	64
#define	MAX_CERTSET	128
#define	MAX_LISTENER	128
#define	MAX_POLICY	128
#define	MAX_SERVER	256
#define	MAX_SVRPOOL	128


#endif /* end of FZ_PROXY_COMMON_H */


