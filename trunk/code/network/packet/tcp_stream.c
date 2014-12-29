/**
 *	@file	tcp_stream.c
 *
 *	@brief	TCP stream implement.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date
 */

#include "gcc_common.h"
#include "dbg_common.h"

#include "jhash.h"

#include "tcp_stream.h"


int 
tcp_tup_hash(tcp_tup_t *tup, u_int32_t *hval)
{
	u_int32_t a, b, c;
	u_int32_t ports;
	struct in6_addr *s;

	if (unlikely(!tup || !hval))
		return -1;

	ports = ((u_int32_t)(tup->dst.port) << 16) + (u_int32_t)(tup->src.port);

	if (tup->src.family == AF_INET) {
		*hval = jhash_3words(tup->dst._addr4.s_addr, 
				     tup->src._addr4.s_addr,
				     ports);
	}
	else if (tup->src.family == AF_INET6) {
		a = tup->dst._addr6.s6_addr32[3];
		s = &tup->src._addr6;
		b = s->s6_addr32[0] ^ s->s6_addr32[1];
		c = jhash_3words(b, s->s6_addr32[2], s->s6_addr32[3]);
		*hval = jhash_3words(a, c, ports);
	}
	else 
		return -1;

	return 0;
}

int 
tcp_tup_cmp(const void *tcp1, const void *tcp2)
{
	int ret;
	const tcp_tup_t *t1;
	const tcp_tup_t *t2;

	if (unlikely(!tcp1 || !tcp2))
		return -1;

	t1 = tcp1;
	t2 = tcp2;

	ret = ip_port_compare(&t1->src, &t2->src);
	if (ret)
		return ret;

	ret = ip_port_compare(&t1->dst, &t2->dst);

	return ret;
}

tcp_stream_t * 
tcp_stream_alloc(void)
{
	tcp_stream_t *t;

	t = malloc(sizeof(tcp_stream_t));
	if (!t) 
		ERR_RET(NULL, "malloc failed: %s\n", ERRSTR);
	memset(t, 0, sizeof(*t));

	return t;
}

void 
tcp_stream_free(void *tcp)
{
	tcp_stream_t *t;
	
	if (unlikely(!tcp))
		return;

	t = tcp;
	if (t->cli_buf)
		dbuf_free(t->cli_buf);

	if (t->svr_buf)
		dbuf_free(t->svr_buf);

	free(t);
}

void 
tcp_stream_print(void *tcp)
{
	tcp_stream_t *t;
	char ipstr1[IP_STR_LEN];
	char ipstr2[IP_STR_LEN];

	if (unlikely(!tcp))
		return;

	t = tcp;
	printf("%s->%s: state: %d\n",
		ip_port_to_str(&t->src, ipstr1, IP_STR_LEN),
		ip_port_to_str(&t->dst, ipstr2, IP_STR_LEN),
		t->state);
}




