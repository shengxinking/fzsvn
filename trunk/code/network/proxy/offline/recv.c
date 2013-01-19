/*
 *	@file	recv.c
 *
 *	@brief	It's the recv thread functions, it use pcap capture packet, then use snort's IP-frag
 *		TCP-asm functions to get TCP data, then put the data to work thread.
 *	
 *	@author	Forrest.zhang
 *
 *	@date	2008-07-17
 */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

#include "recv.h"
#include "proxy.h"

/**
 *	Define macro to print error message
 */
#define _RECV_DBG
#ifdef	_RECV_DBG
#define _RECV_ERR(fmt, args...)	printf("recv:%s:%d: " fmt, \
				       __FILE__, __LINE__, ##args)
#else
#define _RECV_ERR(fmt, args...)
#endif

static int 
_recv_init(thread_t *info)
{
	recv_info_t *rinfo;
	char errbuf[PCAP_ERRBUF_SIZE + 1] = {0};

	assert(info);

	rinfo = malloc(sizeof(recv_info_t));
	if (!rinfo) {
		_RECV_ERR("malloc memory for recv_info_t error: %s\n",
			  strerror(errno));
		return -1;
	}

	rinfo->pcap = pcap_open_live("any", 2048, 0, 0, errbuf);
	if (!rinfo->pcap) {
		_RECV_ERR("pcap open device any error: %s\n", errbuf);
		return -1;
	}
	
	return 0;
}


static void
_recv_free(thread_t *info)
{

}


static void 
_recv_parse(u_char *user, const struct pcap_pkthdr *h, const u_char *bytes)
{
	
}  

static int 
_recv_loop(thread_t *info)
{
	recv_info_t *rinfo;

	assert(info);
	
	rinfo = info->priv;
	assert(rinfo);

	while (!g_proxy.stop) {
		pcap_dispatch(rinfo->pcap, 1, _recv_parse, NULL);
	}

	return 0;
}

void *
recv_run(void *arg)
{
	thread_t *info;

	info = (thread_t *)arg;

	if (_recv_init(info)) {
		pthread_exit(0);
	}
	
	_recv_loop(info);

	_recv_free(info);

	pthread_exit(0);
}





