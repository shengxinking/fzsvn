/**
 *	@file	ssldump.c
 *
 *	@brief	a simple SSL dump program using PCAP.
 *
 *	@author	Forrest.zhang
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pcap/pcap.h>
#include <net/if.h>

#include "gcc_common.h"
#include "dbg_common.h"
#include "shash.h"

#include "netpkt.h"
#include "packet_eth.h"
#include "packet_arp.h"
#include "packet_ipv4.h"
#include "packet_ipv6.h"
#include "packet_icmpv4.h"
#include "packet_icmpv6.h"
#include "packet_udp.h"
#include "packet_tcp.h"
#include "dssl_util.h"
#include "tcp_stream.h"

static volatile int	_g_stop;		/* stop vairable */
static pcap_t		*_g_pcap;		/* pcap object */
static int		_g_verbose;		/* verbose level */
static int		_g_pcaplen = 1024;
static char		_g_intf[IFNAMSIZ] = "any";
static char		_g_filter[PATH_MAX];
static char		_g_infile[PATH_MAX];
static char		_g_outfile[PATH_MAX];
static char		_g_errbuf[PCAP_ERRBUF_SIZE];
static char		_g_keyfile[PATH_MAX];
static shash_t		*_g_tcphash;
static dssl_ctx_t	*_g_dssl_ctx;

/**
 *	Show help message
 *	
 *	No return.
 */
static void 
_usage(void)
{
	printf("ssldump <options>\n");
	printf("\t-i <interface>\tinterface name\n");
	printf("\t-f <filter>\tfilter string\n");
	printf("\t-l <N>\t\tpcap packet length\n");
	printf("\t-r <file>\tread packets from file\n");
	printf("\t-w <file>\twrite packets into file\n");
	printf("\t-k <file>\tprivate key file\n");
	printf("\t-v <N>\t\tverbose level\n");
	printf("\t-h\t\tshow help message\n");
}

/**
 *	Parse command line argument.	
 *
 * 	Return 0 if parse success, -1 on error.
 */
static char	_g_optstr[] = ":i:f:l:r:w:k:v:h";
static int 
_parse_cmd(int argc, char **argv)
{
	char opt;
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, _g_optstr)) != -1) {
		
		switch (opt) {
		
		case 'i':
			strncpy(_g_intf, optarg, IFNAMSIZ);
			break;

		case 'f':
			strncpy(_g_filter, optarg, PATH_MAX);
			break;

		case 'l':
			_g_pcaplen = atoi(optarg);
			if (_g_pcaplen < 1) {
				printf("Option %c invalid pcap length\n", 
					optopt);
				return -1;
			}
			break;

		case 'r':
			strncpy(_g_infile, optarg, PATH_MAX);
			break;

		case 'w':
			strncpy(_g_outfile, optarg, PATH_MAX);
			break;

		case 'k':
			strncpy(_g_keyfile, optarg, PATH_MAX);
			break;

		case 'v':
			_g_verbose = atoi(optarg);
			if (_g_verbose < 0 || _g_verbose > 7) {
				printf("Option %c invalid range(0-7)\n", 
					optopt);
				return -1;
			}
			break;


		case 'h':
			return -1;

		case ':':
			printf("Option %c missing argument\n", optopt);
			return -1;

		case '?':
			printf("Unknowed option %c\n", optopt);
			return -1;
		}
	}

	if (argc != optind)
		return -1;

	if (_g_infile[0] && access(_g_infile, F_OK)) {
		printf("access input file %s failed: %s\n", 
			_g_infile, ERRSTR);
		return -1;
	}

	return 0;
}

/**
 *	Stop signal handler
 *
 */
static void 
_sig_stop(int signo)
{
	printf("\nsniffex receive stop signal(%d)\n", signo);
	_g_stop = 1;
}

/**
 *	Init some global resource used in program.	
 *
 * 	Return 0 if success, -1 on error.
 */
static int 
_initiate(void)
{
	char *f;
	struct bpf_program bpf;

	/* set stop signal */
	signal(SIGINT, _sig_stop);

	/* open device or file */
	if (!_g_infile[0]) {
		f = _g_intf;
		_g_pcap = pcap_open_live(f, _g_pcaplen, 1, 10, _g_errbuf);
	}
	else {
		f = _g_infile;
		_g_pcap = pcap_open_offline(f, _g_errbuf);
	}
	if (!_g_pcap) 
		ERR_RET(-1, "pcap open (%s) failed: %s\n", f, _g_errbuf);

	/* set filter */
	if (_g_filter[0]) {
		if (pcap_compile(_g_pcap, &bpf, _g_filter, 1, 0))
			ERR_RET(-1, "compile filter (%s) failed: %s\n",
				_g_filter, _g_errbuf);

		if (pcap_setfilter(_g_pcap, &bpf)) 
			ERR_RET(-1, "set filter (%s) failed: %s\n", 
				_g_filter, _g_errbuf);

		pcap_freecode(&bpf);
	}

	/* create dssl_ctx */
	_g_dssl_ctx = dssl_ctx_new();
	if (!_g_dssl_ctx) 
		ERR_RET(-1, "dssl_ctx_new failed\n");
	
	if (dssl_ctx_load_pkey(_g_dssl_ctx, _g_keyfile, NULL))
		ERR_RET(-1, "dssl_ctx_set_pkey failed\n");

	_g_tcphash = shash_alloc(1000, tcp_tup_cmp, tcp_stream_free);
	if (!_g_tcphash)
		ERR_RET(-1, "shash_alloc failed\n");

	return 0;
}

