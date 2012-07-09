/*
 *	@file	http_client.c
 *
 *	@brief	a simple HTTP client, it send HTTP request, and recv
 *		HTTP response.
 *
 *	@auth	FZ
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "http_protocol.h"
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

#define FZ_NAME_MAX	63

static u_int32_t	_g_svr_ip;		/* HTTP server IP */
static u_int16_t	_g_svr_port;		/* HTTP server port */
static char	_g_head_file[FZ_NAME_MAX + 1];	/* header file */
static char	_g_body_file[FZ_NAME_MAX + 1];	/* body file */
static char	*_g_head;			/* HTTP header buffer */
static int	_g_hlen = 0;			/* HTTP header length */
static int	_g_head_len = 0;		/* header buffer length */
static char	*_g_body;			/* HTTP body buffer */
static int 	_g_blen;			/* HTTP body length */
static int	_g_body_len;			/* HTTP body buffer length */
static int	_g_http_ver = HTTP_VER_11;	/* HTTP version */
static int	_g_http_method;			/* HTTP method */
static char	_g_http_url[1024];		/* http URL */


/**
 *	Show help message.
 *
 *	No return.
 */
static void 
_usage(void)
{
	printf("http_client <options>\n");
	printf("\t-a\tHTTP server IP address\n");
	printf("\t-p\tHTTP server port\n");
	printf("\t-v <0 | 1>\tHTTP request version\n");
	printf("\t-m <get | post>\tHTTP request method\n");
	printf("\t-u\tHTTP URL\n");
	printf("\t-d <file>\tHTTP request header\n");
	printf("\t-b <file>\tget HTTP body from file");
	printf("\t-h\tshow help message\n");
}

/**
 *	Parse command line arguments.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_parse_cmd(int argc, char **argv)
{
	char optstr[] = ":a:p:m:v:b:d:u:h";
	char c;
	int port;

	opterr = 0;
	while ( (c = getopt(argc, argv, optstr)) != -1) {
		
		switch (c) {
			
		case 'a':
			_g_svr_ip = inet_addr(optarg);
			break;

		case 'p':
			port = atoi(optarg);
			if (port < 1 || port > 65535) {
				printf("port(%s) out of 1-65535", optarg);
				return -1;
			}
			_g_svr_port = port;
			break;

		case 'm':
			if (strcmp(optarg, "get") == 0)
				_g_http_method = HTTP_MED_GET;
			else if (strcmp(optarg, "post") == 0)
				_g_http_method = HTTP_MED_POST;
			else {
				printf("unkowned http method %s\n", optarg);
				return -1;
			}
			break;

		case 'v':
			if (strcmp(optarg, "0") == 0)
				_g_http_ver = HTTP_VER_10;
			else if (strcmp(optarg, "1") == 0)
				_g_http_ver = HTTP_VER_11;
			else {
				printf("unkowned http version %s\n", optarg);
				return -1;
			}
			break;

		case 'b':
			strncpy(_g_body_file, optarg, FZ_NAME_MAX);
			break;

		case 'd':
			strncpy(_g_head_file, optarg, FZ_NAME_MAX);
			break;

		case 'u':
			strncpy(_g_http_url, optarg, 1023);
			break;

		case 'h':
			_usage();
			exit(0);

		case ':':
			printf("option %c missing argument\n", optopt);
			return -1;

		case '?': 
			printf("unkowned option %c\n", optopt);
			return -1;
		}
	}

	if (argc != optind) {
		_usage();
		return -1;
	}
	
	if (!_g_svr_ip || !_g_svr_port) {
		printf("need set HTTP server using -a -p option\n");
		return -1;
	}

	return 0;
}

static int 
_create_http_request_line(size_t hlen)
{
	if (_g_http_method == HTTP_MED_POST)
		strncat(_g_head, "POST ", hlen - 1);
	else
		strncat(_g_head, "GET ", hlen - 1);
	
	if (strlen(_g_http_url) > 0) 
		strncat(_g_head, _g_http_url, hlen - 1);
	else
		strncat(_g_head, "/", hlen - 1);

	if (_g_http_ver == HTTP_VER_11)
		strncat(_g_head, " HTTP/1.1 ", hlen - 1);
	else 
		strncat(_g_head, " HTTP/1.0 ", hlen - 1);


	strncat(_g_head, "\r\n", hlen - 1);

	return 0;
}


static int 
_filter_head_file(char *buf, char d)
{
	char *ptr, *begin;
	char c;
	int hlen = _g_head_len;
	int fin = 0;
		
	begin = buf;
	ptr = strchr(begin, d);
	if (ptr)
		fin = 1;
	while (ptr) {
		c = *ptr;
		*ptr = 0;

		strncat(_g_head, begin, hlen - 1);
		strncat(_g_head, "\r\n", hlen - 1);

		begin = ptr + 1;
		if (*begin == '\n')
			begin++;
		
		if (*begin == 0)
			break;
		
		*ptr = c;		
		ptr = strchr(begin, d);
	}

	return fin;
}

/**
 *	Create HTTP header into @_g_head.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_create_http_head(char *buf, size_t len)
{
	int hlen = 0;

	if (buf && len > 0) {
		hlen = len + 1024;
	}
	else {
		hlen = 1024;
	}

	_g_head_len = hlen;
	_g_head = malloc(hlen);
	if (!_g_head) {
		printf("malloc memory for head(%d) failed\n", hlen);
		return -1;
	}
	memset(_g_head, 0, hlen);

	/* http request line */
	_create_http_request_line(hlen);

	/* http other header line */
	if (buf) {
		int fin = 0;
		
		if (!fin) 
			fin = _filter_head_file(buf, '\r');
		
		if (!fin)
			fin = _filter_head_file(buf, '\n');		
	}

	/* add Host */
	if (_g_svr_ip && _g_svr_port) {
		char host[128] = {0};
		snprintf(host, 127, "Host: %u.%u.%u.%u:%u\r\n", 
			 NIPQUAD(_g_svr_ip), _g_svr_port);
		strncat(_g_head, host, hlen - 1);
	}

	/* add content-length */
	if (_g_body) {
//		char clen[128] = {0};
//		snprintf(clen, 127, "Content-Length: %d\r\n", _g_blen);
//		strncat(_g_head, clen, hlen - 1);		
	}

	/* Add head/body split line */
	strncat(_g_head, "\r\n", hlen - 1);
	
	_g_hlen = strlen(_g_head);

	return 0;
}

