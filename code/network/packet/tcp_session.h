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

typedef struct tcp_stream {
	ip_port_t	*src;
	ip_port_t	*dst;
	dbuf_t		*cli_buf;
	dbuf_t		*svr_buf;
} tcp_stream_t;


#endif /* end of FZ_TCP_STREAM_H */


