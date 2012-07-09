/**
 *	@file	httpsvr.c
 *
 *	@brief	The simple HTTP server implement.
 *	
 *	@date	2009-07-01
 */


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

static u_int32_t	_g_svr_ip = 0;		/* Listen IP */
static u_int16_t	_g_svr_port = 8080;	/* Listen port */
static int		_g_ret_code = 200;	/* return code */
static char		_g_head_file[FZ_NAME_MAX + 1];	/* head file */
static char		_g_body_file[FZ_NAME_MAX + 1];	/* body file */
static char		*_g_head;		/* HTTP header buffer */
static int		_g_hlen;		/* HTTP header length */
static int		_g_head_len;		/* HTTP header buffer length */
static char		*_g_body;		/* HTTP body buffer */
static int		_g_blen;		/* HTTP body length */
static int		_g_body_len;		/* HTTP body buffer length */
static int		_g_http_ver;		/* HTTP version */
static int		_g_listen_fd = -1;	/* listen socket fd */
static int		_g_stop;


/**
 *	Show help message	
 *	
 *	No return.
 */
static void 
_usage(void)
{
	printf("httpsvr [options]\n");
	printf("\t-a\tListen address\n");
	printf("\t-p\tport address\n");
	printf("\t-d\tresponse head\n");
	printf("\t-b\tresponse body\n");
	printf("\t-v\thttp version: 0 is HTTP/1.0, 1 is HTTP/1.1\n");
	printf("\t-r\tresponse code, default 200\n");
	printf("\t-h\tshow help message\n");
}

/**
 *	Parse command line argument.	
 *
 * 	Return 0 if parse success, -1 on error.
 */
static int 
_parse_cmd(int argc, char **argv)
{
	char opt;
	char optstr[] = ":a:p:v:d:b:r:h";
	int port;

	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {
			
		case 'a':
			_g_svr_ip = inet_addr(optarg);		       
			break;

		case 'p':
			port = atoi(optarg);
			if (port < 1 || port > 65535) {
				printf("port %s out of range(1 - 65535)", 
				       optarg);
				return -1;
			}
			_g_svr_port = port;
			break;

		case 'v':
			if (strcmp(optarg, "0") == 0)
				_g_http_ver = HTTP_VER_10;
			else if (strcmp(optarg, "1") == 0)
				_g_http_ver = HTTP_VER_11;
			else {
				printf("unkowned HTTP version %s\n", optarg);
				return -1;
			}
			break;

		case 'd':
			strncpy(_g_head_file, optarg, FZ_NAME_MAX);
			break;

		case 'b':
			strncpy(_g_body_file, optarg, FZ_NAME_MAX);
			break;

		case 'r':
			_g_ret_code = atoi(optarg);
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

	return 0;
}


static void _sig_stop(int signo)
{
	if (signo == SIGINT) {
		printf("user stopped\n");
		_g_stop = 1;
	}
}

/**
 *	Return HTTP reponse status code description.
 */
static const char *
_get_res_reason(int retcode)
{
	switch (retcode) {
	case 200:
		return "OK";
	case 201:
		return "Created";
	case 202:
		return "Accepted";
	case 203:
		return "Non-Authoritative Information";
	case 204:
		return "No Content";
	case 205:
		return "Reset Content";
	case 206:
		return "Partial Content";

	case 300:
		return "Multiple Choices";
	case 301:
		return "Moved Permanently";
	case 302:
		return "Found";
	case 303:
		return "See Other";
	case 304:
		return "Not Modified";
	case 305:
		return "Use Proxy";
	case 306:
		return "Unused";
	case 307:
		return "Temporary Redirect";

	case 400:
		return "Bad Request";
	case 401:
		return "Unauthorized";
	case 402:
		return "Payment Required";
	case 403:
		return "Forbidden";
	case 404:
		return "Not Found";
	case 405:
		return "Method Not Allowed";
	case 406:
		return "Not Acceptable";
	case 407:
		return "Proxy Authentication Required";
	case 408:
		return "Request Timeout";
	case 409:
		return "Conflict";

	case 500:
		return "Internal Server Error";
	case 501:
		return "Not Implemented";
	case 502:
		return "Bad Gateway";
	case 503:
		return "Service Unavailable";
	case 504:
		return "Gateway Timeout";
	case 505:
		return "HTTP version Not Supported";

	default:
		return "Unkown";
	}
}

/**
 *	Create HTTP response line
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_create_http_response_line(size_t hlen)
{
	char retcode[64] = {0};

	if (_g_http_ver == HTTP_VER_11)
		strncat(_g_head, "HTTP/1.1 ", hlen - 1);
	else 
		strncat(_g_head, "HTTP/1.0 ", hlen - 1);

	snprintf(retcode, 63, " %d %s", _g_ret_code, 
		 _get_res_reason(_g_ret_code) );

	strncat(_g_head, retcode, hlen - 1);

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
	_create_http_response_line(hlen);

	/* http other header line */
	if (buf) {
		int fin = 0;
		
		if (!fin) 
			fin = _filter_head_file(buf, '\r');
		
		if (!fin)
			fin = _filter_head_file(buf, '\n');		
	}

	strncat(_g_head, "Content-Type: text/html\r\n", hlen - 1);

	/* add content-length */
#if 0
	if (_g_body && _g_http_ver == HTTP_VER_11) {
		char clen[128] = {0};
		snprintf(clen, 127, "Content-Length: %d\r\n", _g_blen);
		strncat(_g_head, clen, hlen - 1);		
	}
#endif

	/* Add head/body split line */
	strncat(_g_head, "\r\n", hlen - 1);
	
	_g_hlen = strlen(_g_head);

	return 0;
}


