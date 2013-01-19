/*
 *	@file
 *
 *	@brief
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_PROXY_H
#define FZ_PROXY_H

#include <sys/types.h>
#include <pcap.h>

/**
 *	The thread struct of proxy.
 */
typedef struct thread {
	pthread_t	tid;
	int		type;
	void		*priv;
} thread_t;



/**
 *	The global struct of proxy.
 */
typedef struct proxy {
	u_int32_t	ip;
	u_int32_t	port;

	int		stop;

	/* the thread info */
#define	WORK_MAX	20
	thread_t	works[WORK_MAX];
	thread_t	recv;
} proxy_t;


/**
 *	g_proxy is a global variable, so take it easy when change it.
 */
extern proxy_t g_proxy;


/**
 *	The main entry of off-line proxy
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
proxy_main(u_int32_t ip, u_int16_t port);


#endif /* end of FZ_PROXY_H  */


