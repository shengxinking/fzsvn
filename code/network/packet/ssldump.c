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

static void 
_tcp_free(void *tcp)
{
	return tcp_stream_free(tcp);
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
	
	if (_g_keyfile[0])
		if (dssl_ctx_load_pkey(_g_dssl_ctx, _g_keyfile, NULL))
			ERR_RET(-1, "dssl_ctx_set_pkey failed\n");

	_g_tcphash = shash_alloc(1000, tcp_tup_cmp, _tcp_free);
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
_decode_tcp(const netpkt_t *pkt)
{
	int n;
	int m;
	int dir;
	tcp_tup_t tup;
	u_int32_t hval;
	struct tcphdr *h;
	tcp_stream_t *t = NULL;

	h = pkt->hdr4_tcp;

	/* is server packet ? server packet must in hash */
	dir = 1;
	if (tcp_tup_init(&tup, pkt, dir))
		ERR_RET(-1, "tup init failed\n");
	if (tcp_tup_hash(&tup, &hval))
		ERR_RET(-1, "tup hash failed\n");
	t = shash_find(_g_tcphash, &tup, hval);
	/* client packet */
	if (!t) {
		dir = 0;
		if (tcp_tup_init(&tup, pkt, dir))
			return -1;
		if (tcp_tup_hash(&tup, &hval))
			return -1;
		t = shash_find(_g_tcphash, &tup, hval);
	}

	/* already exist TCP stream, parse TCP flow */
	if (t) {
		printf("<%d>hval %u find stream (%p)\n", dir, hval, t);
		n = tcp_flow(t, pkt, dir);
		if (n > 0) {
			printf("tcp flow return %d\n", n);
		}

		return n;
	}
	else {	
		/* new tcp stream */
		if (h->syn && !h->ack && !h->fin && !h->psh && !h->rst) {
			t = tcp_stream_alloc();
			if (!t)
				ERR_RET(-1, "tcp stream alloc failed\n");
			t->state = TCP_ST_SYN;
			t->seqacks[0].seq = ntohl(h->seq);
			t->seqacks[0].ack = ntohl(h->seq) + 1;
			if (tcp_tup_init((tcp_tup_t *)t, pkt, dir)) {
				tcp_stream_free(t);
				ERR_RET(-1, "tcp_tup_init failed\n");
			}
			shash_add(_g_tcphash, t, hval, 0);
			printf("<%d>hval %u add stream (%p)\n", dir, hval, t);
		}
	}

	return -1;
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
	}

	_decode_tcp(pkt);
	return;

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