/**
 *	Release global resource alloced by _initiate().	
 *
 * 	No Return.
 */
static int 
_release(void)
{
	if (_g_pcap)
		pcap_close(_g_pcap);

	return 0;
}

static int 
_tcp_init_tup(tcp_tup_t *t, const netpkt_t *pkt, int dir)
{
	ip_port_t *src;
	ip_port_t *dst;

	if (dir) {
		src = &t->dst;
		dst = &t->src;
	}
	else {
		src = &t->src;
		dst = &t->dst;
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

static int 
_tcp_flow(const netpkt_t *pkt)
{
	int dir;
	int ret = 0;
	u_int8_t *data;
	u_int32_t hval;
	tcp_tup_t tup;
	tcp_stream_t *t = NULL;
	struct tcphdr *h;

	h = pkt->hdr4_tcp;

	/* is server packet? */
	dir = 1;
	if (_tcp_init_tup(&tup, pkt, dir))
		return -1;
	if (tcp_tup_hash(&tup, &hval))
		return -1;
	t = shash_find(_g_tcphash, &tup, hval);

	/* not server packet */
	if (!t) {
		dir = 0;
		if (_tcp_init_tup(&tup, pkt, dir))
			return -1;
		if (tcp_tup_hash(&tup, &hval))
			return -1;
	}

	/* RST packet */
	if (h->rst) {
		t = shash_del(_g_tcphash, &tup, hval);
		if (!t) {
			return -1;
		}

		tcp_stream_free(t);
		return 0;
	}

	/* SYN packet */
	if (h->syn) {
		/* not syn ack */
		if (!h->ack) {
			t = tcp_stream_alloc();
			if (!t)
				return -1;
			t->state = TCP_ST_SYN;
			ret = shash_add(_g_tcphash, &tup, hval, 1);
			if (ret)
				return -1;
		}
		else {
			t = shash_find(_g_tcphash, &tup, hval);
			if (!t)
				return -1;

			/* current state is not SYN */
			if (t->state != TCP_ST_SYN)
				return -1;

			t->state = TCP_ST_SYN_ACK;
		}
	}

	/* FIN packet */
	if (h->fin) {
		t = shash_find(_g_tcphash, &tup, hval);
		if (!t)
			return -1;
		if (dir)
			t->state = TCP_ST_FIN2;
		else
			t->state = TCP_ST_FIN1;
	}

	/* PSH packet */
	if (h->psh) {
		data = pkt->head + pkt->hdr2_len + pkt->hdr3_len + pkt->hdr4_len;
		//ssl_decode(t->ssl, data, buf); 
	}

	/* ACK packet */
	if (h->ack) {
		t = shash_find(_g_tcphash, &tup, hval);
		if (!t)
			return -1;

		if (t->state == TCP_ST_SYN_ACK)
			t->state = TCP_ST_EST;
	}

	return 0;
}

static void 
_decode(u_char *user, const struct pcap_pkthdr *h, const u_char *bytes)
{
	netpkt_t *pkt;
	
	pkt = netpkt_alloc(h->len);
	if (!pkt) {
		_g_stop = 1;
		ERR("malloc failed: %s\n", ERRSTR);
		return;
	}

	if (netpkt_add_data(pkt, bytes, h->len))
		ERR("add data failed\n");

	if (eth_decode(pkt)) 
		exit(0);
	
	//printf(">--------eth header--------<\n");
	//eth_print(pkt, "\t");

	/* layer 3 */
	switch (pkt->hdr3_type) {
		case ETHERTYPE_IP:
			if (ipv4_decode(pkt))
				goto out_free;
			//printf(">--------ipv4 header--------<\n");
			//ipv4_print(pkt, "\t");
			break;
		case ETHERTYPE_IPV6:
			if (ipv6_decode(pkt))
				goto out_free;
			//printf(">--------ipv6 header--------<\n");
			//ipv6_print(pkt, "\t");
			break;
		default:
			goto out_free;
	}

	/* layer 4 */
	switch (pkt->hdr4_type) {
		case IPPROTO_TCP:
			if (tcp_decode(pkt))
				goto out_free;
			//printf(">--------tcp header--------<\n");
			//tcp_print(pkt, "\t");
			break;
		default:
			goto out_free;
			break;
	}

	if (_tcp_flow(pkt))
		goto out_free;


out_free:

	if (pkt)
		netpkt_free(pkt);

}

static int 
_process(void)
{
	int ret;
	
	while (!_g_stop) {
		ret = pcap_dispatch(_g_pcap, 1, _decode, NULL);
		if (ret < 0)
			break;
	}
	return 0;
}

/**
 *	The main entry of program.	
 *
 * 	Return 0 if success, other value on error.
 */
int 
main(int argc, char **argv)
{
	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_initiate())
		return -1;

	_process();

	_release();
	
	return 0;
}



