/*
 *   the ping program using packet APIs
 *
 *   write by Forrest.zhang
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>

#include "packet.h"

#define              ICMP_DATA            "icmp_echo message using packet library"
#define              INTERVAL             1
#define              ICMP_ID              1234

static int                   g_fd = -1;
static u_int8_t              g_buf[100] = {0};
static int                   g_times = 1;
static int                   g_send  = 0;
static int                   g_recv  = 0;
static struct sockaddr_in    g_daddr;

static void _usage(void)
{
    printf("packet_ping <dest ip> [times]\n");
}

static int _is_valid_reply(u_int8_t *buf)
{
    struct iphdr        *ip;
    struct icmphdr      *icmp;

    ip = (struct iphdr*)buf;
    icmp = (struct icmphdr*)(buf + ip->ihl * 4);

    if (icmp->un.echo.id == htons(ICMP_ID))
	return 1;
    else
	return 0;
}

static int _get_id(u_int8_t *buf)
{
    struct iphdr        *ip;
    struct icmphdr      *icmp;

    ip = (struct iphdr*)buf;
    icmp = (struct icmphdr*)(buf + ip->ihl * 4);

    return ntohs(icmp->un.echo.id);
}

static int _get_seq(u_int8_t *buf) 
{
    struct iphdr        *ip;
    struct icmphdr      *icmp;

    ip = (struct iphdr*)buf;
    icmp = (struct icmphdr*)(buf + ip->ihl * 4);

    return ntohs(icmp->un.echo.sequence);
}

static void sig_alrm(int sig)
{
    static int   seq = 0;
    int          buf_len;
    u_int8_t     recvbuf[100];
    int          recv_len = 0;

    if (g_fd < 0)
	return;

    if (g_times > 0) {
	buf_len = packet_icmp_echo((const u_int8_t *)ICMP_DATA, strlen(ICMP_DATA), 
				   g_buf, 100, ICMP_ECHO, ICMP_ID, seq);
	if (buf_len < 0) {
	    printf("create icmp echo message error: %s\n", packet_error());
	    return;
	}

	if (sendto(g_fd, g_buf, buf_len, 0, (struct sockaddr*)&g_daddr, 
		   sizeof(g_daddr)) != buf_len) {
	    printf("send icmp message error: %s\n", packet_error());
	    return;
	}
    
	g_send++;
	seq++;
    }

    g_times--;

    while (1) {
	recv_len = recvfrom(g_fd, recvbuf, 50, MSG_DONTWAIT, NULL, NULL);
	if (recv_len < 50)
	    return;

	if (_is_valid_reply(recvbuf)) {
	    printf("Recv icmp reply id is %d, sequeue is %d\n", 
		   _get_id(recvbuf), _get_seq(recvbuf));
	    g_recv++;
	}
    }
}



int main(int argc, char **argv)
{
    struct itimerval           itimer;

    /* parse command line parameter */
    if (argc != 2 && argc != 3) {
	_usage();
	return -1;
    }

    memset(&g_daddr, 0, sizeof(struct sockaddr_in));
    g_daddr.sin_family = AF_INET;
    if (inet_pton(AF_INET, argv[1], &g_daddr.sin_addr) <= 0) {
	printf("error dest address %s\n", argv[1]);
	return -1;
    }

    if (argc == 3)
	g_times = atoi(argv[2]);

    g_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (g_fd < 0) {
	printf("create RAW socket error: %s\n", strerror(errno));
	return -1;
    }
    
    signal(SIGALRM, sig_alrm);

    itimer.it_interval.tv_sec  = INTERVAL;
    itimer.it_interval.tv_usec = 0;
    itimer.it_value.tv_sec     = INTERVAL;
    itimer.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &itimer, 0);

    printf("send ICMP echo to %s ...\n", argv[1]);

    while (g_times + 1 > 0)
	sleep(INTERVAL);
    
    printf("send %d packet, recved %d packet, lost %.2f packet\n", 
	   g_send, g_recv, ((float)(g_send - g_recv)) / g_send);

    return 0;
}
