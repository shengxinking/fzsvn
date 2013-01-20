/*
 *
 *
 */

#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <sys/resource.h>

#define CLIENT_MAX	(1024 * 40)
#define IPLEN		32
#define BUFLEN		1024
#define POLL_TIME	(1024 * 1024)

static char g_svr_ip[IPLEN] = { 0 };
static short g_svr_port = 8080;
static int g_conns[CLIENT_MAX];
static struct pollfd g_polls[CLIENT_MAX];
static int g_conn_num = 0;

static int g_begin;
static int g_end;
static int g_stop = 0;
static long long g_nbytes = 0;
static long long g_nconns = 0;

static void _usage(void)
{
	printf("tcpsvr_select [options]\n\n");
	printf("-a\tserver address\n");
	printf("-p\tserver port\n");
	printf("-h\tshow help message\n");
}

static int _parse_cmd(int argc, char **argv)
{
	char c;
	int port;
	char optstr[] = ":a:p:h";

	while ( (c = getopt(argc, argv, optstr)) != -1) {
		switch (c) {
	
		case 'a':
			strncpy(g_svr_ip, optarg, IPLEN - 1);
			break;
		
		case 'p':
			port = atoi(optarg);
			if (port <= 0 || port > 65535) {
				printf("-p <1 - 65535>");
				return -1;
			}
			g_svr_port = port;
			break;

		case 'h':
			return -1;

		case ':':
			printf("%c need argument\n", optopt);
			return -1;

		case '?':
			printf("unkowned option %c\n", optopt);
			return -1;

		}
	}

	if (optind < argc)
		return -1;

	return 0;
}

static void _sig_int(int signo)
{
	if (signo == SIGINT) {
		printf("user stopped!\n");
		g_stop = 1;
	}
}

static int _init(void)
{
	int i;
	struct sigaction act;
	struct rlimit rlim;

	for (i = 0; i < CLIENT_MAX; i++)
		g_conns[i] = -1;

	/* block SIGINT signal */
	memset(&act, 0, sizeof(act));
	act.sa_handler = _sig_int;
	act.sa_flags = 0;
	act.sa_restorer = NULL;
	sigemptyset(&act.sa_mask);
	if (sigaction(SIGINT, &act, NULL)) {
		printf("sigaction error: %s\n", strerror(errno));
		return -1;
	}

	/* increase open file number to CLIENT_MAX */
	rlim.rlim_cur = CLIENT_MAX;
	rlim.rlim_max = CLIENT_MAX;
	if (setrlimit(RLIMIT_NOFILE, &rlim)) {
		printf("setrlimit error: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}

static void _cleanup(void)
{
}

static int _create_svr_socket(void)
{
	struct sockaddr_in svraddr;
	int fd;	
	int sockopt;
	long fdflags = 0;

	memset(&svraddr, 0, sizeof(svraddr));
	svraddr.sin_family = AF_INET;
	svraddr.sin_port = htons(g_svr_port);
	svraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		printf("socket error: %s\n", strerror(errno));
		return -1;
	}

	/* reuse address */
	sockopt = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt))) {
		printf("setsockopt SO_REUSEADDR error: %s\n", strerror(errno));
		return -1;
	}

	/* listen socket non-block */
	fdflags = fcntl(fd, F_GETFL, fdflags);
	fdflags |= O_NONBLOCK;
	fcntl(fd, F_SETFL, fdflags);

	if (bind(fd, (struct sockaddr*)&svraddr, sizeof(svraddr))) {
		printf("bind error: %s\n", strerror(errno));
		return -1;
	}

	if (listen(fd, CLIENT_MAX)) {
		printf("listen error: %s\n", strerror(errno));
		return -1;
	}

	return fd;
}

static int _do_accept(int fd)
{
	int clifd;
	struct sockaddr_in cliaddr;
	socklen_t clilen = sizeof(cliaddr);

	while (1) {
		clifd = accept(fd, (struct sockaddr*)&cliaddr, &clilen);
		if (clifd < 0) {
			if (errno == EINTR)
				continue;
			if (errno == EAGAIN)
				break;
			printf("accept error: %s\n", strerror(errno));
			return -1;
		}
		
		if (clifd > CLIENT_MAX) {
			close(clifd);
			break;
		}

		if (g_conns[clifd] > 0) {
			printf("%d already in use\n", clifd);
			break;
		}
		g_conns[clifd] = clifd;
		g_conn_num ++;
		g_nconns ++;
	}
	
	return 0;
}

static int _do_read(int fd)
{
	char buf[BUFLEN] = {0};
	int n;
	
	if (fd < 0) {
		printf("wrong fd when read\n");
		return -1;
	}

	n = read(fd, buf, BUFLEN - 1);
	if (n > 0) {
		printf("read %d bytes from client\n", n);
		g_nbytes += n;
	}
	else {
		printf("client closed\n\n");
		close(fd);
		g_conns[fd] = -1;
		g_conn_num--;
	}
		
	return 0;
}

static int _fill_poll_set(int fd)
{
	int i, j;
	
	g_polls[0].fd = fd;
	g_polls[0].events = POLLIN;
	
	j = 1;
	for (i = 0; i < CLIENT_MAX; i++) {
		if (g_conns[i] > 0) {
			g_polls[j].fd = g_conns[i];
			g_polls[j].events = POLLIN;
			j++;
		}
	}
	return j;	
}

static int _do_loop(int fd)
{
	int n, i;
	int nfds;

	if (fd < 0)
		return -1;

	g_begin = time(NULL);
	while (!g_stop) {

		nfds = _fill_poll_set(fd);

		n = poll(g_polls, nfds, POLL_TIME);
		if (n < 0) {
			if (errno == EINTR)
				continue;
			printf("select error: %s\n", strerror(errno));
			break;
		}

		if (n == 0)
			continue;

		/* have client connect */
		if (g_polls[0].revents & POLLIN)
			_do_accept(fd);

		/* have client data */
		for (i = 1; i < nfds; i++) {
			if ( g_polls[i].revents & POLLIN) {
				_do_read(g_polls[i].fd);
			}
		}

	}
	g_end = time(NULL);
	
	return 0;
}

int main(int argc, char **argv)
{
	int fd;
	int intval;

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	_init();

	fd = _create_svr_socket();
	if (fd < 0)
		return -1;
	
	_do_loop(fd);

	_cleanup();

	intval = g_end - g_begin;
	printf("total %lld connection in %d seconds\n", g_nconns, intval);
	printf("total %lld bytes recv\n", g_nbytes);

	return 0;
}

