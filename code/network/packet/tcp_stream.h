/**
 *	@file	tcp_stream.h
 *
 *	@brief	TCP stream structure and APIs.
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_TCP_STREAM_H
#define FZ_TCP_STREAM_H

#include "dbuffer.h"
#include "ip_addr.h"

enum {
	TCP_ST_SYN,
	TCP_ST_SYN_ACK,
	TCP_ST_EST,
	TCP_ST_FIN1,
	TCP_ST_FIN1_ACK,
	TCP_ST_FIN2,
	TCP_ST_CLOSE,
};

/**
 *
 */
typedef struct tcp_tup {
	ip_port_t	src;
	ip_port_t	dst;
} tcp_tup_t;

/**
 *	TCP stream structure.
 */
typedef struct tcp_stream {
	ip_port_t	src;
	ip_port_t	dst;
	int		state;
	dbuf_t		*cli_buf;
	dbuf_t		*svr_buf;
} tcp_stream_t;

extern int 
tcp_tup_hash(tcp_tup_t *tup, u_int32_t *hval);

extern int 
tcp_tup_cmp(const void *tcp1, const void *tcp2);

extern tcp_stream_t * 
tcp_stream_alloc(void); 

extern void 
tcp_stream_free(void *tcp);

extern void 
tcp_stream_print(void *tcp);

#endif /* end of FZ_TCP_STREAM_H */