/**
 *	Initiate the resource.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_initiate(void)
{
	int len = 0;
	struct stat st;
	int fd;

	if (strlen(_g_body_file) > 0) {
		fd = open(_g_body_file, O_RDONLY);
		if (fd < 0) {
			printf("open body file %s failed\n", _g_body_file);
			return -1;
		}

		if (fstat(fd, &st)) {
			printf("stat body file %s failed\n", _g_body_file);
			close(fd);
			return -1;
		}

		len = st.st_size;
		_g_body_len = len + 1;
		_g_body = malloc(len + 1);		
		if (!_g_body) {
			printf("malloc memory for body(%d) failed\n", len);
			close(fd);
			return -1;
		}

		if (read(fd, _g_body, len) != len) {
			printf("read body file %s failed\n", _g_body_file);
			close(fd);
			return -1;
		}

		_g_body[len] = 0;
		_g_blen = len;
		close(fd);
	}

	if (strlen(_g_head_file) > 0) {
		char *buf = NULL;

		fd = open(_g_head_file, O_RDONLY);
		if (fd < 0) {
			printf("open head file %s failed\n", _g_head_file);
			return -1;
		}

		if (fstat(fd, &st)) {
			printf("stat head file %s failed\n", _g_head_file);
			close(fd);
			return -1;
		}

		len = st.st_size;
		buf = malloc(len + 1);
		if (!buf) {
			printf("malloc memory for head(%d) failed\n", len);
			close(fd);
			return -1;
		}

		if (read(fd, buf, len) != len) {
			printf("read body file %s failed\n", _g_head_file);
			free(buf);
			close(fd);
			return -1;
		}
		buf[len] = 0;

		_create_http_head(buf, len);

		free(buf);
		close(fd);
	}
	else {
		_create_http_head(NULL, 0);
	}

	return 0;
}

/**
 *	Free the resource.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_release(void)
{
	if (_g_head) {
		free(_g_head);
		_g_hlen = 0;
		_g_head_len = 0;
	}

	if (_g_body) {
		free(_g_body);
		_g_blen = 0;
		_g_body_len = 0;
	}

	return 0;
}


static int 
_do_send(int fd)
{
	char *buf = NULL;
	int left = 0;
	int len = 0;
	int m = 0;
	char *begin = NULL;

	len = _g_hlen + _g_blen;
	buf = malloc(len + 1);
	if (!buf)
		return -1;

	if (_g_head)
		memcpy(buf, _g_head, _g_hlen);

	if (_g_body)
		memcpy(buf + _g_hlen, _g_body, _g_blen);

	buf[len] = 0;

	begin = buf;
	left = len;
	while (left > 0) {
		m = send(fd, begin, left, MSG_DONTWAIT);
		if (m < 0) {
			if (errno == EAGAIN)
				continue;
			if (errno == EINTR)
				continue;
			printf("send to server failed\n");

			return -1;
		}

		left -= m;
		begin += m;
	}

	printf("send(%d): \n%s\n", len, buf);

	return 0;
}

static int 
_do_recv(int fd)
{
	char buf[1024] = {0};
	int m, total = 0;
	int retries = 0;
	http_info_t info;
	
	memset(&info, 0, sizeof(info));

	printf("recved:\n");
	do {
		m = recv(fd, buf, 1023, MSG_DONTWAIT);
		if (m < 0) {
			if (errno == EAGAIN) {
				sleep(1);
				retries++;
				continue;
			}
			if (errno == EINTR)
				break;

			printf("recv server failed\n");

			return -1;
		}
		else if (m == 0) {
			printf("recv server closed\n");
			return 0;
		}
		
		if (http_parse_response(&info, buf, m, 0)) {
			printf("HTTP parse response failed\n");
			return -1;
		}
		if (info.state.mstate == HTTP_STE_FIN) {
			printf("http response finished(%d):\n%s\n", 
			       m, buf);
			continue;
		}

		buf[m] = 0;
		total += m;
		retries++;

		printf("http response not-finished(%d):\n%s\n",
		       info.state.mstate, buf);

	} while (retries < 1000);

	return 0;
}


static int 
_do_process(void)
{
	int fd;

	fd = sock_tcpcli(_g_svr_ip, htons(_g_svr_port));
	if (fd < 0) {
		printf("connect to %u.%u.%u.%u:%u failed\n",
		       NIPQUAD(_g_svr_ip), _g_svr_port);
		return -1;
	}
	
	printf("connect to %u.%u.%u.%u:%u success\n",
		       NIPQUAD(_g_svr_ip), _g_svr_port);

	if (_do_send(fd) < 0)
		return -1;

	if (_do_recv(fd) < 0)
		return -1;

	close(fd);

	return 0;
}

int main(int argc, char **argv)
{
	if (_parse_cmd(argc, argv))
		return -1;

	if (_initiate()) 
		return -1;

	_do_process();

	_release();

	return 0;
}


