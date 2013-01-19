/*
 *	@file	sock_test.c
 *
 *	@brief	the sock.c APIs test program
 *	
 *	@date	2009-08-30
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <getopt.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "sock.h"

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


static u_int32_t _g_ip = 0;
static u_int16_t _g_port = 8080;
static int _g_loop_times = 1;
static int _g_nonblock = 0;
static int _g_timeout = 0;
static int _g_server = 0;
static int _g_buflen = 1024;
static char *_g_buffer = NULL;

static void _usage(void)
{
	printf("sock_test <options>\n");
	printf("\t-t\tconnection timeout\n");
	printf("\t-s\tact as server\n");
	printf("\t-i\tserver address\n");
	printf("\t-p\tserver port\n");
	printf("\t-b\tsend buffer size\n");
	printf("\t-o\tusing non-block socket\n");
	printf("\t-n\tconnect <N> times, default is 1\n");
	printf("\t-h\tshow help message\n");
}

static int _parse_cmd(int argc, char **argv)
{
	char opt;
	char optstr[] = ":t:i:p:n:b:osh";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {

		case 't':
			_g_timeout = atoi(optarg);
			if (_g_timeout < 0)
				return -1;
			break;

		case 'i':
			_g_ip = inet_addr(optarg);
			break;

		case 'p':
			_g_port = atoi(optarg);
			break;

		case 'n':
			_g_loop_times = atoi(optarg);
			if (_g_loop_times < 1)
				return -1;
			break;

		case 'b':
			_g_buflen = atoi(optarg);
			if (_g_buflen < 1)
				return -1;
			break;

		case 's':
			_g_server = 1;
			break;
			
		case 'o':
			_g_nonblock = 1;
			break;

		case 'h':
			return -1;

		case ':':
			printf("option %c missing argument\n", optopt);
			return -1;

		case '?':
			printf("unknowed option %c\n", optopt);
			return -1;
		}
	}

	if (argc != optind)
		return -1;

	if (!_g_port)
		return -1;

	if (_g_server == 0 && _g_ip == 0)
		return -1;
	
	_g_port = htons(_g_port);

	return 0;
}

static int _initiate(void)
{
	if (_g_buflen > 0) {
		_g_buffer = malloc(_g_buflen + 1);
		if (!_g_buffer)
			return -1;
	}
	
	return 0;
}


static void _release(void)
{
	if (_g_buffer) {
		free(_g_buffer);
		_g_buffer = NULL;
	}
}

/**
 *	Recv data from peer, we need just recv @len bytes, if not recv
 *	all of them, return -1; if more bytes then @len, return -1.
 *
 *	Return 0 if success, -1 on error.
 */
static int
_do_recv(int fd, char *buf, size_t len) 
{
	int n;
	int remain;
	int pos;
	int closed = 0;

	if (!buf || len < 1)
		return -1;

	remain = len;
	pos = 0;
	while (remain > 0) {
		n = sock_recv(fd, buf + pos, remain, &closed);
		if (n < 0) {
			printf("recv error\n");
			return -1;
		}

		if (closed)
			return pos;

		remain -= n;
		pos += n;
	}

	return 0;
}

static int 
_do_verify(const char *buf, size_t len, char c)
{
	int i;

	if (!buf || len < 1)
		return -1;

	for (i = 0; i < len; i++)
		if (buf[i] != c)
			return -1;

	return 0;
}

static int 
_do_server(void)
{
	int listenfd;
	int fd;
	int i;
	int failed = 0;
	int success = 0;
	int n = 0;
	u_int32_t ip;
	u_int16_t port;

	listenfd = sock_tcpsvr(_g_ip, _g_port);
	if (listenfd < 0)
		return -1;

	for (i = 0; i < _g_loop_times; i++) {
		fd = sock_accept(listenfd, &ip, &port);
		if (fd < 0) {
			printf("accept error\n");
			return -1;
		}

		n = _do_recv(fd, _g_buffer, _g_buflen);
		if (n < 0) {
			failed++;
			close(fd);
			continue;
		}
		
		/* verify recved data */
		if (_do_verify(_g_buffer, _g_buflen, 'b')) {
			failed++;
			close(fd);
			continue;
		}

		memset(_g_buffer, 'a', _g_buflen);
		n = sock_send(fd, _g_buffer, _g_buflen);
		if (n != _g_buflen) {
			failed++;
			close(fd);
			continue;
		}

		success++;
		close(fd);
	}

	close(listenfd);

	printf("Accept connect %d, success %d, failed %d\n",
	       i, success, failed);

	return 0;
}


static int
_do_client(void)
{
	int fd;
	int i;
	int n;
	int success = 0;
	int failed = 0;

	for (i = 0; i < _g_loop_times; i++) {

		fd = sock_tcpcli(_g_ip, _g_port);

		if (fd >= 0) {
			
			memset(_g_buffer, 'b', _g_buflen);
			n = sock_send(fd, _g_buffer, _g_buflen);
			if (n < 0) {
				failed++;
				close(fd);
				continue;
			}

			shutdown(fd, SHUT_WR);

			n = _do_recv(fd, _g_buffer, _g_buflen);
			if (n < 0) {
				failed++;
				close(fd);
				continue;
			}
			
			if (_do_verify(_g_buffer, _g_buflen, 'a')) {
				failed++;
				close(fd);
				continue;
			}
				

			shutdown(fd, SHUT_RD);

			success++;
			close(fd);
		}
		else {
			printf("connect %d times failed\n", i);
			failed++;
		}
	}

	printf("Try connect %d, success %d, failed %d\n",
	       i, success, failed);

	return 0;
}


int main(int argc, char **argv)
{
	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_initiate()) {
		return -1;
	}


	if (_g_server)
		_do_server();
	else
		_do_client();

	_release();

	return 0;
}

