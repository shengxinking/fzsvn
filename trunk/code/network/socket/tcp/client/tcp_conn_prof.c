/*
 *	tcp_conn_prof:		test tcp connection profermance. using gettimeof day to print time
 *
 *
 *	author:			forrest.zhang
 */

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define IPLEN	32
static struct sockaddr_in g_svr_addr;
static char g_svr_ip[IPLEN] = {0};
static short g_svr_port = 8000;

static void _usage(void)
{
	printf("tcp_conn_prof [options]\n");
	printf("tcp_conn_prof:  test the connection time\n");
	printf("-a\tserver address\n");
	printf("-p\tserver port\n");
	printf("-h\tshow help infomation\n");
}

static int _parse_cmd(int argc, char **argv)
{
	char c;
	int port;
	char optstr[] = ":a:p:h";

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

		case 'h':
			return -1;
		
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

static int _init(void)
{
	memset(&g_svr_addr, 0, sizeof(g_svr_addr));
	g_svr_addr.sin_family = AF_INET;
	g_svr_addr.sin_port = htons(g_svr_port);
	if (inet_pton(AF_INET, g_svr_ip, &g_svr_addr.sin_addr) < 1) {
		printf("inet_pton error: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}

static int _release(void)
{

	return 0;
}


int _process(void)
{
	int fd;
	struct timeval tv_begin, tv_end;

	gettimeofday(&tv_begin, NULL);
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		printf("socket error: %s\n", strerror(errno));
		return -1;
	}
	gettimeofday(&tv_end, NULL);
	printf("spend %lu second %lu microsecond to create a tcp socket\n",
		tv_end.tv_sec - tv_begin.tv_sec, tv_end.tv_usec - tv_begin.tv_usec);

	gettimeofday(&tv_begin, NULL);
	if (connect(fd, (struct sockaddr *)&g_svr_addr, sizeof(g_svr_addr))) {
		printf("connect error(%d): %s\n", errno, strerror(errno));
		close(fd);
		return -1;
	}
	gettimeofday(&tv_end, NULL);
	printf("spend %lu second %lu microsecond to connect %s:%u\n",
		tv_end.tv_sec - tv_begin.tv_sec, tv_end.tv_usec - tv_begin.tv_usec,
		g_svr_ip, g_svr_port);

	close(fd);

	{
	int i = 0;
	gettimeofday(&tv_begin, NULL);
	for (i = 0; i < 10000; i++)
		i = i + 1 - 1;
	gettimeofday(&tv_end, NULL);
	printf("spend %lu second %lu microsecond to 10000 loops\n",
		tv_end.tv_sec - tv_begin.tv_sec, tv_end.tv_usec - tv_begin.tv_usec);
	}

	return 0;
}


int main(int argc, char **argv)
{
	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_init())
		return -1;

	_process();

	if (_release())
		return -1;

	return 0;
}


