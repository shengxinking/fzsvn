/*
 *	@file	sniffex.c
 *
 *	@brief	a simple sniffer program using LSF(linux socket filter).
 *		see kernel document: Documentation/networking/filter.txt
 *		and BPF(Berkley Packet filter)
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

#include "netpkt.h"
#include "packet_eth.h"
#include "packet_arp.h"
#include "packet_ipv4.h"
#include "packet_ipv6.h"
#include "packet_icmpv4.h"
#include "packet_icmpv6.h"
#include "packet_udp.h"
#include "packet_tcp.h"


/* sniffex statistic data */
typedef struct sniffex_stat {
	u_int64_t	npkt;
	u_int64_t	nbyte;
} sniffex_stat_t;


static volatile int	_g_stop;		/* stop vairable */
static pcap_t		*_g_pcap;		/* pcap object */
static int		_g_verbose;		/* verbose level */
static int		_g_pcaplen = 1024;
static char		_g_intf[IFNAMSIZ] = "any";
static char		_g_filter[PATH_MAX];
static char		_g_infile[PATH_MAX];
static char		_g_outfile[PATH_MAX];
static char		_g_errbuf[PCAP_ERRBUF_SIZE];

/**
 *	Show help message	
 *	
 *	No return.
 */
static void 
_usage(void)
{
	printf("sniffex <options>\n");
	printf("\t-i <interface>\tinterface name\n");
	printf("\t-f <filter>\tfilter string\n");
	printf("\t-l <N>\t\tpcap packet length\n");
	printf("\t-r <file>\tread packets from file\n");
	printf("\t-w <file>\twrite packets into file\n");
	printf("\t-v <N>\t\tverbose level\n");
	printf("\t-h\t\tshow help message\n");
}

/**
 *	Parse command line argument.	
 *
 * 	Return 0 if parse success, -1 on error.
 */
static char	_g_optstr[] = ":i:f:l:r:w:v:h";
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

		case 'v':
			_g_verbose = atoi(optarg);
			if (_g_verbose < 0 || _g_verbose > 7) {
				printf("Option %c invalid range(0-7)\n", 
					optopt);
				return -1;
			}
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

static void 
_decode(u_char *user, const struct pcap_pkthdr *h, const u_char *bytes)
{
	netpkt_t *pkt;
	sniffex_stat_t *stat;
	
	stat = (sniffex_stat_t *)user;

	pkt = netpkt_alloc(h->len);
	if (!pkt) {
		_g_stop = 1;
		ERR("malloc failed: %s\n", ERRSTR);
		return;
	}

	printf("========================================\n");

	if (netpkt_add_data(pkt, bytes, h->len))
		ERR("add data failed\n");

#if 0
	printf(">--------netpkt--------<");
	netpkt_print(pkt, "\t");
#endif

	if (eth_decode(pkt)) 
		exit(0);
	
	printf(">--------eth header--------<\n");
	eth_print(pkt, "\t");

	/* layer 3 */
	switch (pkt->hdr3_type) {
		case ETHERTYPE_ARP:
			if (arp_decode(pkt))
				goto out_free;
			printf(">--------arp header--------<\n");
			arp_print(pkt, "\t");
			break;
		case ETHERTYPE_IP:
			if (ipv4_decode(pkt))
				goto out_free;
			printf(">--------ipv4 header--------<\n");
			ipv4_print(pkt, "\t");
			break;
		case ETHERTYPE_IPV6:
			if (ipv6_decode(pkt))
				goto out_free;
			printf(">--------ipv6 header--------<\n");
			ipv6_print(pkt, "\t");
			break;
		default:
			break;
	}

	/* layer 4 */
	switch (pkt->hdr4_type) {
		case IPPROTO_ICMP:
			if (icmpv4_decode(pkt))
				goto out_free;
			printf(">--------icmp header--------<\n");
			icmpv4_print(pkt, "\t");
			break;
	
		case IPPROTO_ICMPV6:
			if (icmpv6_decode(pkt))
				goto out_free;
			printf(">--------icmpv6 header--------<\n");
			icmpv6_print(pkt, "\t");
			break;
	

		case IPPROTO_UDP:
			if (udp_decode(pkt))
				goto out_free;
			printf(">--------udp header--------<\n");
			udp_print(pkt, "\t");
			break;

		case IPPROTO_TCP:
			if (tcp_decode(pkt))
				goto out_free;
			printf(">--------tcp header--------<\n");
			tcp_print(pkt, "\t");
			break;
		default:
			break;
	}

	stat->npkt++;
	stat->nbyte += h->len;

out_free:

	if (pkt)
		netpkt_free(pkt);

}

static int 
_process(void)
{
	int ret;
	sniffex_stat_t stat;
	
	memset(&stat, 0, sizeof(stat));
	while (!_g_stop) {
		ret = pcap_dispatch(_g_pcap, 1, _decode, (u_char *)&stat);
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