/**
 *	Init some global resource used in program.	
 *
 * 	Return 0 if success, -1 on error.
 */
static int 
_initiate(void)
{
	int fd;
	struct stat st;
	int len;

	/* read body file */
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
	/* read head file */
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

	signal(SIGINT, _sig_stop);
	
	_g_listen_fd = sock_tcpsvr(_g_svr_ip, htons(_g_svr_port));
	if (_g_listen_fd < 0)
		return -1;

	return 0;
}


/**
 *	Release global resource alloced by _initiate().	
 *
 * 	No Return.
 */
static void 
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

	if (_g_listen_fd >= 0) {
		close(_g_listen_fd);
		_g_listen_fd = -1;
	}
}

static int 
_do_recv(int fd)
{
	char buf[1024] = {0};
	int m, total = 0;
	int retries = 0;
	http_info_t info;	

	memset(&info, 0, sizeof(info));

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

			printf("recv client failed\n");

			return -1;
		}
		if (m == 0) {
			printf("\nrecv client closed\n");
			return 0;
		}
		
		if (http_parse_request(&info, buf, m, 0)) {
			printf("parse request failed\n");
			return -1;
		}
		
		if (info.state.mstate == HTTP_STE_FIN) {
			printf("request finished:\n%s\n", buf);
			return 0;
		}

		buf[m] = 0;
		total += m;
		retries++;

		printf("request non-fin(%d):\n%s\n", info.state.mstate, buf);

	} while (retries < 2);

	return 0;
}

static int 
_do_send(int fd)
{
	int ret = 0;

# if 0
	char *buf = NULL;
	int len;

	len = _g_hlen + _g_blen;
	buf = malloc(len + 1);
	if (!buf)
		return -1;

	if (_g_head)
		memcpy(buf, _g_head, _g_hlen);

	if (_g_body)
		memcpy(buf + _g_hlen, _g_body, _g_blen);

	buf[len] = 0;

	if (_send_fin(fd, buf, len) < 0)
		return -1;
#endif
	
	if (_g_head) {
		ret = sock_send(fd, _g_head, _g_hlen);
		if (ret < 0) {
			printf("send header failed\n");
			return -1;
		}
		printf("send header (%d): \n%s\n", _g_hlen,  _g_head);
	}
	
	sleep(2);

	if (_g_body) {
		ret = sock_send(fd, _g_body, _g_blen);
		if (ret < 0) {
			printf("send body failed\n");
			return -1;
		}
		printf("send body (%d): \n%s\n", _g_blen,  _g_body);
	}	

	return 0;
}

static int 
_do_process(int clifd)
{
	_do_recv(clifd);
	_do_send(clifd);
	
	close(clifd);
	printf("closed clifd\n");

	return 0;
}

static int 
_do_accept(void)
{
	int clifd = -1;
	u_int32_t ip = 0;
	u_int16_t port = 0;

	clifd = sock_accept(_g_listen_fd, &ip, &port);
	if (clifd >= 0) {
		printf("accept %u.%u.%u.%u:%u\n", NIPQUAD(ip), ntohs(port));

		_do_process(clifd);
	}
	
	return 0;
}

static int 
_do_loop(void)
{
	while (!_g_stop) {
		_do_accept();
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

	if (_initiate()) {
		return -1;
	}

	_do_loop();

	_release();

	return 0;
}



