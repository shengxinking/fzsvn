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
#include "netpkt.h"

#define	TCP_PKTQ_MAX	128
#define	TCP_PKTQ_INC	16

/**
 *	TCP state.
 */
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
 *	TCP stream tup.
 */
typedef struct tcp_tup {
	ip_port_t	src;
	ip_port_t	dst;
} tcp_tup_t;

/**
 *	TCP sequence and ack.
 */
typedef struct tcp_seqack {
	u_int32_t	seq;
	u_int32_t	ack;
} tcp_seqack_t;

/**
 *	TCP packet include netpkt_t @pkt.
 */
typedef struct tcp_pkt {
	u_int32_t	seq;
	u_int16_t	len;
	u_int16_t	off;
	netpkt_t	*pkt;	
} tcp_pkt_t;

/**
 *	TCP reassemble struct and data in it.
 */
typedef struct tcp_pktq {
	tcp_pkt_t	*pkts;
	u_int16_t	npkt;
	u_int16_t	max;		/* max pkts can store in @pkts */
	u_int32_t	len;		/* length of continuous data */
	u_int32_t	nassembly;	/* assembly packet number */
	u_int32_t	first_seq;	/* first seq number for reassemble */
} tcp_pktq_t;

/**
 *	TCP stream structure.
 */
typedef struct tcp_stream {
	ip_port_t	src;		/* src address */
	ip_port_t	dst;		/* dst address */
	tcp_seqack_t	seqacks[2];	/* syn ack number */
	u_int32_t	winfcts[2];	/* window factor for TCP window option */
	tcp_pktq_t	pktqs[2];	/* PSH packets queue for reassemble */
	int		state;		/* state: see above */
	void		*ssl;		/* ssl data */
} tcp_stream_t;

extern int 
tcp_tup_init(tcp_tup_t *tup, const netpkt_t *pkt, int dir);

extern int 
tcp_tup_hash(tcp_tup_t *tup, u_int32_t *hval);

extern int 
tcp_tup_cmp(const void *tcp1, const void *tcp2);

extern const char * 
tcp_tup_to_str(const tcp_tup_t *tup, char *buf, size_t len);

extern tcp_stream_t * 
tcp_stream_alloc(void); 

extern void 
tcp_stream_free(tcp_stream_t *t);

extern int 
tcp_stream_flow(tcp_stream_t *t, const netpkt_t *pkt, int dir);

extern int 
tcp_stream_data_len(const tcp_stream_t *t, int dir);

extern int
tcp_stream_get_data(tcp_stream_t *t, u_int8_t *buf, size_t len, int dir);

extern void 
tcp_stream_print(void *tcp);


#endif /* end of FZ_TCP_STREAM_H */


