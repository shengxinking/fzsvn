/*
 *  this program try to capture all HTTP packet using pcap and socket mmap
 *  It's used to test socket mmap efficiency 
 *
 *  write by Forrest.zhang
 */

#define _GNU_SOURCE

#include <unistd.h>
#include <pcap.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>

#define PROG_NAME         "sniffer"
#define PROG_OPTIONS      ":e:n:d:s:p:h"
#define DEV_NAMEMAX       24
#define PROTO_TCP         0x1
#define PROTO_UDP         0x2
#define PKT_LEN           2048
#define VALVE             1024*1024*100
#define LOOP_SIZE         50000

static char      device[DEV_NAMEMAX] = {0};
static long long npackets = 100;
static int       sport = 0;
static int       dport = 0;
static int       protocol = 0;                    /* default is TCP */
static char      errbuf[PCAP_ERRBUF_SIZE] = {0};  /* pcap error message */
static long long throughout = 0;
static time_t    last = 0;

static long my_atol(const char* str)
{
    long      ret = 0;

    if (str)
	sscanf(str, "%ld", &ret);

    return ret;
}

static long long my_atoll(const char* str)
{
    long long ret = 0;

    if (str)
	sscanf(str, "%lld", &ret);

    return ret;
}

static short my_atos(const char *str)
{
    int     ret = 0;
    
    if (str) 
	sscanf(str, "%d", &ret);

    return (short)ret;
}

/*
 *  show usage
 */
static void usage(void)
{
    printf("usage: %s <options>\n", 
	   PROG_NAME);
    printf("options is following:\n");
    printf("\t-e <device name>     \tdevice name\n");
    printf("\t-n <packet number>   \tstoped when capture n packets\n");
    printf("\t-d <destination port>\tdestination port, only in TCP/UDP protocol\n");
    printf("\t-s <source port>     \tsource port, only in TCP/UDP protocol\n");
    printf("\t-p <protocol TCP|UDP>\tprotocol, use TCP or UDP, default is all\n");
    printf("\t-h                   \tshow help message\n");
    printf("\nreport bugs to Forrest.zhang(guozhang@fortinet.com)\n");
}

/*
 *  parse command line parameter using getopt
 */
extern char *optarg;
extern int  optind, opterr, optopt;
static int do_command_parse(int argc, char **argv, char *options)
{
    char        c;
    
    opterr = 0;      /* close error print by getopt */

    while ( (c = getopt(argc, argv, options)) != -1) {
	switch (c) {

	case 'e' :
	    strncpy(device, optarg, DEV_NAMEMAX - 1);
	    break;
	case 'd' :
	    dport = my_atos(optarg);
	    break;
	case 's' :
	    sport = my_atos(optarg);
	    break;
	case 'n' :
	    npackets = my_atoll(optarg);
	    break;
	case 'p' :
	    if (strcasecmp(optarg, "TCP") == 0)
		protocol = PROTO_TCP;
	    else if (strcasecmp(optarg, "UDP") == 0)
		protocol = PROTO_UDP;
	    else {
		printf("protocol must be TCP or UDP\n");
		return -1;
	    }
	    break;
	case 'h' :
	    usage();
	    return -1;
      	case ':' :
	    printf("option %c missing parameter, see help\n", optopt);
	    return -1;
	case '?' :
	    printf("unkown options %c\n", optopt);
	    return -1;
	default:
	    printf("unknowned option %c\n", c);
	    return -1;
	}
    }

    if (argc != optind) {
	printf("too many args\n");
	return -1;
    }

    return 0;
}

/*
 *  callback function to filter packet
 */
static void pkt_handler(u_char *user, const struct pcap_pkthdr *h, const u_char *p)
{
    static long long  recv = 0;
    time_t            now  = time(NULL);

    throughout += h->len;
    recv += h->len;

    if (recv > VALVE) {
	printf("received %lld bytes in %ld second\n", recv, now - last);
	last = now;
	recv = 0;
    }
}

int main(int argc, char **argv)
{
    struct bpf_program    filter;
    pcap_t                *ppcap = NULL;
    int                   cnt;
    int                   left;
    
    if (do_command_parse(argc, argv, PROG_OPTIONS))
	return -1;
    
    if (device[0] == '\0') {
	usage();
	return -1;
    }

    ppcap = pcap_open_live(device, PKT_LEN, 0, 1000, errbuf);
    if (!ppcap) {
	printf("%s\n", errbuf);
	return -1;
    }

    pcap_compile(ppcap, &filter, NULL, 1, 0);
    pcap_setfilter(ppcap, &filter);
    
    last = time(NULL);
    
    /* recv packet */
    cnt = npackets / LOOP_SIZE;
    left = npackets % LOOP_SIZE;

    while (cnt > 0) {
	pcap_loop(ppcap, LOOP_SIZE, pkt_handler, NULL);
	--cnt;
    }

    if (left)
	pcap_loop(ppcap, left, pkt_handler, NULL);

    printf("receive %lld packet, %lld bytes, %.2f KiloBytes, %.2f MetaBytes\n",
	   npackets, throughout, throughout/1024.0, (throughout/1024.0)/1024);

    pcap_close(ppcap);
        
    return 1;
}
