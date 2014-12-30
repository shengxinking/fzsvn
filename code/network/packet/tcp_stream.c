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

static int 
_tcp_check_seq(tcp_stream_t *t, struct tcphdr *h, int dir)
{
	tcp_seqack_t *sa;

	sa = &t->seqacks[dir%2];

	if (sa->seq == 0)
		return 0;

	/* sequence out of range */
	if (ntohl(h->seq) <= sa->seq)
		return -1;

	return 0;
}

static int 
_tcp_pktq_pos(tcp_pktq_t *q, u_int32_t seq)
{
	int l, h, m;

	if (q->npkt = 0)
		return 0;

	l = 0;
	h = q->npkt;
	m = h / 2;

}

static int 
_tcp_pktq_add(tcp_pktq_t *q, const netpkt_t *pkt)
{
	int n;
	int m;
	int size;
	int max;
	tcp_pkt_t *tp;
	//int high, low, mid;

	n = pkt->tail - pkt->head;
	m = pkt->hdr2_len + pkt->hdr3_len + pkt->hdr4_len;

	size = (n - m);

	if (size < 0)
		return -1;
	else if (size == 0)
		return 0;
	
	if (tf->npkt >= TCP_PKTQ_MAX)
		ERR_RET(-1, "too many pkt in frag\n");

	if (tf->npkt == q->max) {
		if (max == 0)
			max = 10;
		if (tcp_pktq_init(q, max))
			return -1;
	}

	

	return size;
}

static int 
_tcp_pktq_get_data(tcp_pktq_t *pktq, u_int8_t *buf, size_t len)
{
	return 0;
}



int 
tcp_tup_init(tcp_tup_t *tup, const netpkt_t *pkt, int dir)
{
	ip_port_t *src;
	ip_port_t *dst;

	if (unlikely(!tup || !pkt))
		ERR_RET(-1, "invalid argument\n");

	if (unlikely(!pkt->hdr4_tcp))
		ERR_RET(-1, "need decode TCP header\n");

	if (dir) {
		src = &tup->dst;
		dst = &tup->src;
	}
	else {
		src = &tup->src;
		dst = &tup->dst;
	}

	if (pkt->hdr3_type == ETHERTYPE_IP) {
		src->family = AF_INET;
		dst->family = AF_INET;
		src->_addr4.s_addr = pkt->hdr3_ipv4->saddr;
		dst->_addr4.s_addr = pkt->hdr3_ipv4->daddr;
	}
	else if (pkt->hdr3_type == ETHERTYPE_IPV6) {
		src->family = AF_INET6;
		dst->family = AF_INET6;
		src->_addr6 = pkt->hdr3_ipv6->ip6_src;
		dst->_addr6 = pkt->hdr3_ipv6->ip6_dst;
	}
	else 
		return -1;

	if (pkt->hdr4_type == IPPROTO_TCP) {
		src->port = pkt->hdr4_tcp->source;
		dst->port = pkt->hdr4_tcp->dest;
	}
	else
		return -1;

	return 0;
}


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

const char * 
tcp_tup_to_str(const tcp_tup_t *tup, char *buf, size_t len)
{
	char ipstr1[IP_STR_LEN];
	char ipstr2[IP_STR_LEN];

	if (!tup || !buf || len < IP_STR_LEN)
		return NULL;

	snprintf(buf, len, "%s->%s", 
		 ip_port_to_str(&tup->src, ipstr1, IP_STR_LEN),
		 ip_port_to_str(&tup->dst, ipstr2, IP_STR_LEN));

	return buf;
}

int 
tcp_pktq_init(tcp_pktq_t *q, int max)
{
	tcp_pktq_t *p;

	if (unlikely(!q || max < q->max))
		ERR_RET(-1, "invalid argument\n");

	if (max == q->max)
		return 0;

	if (max > TCP_PKTQ_MAX)
		return -1;
	
	p = realloc(q->pkts, max * sizeof(tcp_pkt_t));
	if (!p)
		return -1;

	if (q->pkts && q->pkts != p)
		free(q->pkts);

	q->pkts = p;

	return 0;
}

int 
tcp_pktq_free(tcp_pktq_t *q)
{
	int i;
	netpkt_t *pkt;

	if (!q)
		return;

	for (i = 0; i < q->npkt; i++) {
		pkt = q->pkts[i].pkt;
		if (pkt)
			free(pkt);
	}

	if (q->pkts)
		free(q->pkts);
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
tcp_stream_free(tcp_stream_t *t)
{
	if (unlikely(!t))
		return;

	tcp_pktq_free(t->pktqs[0]);
	tcp_pktq_free(t->pktqs[1]);

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

int 
tcp_flow(tcp_stream_t *t, const netpkt_t *pkt, int dir)
{
	int ret;
	struct tcphdr *h;

	if (unlikely(!t || !pkt))
		ERR_RET(-1, "invalid argument\n");

	if (unlikely(!pkt->hdr4_tcp))
		ERR_RET(-1, "not decode TCP header\n");

	h = pkt->hdr4_tcp;

	/* check seq/ack number */
	if (_tcp_check_seq(t, h, dir))
		ERR_RET(-1, "invalid seq number\n");

	/* RST packet */
	if (h->rst) {
		printf("<%d> RST: seq %u, ack %u\n", 
			dir, ntohl(h->seq), ntohl(h->ack));
		return -1;
	}

	printf("<%d>stream(%p) before state: %d\n", dir, t, t->state);

	/* TCP state */
	switch (t->state) {

		case TCP_ST_SYN:
			/* server syn-ack packet */
			if (dir == 1 && h->syn && h->ack)
				t->state = TCP_ST_SYN_ACK;
			break;
		case TCP_ST_SYN_ACK:
			/* client ACK, establish connection */
			if (dir == 0 && h->ack)
				t->state = TCP_ST_EST;
			break;
		case TCP_ST_EST:
			/* receive Fin */
			if (h->fin)
				t->state = TCP_ST_FIN1;
			if (h->psh) {
				ret = _tcp_pktq_add(t, pkt, dir);
			}
			break;
		case TCP_ST_FIN1:
			if (h->ack)
				t->state = TCP_ST_FIN1_ACK;
			if (h->fin)
				t->state = TCP_ST_FIN2;
			if (h->psh) {
				ret = _tcp_pktq_add(t, pkt, dir);
			}
			break;
		case TCP_ST_FIN1_ACK:
			if (h->fin)
				t->state = TCP_ST_FIN2;
			break;
		case TCP_ST_FIN2:
			if (h->ack) {
				t->state = TCP_ST_CLOSE;
				return 0;
			}
			break;
		default:
			break;
	}

	printf("<%d>stream(%p) after state: %d\n", dir, t, t->state);

	return ret;
}

int 
tcp_data_len(const tcp_stream_t *t, int dir)
{
	if (unlikely(!t))
		ERR_RET(-1, "invalid argument\n");

	return t->frags[dir % 2].len;
}

int 
tcp_get_data(tcp_stream_t *t, char *buf, size_t len, int dir)
{
	int n;
	dbuf_t *buf;

	if (unlikely(!t || !buf || len < 1))
		return NULL;

	n = _tcp_frag_get_data(t->frags[dir % 2], buf, len);
	return n;
}



