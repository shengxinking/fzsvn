/**
 *
 *
 */

#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <getopt.h>

#include "sock.h"

#define MAX_CLIENTS	1024

static u_int16_t _g_port = 8000;
static u_int32_t _g_ip = 0;
static int _g_listenfd = -1;
static int _g_epfd = -1;
static int _g_accept_count = 1000;
static int _g_client_count = 0;

static void 
_usage(void)
{
	printf("tcpsvr_epoll [options]\n");
	printf("\t-a\tserver address\n");
	printf("\t-p\tserver port<1 - 65535>\n");
	printf("\t-t\taccept times\n");
	printf("\t-h\tshow help message\n");
}

static int 
_parse_cmd(int argc, char **argv)
{
	char c;
	int port;
	char optstr[] = ":a:p:t:h";

	opterr = 0;
	while ( (c = getopt(argc, argv, optstr)) != -1) {
		switch (c) {
	
		case 'a':
			_g_ip = inet_addr(optarg);
			break;
		
		case 'p':
			port = atoi(optarg);
			if (port <= 0 || port > 65535) {
				return -1;
			}
			_g_port = htons(port);
			break;

		case 't':
			_g_accept_count = atoi(optarg);
			if (_g_accept_count < 1)
				return -1;
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

	if (!_g_ip || !_g_port)
		return -1;

	return 0;
}

static int 
_initiate(void)
{
	struct epoll_event event;

	_g_listenfd = sock_tcpsvr(_g_ip, _g_port, 1);
	if (_g_listenfd < 0)
		return -1;

	_g_epfd = epoll_create(MAX_CLIENTS);
	if (_g_epfd < 0)
		return -1;

	memset(&event, 0, sizeof(event));
	event.data.fd = _g_listenfd;
	event.events = EPOLLIN;
	if (epoll_ctl(_g_epfd, EPOLL_CTL_ADD, _g_listenfd, &event))
		return -1;

	return 0;
}

static void 
_release(void)
{
	if (_g_listenfd >= 0)
		close(_g_listenfd);

	if (_g_epfd >= 0)
		close(_g_epfd);
}

static int 
_do_accept(void)
{
	int clifd;
	struct epoll_event event;

	while (_g_accept_count > 0) {
		clifd = sock_accept(_g_listenfd, NULL, NULL, 0);
		if (clifd < 0) {
			if (errno == EINTR)
				break;
			if (errno == EAGAIN)
				break;
			return 0;
		}

		printf("a client(%d) connect to me\n", clifd);

		memset(&event, 0, sizeof(event));
		event.events = EPOLLIN;
		event.data.fd = clifd;
		if (epoll_ctl(_g_epfd, EPOLL_CTL_ADD, clifd, &event)) {
			printf("epoll add failed: %s\n", strerror(errno));
			close(clifd);
			return -1;
		}
		printf("add %d to epoll fd %d\n", clifd, _g_epfd);

		_g_accept_count--;
		_g_client_count++;
	}
	
	return 0;
}

#ifndef BUFLEN
#define BUFLEN	1024
#endif
static int _do_read(int fd)
{
	char buf[BUFLEN] = {0};
	int rlen, wlen;
	int isclose = 0;
	
	if (fd < 0)
		return -1;

	rlen = sock_tcprecv(fd, buf, BUFLEN - 1, -1, &isclose);
	if (rlen == 0 && isclose == 1) {
		printf("recv client closed\n");
		
		_g_client_count--;
		if (epoll_ctl(_g_epfd, EPOLL_CTL_DEL, fd, NULL)) {
			printf("epoll_ctl(EPOLL_CTL_DEL) error: %s\n",
			       strerror(errno));
			close(fd);
			return 0;
		}
		close(fd);
		return -1;
	}

	wlen = sock_tcpsend(fd, buf, rlen, 0);
	if (wlen != rlen) {
		printf("send client error: %s\n", strerror(errno));
		close(fd);
		return -1;
	}

	return 0;
}

static int _do_loop()
{
	int i;
	struct epoll_event events[MAX_CLIENTS];
	int nfds = 0;
	int accept_success = 0, accept_failed = 0;
	int read_success = 0, read_failed = 0;


	while (_g_accept_count || _g_client_count) {
		nfds = epoll_wait(_g_epfd, events, MAX_CLIENTS, 1000);
		if (nfds < 0) {
			if (errno == EINTR)
				continue;
			printf("epoll error: %s\n", strerror(errno));
			break;
		}

		if (nfds == 0)
			continue;

		for (i = 0; i < nfds; i++) {
			if (events[i].data.fd == _g_listenfd) {
				if (_g_accept_count > 0) {
					if (_do_accept())
						accept_failed++;
					else
						accept_success++;
				}
			}
			else {
				if(_do_read(events[i].data.fd))
					read_failed++;
				else
					read_success++;
			}
		}
	}

	printf("Accept(%d), success %d, failed %d\n",
	       _g_accept_count, accept_success, accept_failed);

	printf("Read, success %d, failed %d\n",
	       read_success, read_failed);
	
	return 0;
}

int main(int argc, char **argv)
{
	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_initiate()) {
		_release();
		return -1;
	}

	_do_loop();

	_release();

	return 0;
}

