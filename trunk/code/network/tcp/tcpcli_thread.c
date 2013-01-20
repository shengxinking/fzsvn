/*
 *	tcpcli_thread.c:	the thread implement of TCP client, TCP 
 *				client generator some threads, each thread 
 *				connect TCP server, send some
 *				bytes, and close it, loop it to end.
 *
 * 	author:			forrest.zhang
 */

#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#define IPLEN	32
#define PKTLEN	1024

struct thread_info {
	pthread_t	tid;
	int		nconnects;
	int		begin;
	int		end;
	long long	nbytes;
};

static struct thread_info *g_thread_infos;
static struct sockaddr_in g_svr_addr;
static char g_svr_ip[IPLEN] = {0};
static short g_svr_port = 8080;
static int g_thread_num = 1;
static int g_pkt_size = 64;
static char g_data[PKTLEN] = {0};
static int g_stop = 0;

static void _usage(void)
{
	printf("tcpcli_thread [options]\n");
	printf("tcpcli: make n threads, each thread connect to"
	       " TCP server (a:p), send s bytes data, closed and reagain\n");
	printf("-a\tserver address\n");
	printf("-p\tserver port\n");
	printf("-s\tdata size\n");
	printf("-n\tthread number\n");
	printf("-h\tshow help infomation\n");
}

static int _parse_cmd(int argc, char **argv)
{
	char c;
	int port;
	int num;
	int size;
	char optstr[] = ":a:p:s:n:";

	opterr = 0;
	while ( (c = getopt(argc, argv, optstr)) != -1) {
		switch (c) {

		case 'a':
			strncpy(g_svr_ip, optarg, IPLEN - 1);
			break;

		case 'p':
			port = atoi(optarg);
			if (port <= 0 || port > 65535) {
				printf("-p <1 - 65535>\n");
				return -1;
			}
			g_svr_port = port;
			break;

		case 's':
			size = atoi(optarg);
			if (size <= 0 || size > PKTLEN) {
				printf("-s <1 - %d>\n", PKTLEN);
				return -1;
			}
			g_pkt_size = size;
			break;

		case 'n':
			num = atoi(optarg);
			if (num <= 0) {
				printf("-n <positive integer>\n");
				return -1;
			}
			g_thread_num = num;
			break;
		
		case ':':
			printf("option %c need argument\n", optopt);
			return -1;
		
		case '?':
			printf("unkowned option %c\n", optopt);
			return -1;
		}
	}

	if (optind < argc)
		return -1;
	
	if (g_svr_ip[0] == '\0')
		return -1;

	return 0;
}

static void _sig_int(int signo)
{
	if (signo == SIGINT) {
		printf("thread %lu recv SIGINT\n", pthread_self());
		printf("user stopped!\n");
		g_stop = 1;
	}
}

static int _init(void)
{
	struct sigaction act;
	sigset_t mask;	

	g_thread_infos = malloc(g_thread_num * sizeof(struct thread_info));
	if (!g_thread_infos) {
		printf("malloc error\n");
		return -1;
	}
	memset(g_thread_infos, 0, g_thread_num * sizeof(struct thread_info));

	memset(&g_svr_addr, 0, sizeof(g_svr_addr));
	g_svr_addr.sin_family = AF_INET;
	g_svr_addr.sin_port = htons(g_svr_port);
	if (inet_pton(AF_INET, g_svr_ip, &g_svr_addr.sin_addr) < 1) {
		printf("inet_pton error: %s\n", strerror(errno));
		return -1;
	}

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	if (sigprocmask(SIG_BLOCK, &mask, NULL)) {
		printf("sigprocmask error: %s\n", strerror(errno));
		return -1;
	}

	memset(&act, 0, sizeof(act));
	act.sa_handler = _sig_int;
	act.sa_flags = 0;
	act.sa_restorer = NULL;
	sigemptyset(&act.sa_mask);

	if (sigaction(SIGINT, &act, NULL)) {
		printf("sigaction error: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}

static int _cleanup(void)
{
	if (g_thread_infos)
		free(g_thread_infos);

	return 0;
}

static void _print_info(void)
{
	struct thread_info *info;
	int i;

	for (i = 0; i < g_thread_num; i++) {
		info = &g_thread_infos[i];
		printf("thread %lu info:\n", info->tid);
		printf("spend time: %d seconds\n", (info->end - info->begin));
		printf("connections: %d\n", info->nconnects);
		printf("bytes: %lld\n", info->nbytes);
	}
}

static int _do_process(struct thread_info *info)
{
	int fd;
	int n;
	char buf[PKTLEN];

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		printf("socket error: %s\n", strerror(errno));
		return -1;
	}

	if (connect(fd, (struct sockaddr *)&g_svr_addr, sizeof(g_svr_addr))) {
		printf("connect error(%d): %s\n", errno, strerror(errno));
		close(fd);
		return -1;
	}

	n = send(fd, g_data, g_pkt_size, 0);
	if (n != g_pkt_size) {
		printf("send error: %s\n", strerror(errno));
		close(fd);
		return -1;
	}

	n = recv(fd, buf, g_pkt_size, 0);
	if (n != g_pkt_size) {
		printf("recv error(%d): %s\n", n, strerror(errno));
		close(fd);
		return -1;
	}

	info->nconnects ++;
	info->nbytes += n;

	close(fd);
	
	return 0;
}

static void *_thread_run(void *args)
{
	struct thread_info *info = args;

	pthread_detach(info->tid);

	info->begin = time(NULL);

	while (!g_stop) {
		if (_do_process(info))
			break;
	}

	info->end = time(NULL);

	return 0;
}

int main(int argc, char **argv)
{
	int i;
	sigset_t mask;

	printf("thread %lu run\n", pthread_self());

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	_init();

	for (i = 0; i < g_thread_num; i++) {
		if (pthread_create(&g_thread_infos[i].tid, NULL, 
				   _thread_run, &g_thread_infos[i])) 
		{
			printf("create thread error\n");
			break;
		}
		printf("thread %lu create\n", g_thread_infos[i].tid);
	}

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	if (sigprocmask(SIG_UNBLOCK, &mask, NULL)) {
		printf("sigprocmask error: %s\n", strerror(errno));
		return -1;
	}

	pause();

	sleep(1);

	_print_info();

	_cleanup();

	return 0;
}

