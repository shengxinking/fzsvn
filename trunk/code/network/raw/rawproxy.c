/*
 *	@file	
 *
 *	@brief
 *	
 *	@author	Forrest.zhang	
 *
 *	@date
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <getopt.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sched.h>
#include <assert.h>
#include <arpa/inet.h>

#include "checksum.h"


/**
 *	convert IP address(uint32_t) to dot-decimal string
 *	like xxx.xxx.xxx.xxx 
 */
#ifndef NIPQUAD
#define NIPQUAD(addr) \
        ((unsigned char *)&addr)[0], \
        ((unsigned char *)&addr)[1], \
        ((unsigned char *)&addr)[2], \
        ((unsigned char *)&addr)[3]
#define NIPQUAD_FMT "%u.%u.%u.%u"
#endif

static int _g_recvfd = -1;
static int _g_sendfd = -1;
static int _g_cpu = -1;
static unsigned long _g_bytes = (1000 * 1024 * 1024);
static u_int32_t _g_dip = 0;
static u_int32_t _g_sip = 0;

static void 
_usage(void)
{
	printf("rawproxy <options>\n");
	printf("\t-c <cpu> \tset running CPU\n");
	printf("\t-M <size>\tprocess how many MetaBytes then exit\n");
	printf("\t-d <dip> \tdestination IP\n");
	printf("\t-h       \tshow help message\n");
}

static int 
_parse_cmd(int argc, char **argv)
{
	char opt;
	char optstr[] = ":c:M:d:h";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {

		case 'c':
			_g_cpu = atoi(optarg);
			if (_g_cpu < 0 || _g_cpu > 7)
				return -1;

			break;

		case 'M':
			_g_bytes = atoi(optarg) * 1024 * 1024;
			if (_g_bytes < 1)
				return -1;

			break;
			
		case 'd':
			if (inet_aton(optarg, (struct in_addr *)&(_g_dip)) == 0) {
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

	if (_g_dip == 0)
		return -1;

	return 0;
}

static int 
_initiate(void)
{
	cpu_set_t cset;
	int sockopt;
	int socklen = sizeof(sockopt);

	_g_recvfd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
	if (_g_recvfd < 0) {
		printf("create recv socket error: %s\n", 
		       strerror(errno));
		return -1;
	}

	_g_sendfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	if (_g_sendfd < 0) {
		printf("create send socket error: %s\n", 
		       strerror(errno));
		return -1;
	}

	if (_g_cpu > 0) {
		__CPU_ZERO(&cset);
		__CPU_SET(_g_cpu, &cset);

		if (sched_setaffinity(0, sizeof(cpu_set_t), &cset)) {
			printf("set process CPU error: %s\n", 
			       strerror(errno));
			return -1;
		}
	}

	if (inet_aton("192.168.3.38", (struct in_addr *)&(_g_sip)) == 0) {
		return -1;
	}

	if (getsockopt(_g_recvfd, IPPROTO_IP, 213, &sockopt, &socklen)<0){
		printf("setsockopt(IPPROTO_IP, 213) error: %s\n",
		       strerror(errno));
   
	}

	return 0;
}


static int 
_release(void)
{
	if (_g_recvfd >= 0)
		close(_g_recvfd);

	if (_g_sendfd >= 0)
		close(_g_sendfd);

	return 0;
}

static void 
_print_iphdr(struct iphdr *iphdr)
{
	assert(iphdr);

	printf("hlen %u, ver %u, tos %u, len %u, id %u, off 0x%x, ttl %u, "
	       "prot %u, csum %u, src %u.%u.%u.%u, dst %u.%u.%u.%u\n",
	       iphdr->ihl, iphdr->version, iphdr->tos, ntohs(iphdr->tot_len),
	       ntohs(iphdr->id), ntohs(iphdr->frag_off), iphdr->ttl, iphdr->protocol,
	       ntohs(iphdr->check), NIPQUAD(iphdr->saddr), NIPQUAD(iphdr->daddr));
}


static void
_print_tcphdr(struct tcphdr *tcphdr)
{
	assert(tcphdr);

	printf("sport %u, dport %u, seq %u, ack %u, off %u, "
	       "fin %d, syn %d, rst %d, psh %d, ack %d, urg %d, "
	       "win %u, csum 0x%x, urg_ptr %u\n",
	       ntohs(tcphdr->source), ntohs(tcphdr->dest),
	       ntohl(tcphdr->seq), ntohs(tcphdr->ack_seq), tcphdr->doff,
	       tcphdr->fin, tcphdr->syn, tcphdr->rst, 
	       tcphdr->psh, tcphdr->ack, tcphdr->urg,
	       ntohs(tcphdr->window), ntohs(tcphdr->check),
	       ntohs(tcphdr->urg_ptr));
}

static int 
_do_proxy(void)
{
	unsigned long long nrecvs = 0, nsends = 0;
	int n;
	int m;
	char buf[2048];
	struct sockaddr_in svraddr;
	socklen_t addrlen;
	struct iphdr *iphdr;
	int iphdr_len;
	struct tcphdr *tcphdr;
	

	addrlen = sizeof(svraddr);
	memset(&svraddr, 0, addrlen);
	memcpy(&(svraddr.sin_addr), &(_g_dip), 4);
	svraddr.sin_port = 0;
	svraddr.sin_family = AF_INET;

	while (nrecvs < _g_bytes) {
		n = recvfrom(_g_recvfd, buf, 2047, 0, NULL, NULL);
		if (n <= 20)
			continue;

		iphdr = (struct iphdr *)buf;
		iphdr_len = iphdr->ihl * 4;
		tcphdr = (struct tcphdr *)(buf + iphdr_len);

		if (tcphdr->dest != htons(77))
			continue;

		nrecvs += n;
		
		/* set source IP and destion IP for send it out */
		memcpy(&(iphdr->saddr), &(_g_sip), 4);
		memcpy(&(iphdr->daddr), &(_g_dip), 4);

		/* clear IP checksum, calc TCP checksum */
		buf[n] = 0;
		iphdr->check = 0;
		tcphdr->check = 0;
		tcphdr->check = csum_tcpudp(iphdr->saddr, iphdr->daddr, iphdr->protocol, 
					    htons(n - iphdr_len), (u_int16_t *)tcphdr, 
					    (n - iphdr_len));

		
		m = sendto(_g_sendfd, buf, n, 0, (struct sockaddr *)&svraddr, addrlen);
		if ( m != n) {
			printf("send packet error(%d): %s\n", m, strerror(errno));
			continue;
		}

		nsends += n;

	}

	printf("total recv %lld, send %lld bytes\n",
	       nrecvs, nsends);

	return 0;
}


int 
main(int argc, char **argv)
{
	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_initiate()) {
		return -1;
	}

	_do_proxy();

	_release();

	return 0;
}








