/*
 *	@file	proxy.c	
 *
 *	@brief	the main entry file of off-line proxy.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2008-07-15
 */

#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>

#include "proxy.h"


/**
 *	Define some macro to print error message on screen.
 */
#define _PROXY_DBG	1
#ifdef _PROXY_DBG
#define _PROXY_ERR(fmt, args...)	printf("proxy:%s:%d: "fmt, \
					       __FILE__, __LINE__, ##args)
#else
#define _PROXY_ERR(fmt, args...)
#endif


/**
 *	Define g_proxy for global usage
 */
proxy_t g_proxy;

/**
 *	Signal handler for SIGINT
 *
 *	No return.
 */
static void 
_sig_int(int signo)
{
	if (signo == SIGINT) {
		g_proxy.stop = 1;
	}
}

/**
 *	Signal handler for update(SIGUSR1)
 *
 *	No return.
 */
static void 
_sig_update(int signo)
{

}

/**
 *	Signal handler for update2(SIGUSR2)
 *
 *	No return.
 */
static void 
_sig_update2(int signo)
{

}

/**
 *	Init global proxy struct. 
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_proxy_init(proxy_t *proxy)
{
	assert(proxy);

	signal(SIGINT, _sig_int);
	signal(SIGUSR1, _sig_update);
	signal(SIGUSR2, _sig_update2);

	return 0;
}

/**
 *	Free the resource in global proxy struct.
 *
 *	No return.
 */
static void
_proxy_free(proxy_t *proxy)
{
	assert(proxy);

}

/**
 *	The main thread of proxy, it just sleep until need stop.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_do_loop(proxy_t *proxy)
{
	assert(proxy);

	while (!proxy->stop) {
		sleep(1);
	}

	return 0;
}

int 
proxy_main(u_int32_t ip, u_int16_t port)
{
	g_proxy.ip = ip;
	g_proxy.port = port;
	g_proxy.stop = 0;

	if (_proxy_init(&g_proxy)) {
		return -1;
	}


	_do_loop(&g_proxy);

	_proxy_free(&g_proxy);

	return 0;
}



