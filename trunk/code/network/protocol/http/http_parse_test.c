/*
 *	file	http_test.c
 *
 *	brief	http_protocol test program
 *
 *	author	Forrest.zhang
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdlib.h>

#include "http_protocol.h"

#define _FILE_MAX	32
#define _BUF_MAX	16
#define _SPLIT_MAX	64

static int _g_loop_times = 1;
static int _g_request = 1;
static char _g_header_file[_FILE_MAX];
static char _g_body_file[_FILE_MAX];
static int _g_body_len = 0;
static char _g_split_str[_SPLIT_MAX];

static void
_usage(void)
{
	printf("http_protocol_test <options>\n");
	printf("\t-t <num> \tloop times (need > 0)\n");
	printf("\t-s       \tparse response\n");
	printf("\t-H <file>\thttp header file\n");
	printf("\t-B <file>\thttp body file\n");
	printf("\t-h       \tshow help message\n");
}

static int
_parse_cmd(int argc, char **argv)
{
	char optstr[] = ":t:H:B:sh";
	char opt;

	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		switch (opt) {

		case 't':
			_g_loop_times = atoi(optarg);
			if (_g_loop_times < 1)
				return -1;

			break;

		case 'H':
			strncpy(_g_header_file, optarg, _FILE_MAX - 1);
			if (access(_g_header_file, R_OK))
				return -1;

			break;

		case 'B':
			strncpy(_g_body_file, optarg, _FILE_MAX - 1);
			if (access(_g_body_file, R_OK))
				return -1;

			break;

		case 's':
			_g_request = 0;
			break;

		case 'h':
			return -1;

		case ':':
			printf("option %c missing argument\n", optopt);
			return -1;

		case '?':
			printf("unkowned option %c\n", optopt);
			return -1;
		}
	}

	if (optind != argc)
		return -1;


	return 0;
}

static int
_initiate(void)
{
	struct stat buf;

	if (strlen(_g_body_file) > 0) {
		if (stat(_g_body_file, &buf))
			return -1;

		_g_body_len = buf.st_size;
	}
	else
		_g_body_len = 0;

	snprintf(_g_split_str, _SPLIT_MAX - 1, 
		 "Content-Length: %d\r\n\r\n", 
		 _g_body_len);

	return 0;
}

static void
_release(void)
{
}


static int 
_do_request(void)
{
	http_info_t req;
	int n;
	int pos;
	char buf[_BUF_MAX + 1];
	int fd1, fd2;
	int i;

	fd1 = open(_g_header_file, O_RDONLY);
	if (fd1 < 0)
		return -1;

	if (strlen(_g_body_file) > 0) {
		fd2 = open(_g_body_file, O_RDONLY);
		if (fd2 < 0)
			return -1;
	}
	else
		fd2 = -1;

	for (i = 0; i < _g_loop_times; i++) {
		lseek(fd1, 0L, SEEK_SET);
		memset(&req, 0, sizeof(req));
		pos = 0;

		n = read(fd1, buf, _BUF_MAX);
		while (n > 0) {
			buf[n] = 0;
//			printf("parse: %s\n", buf);
			if (http_parse_request(&req, buf, n, pos))
				return -1;
			
			pos += n;
			n = read(fd1, buf, _BUF_MAX);
		}

		/* add "Content-Length" and "\r\n\r\n" to end of header */
		if (http_parse_request(&req, _g_split_str, strlen(_g_split_str), pos))
			return -1;
		
		if (fd2 < 0)
			continue;

		/* parse body */
		pos += strlen(_g_split_str);
		lseek(fd2, 0L, SEEK_SET);
		n = read(fd2, buf, _BUF_MAX);
		while (n > 0) {
			buf[n] = 0;
//			printf("parse: %s\n", buf);
			if (http_parse_request(&req, buf, n, pos))
				return -1;
			
			pos += n;
			n = read(fd2, buf, _BUF_MAX);
		}
	}

	printf("Host: %s\n", http_get_string(&req, HTTP_STR_HOST));

	printf("parse %d times success\n", _g_loop_times);

	return 0;
}

static int
_do_response(void)
{
	http_info_t res;
	int n;
	int pos;
	char buf[_BUF_MAX];
	int fd1, fd2;
	int i;

	fd1 = open(_g_header_file, O_RDONLY);
	if (fd1 < 0)
		return -1;

	if (strlen(_g_body_file) > 0) {
		fd2 = open(_g_body_file, O_RDONLY);
		if (fd2 < 0)
			return -1;
	}
	else
		fd2 = -1;

	for (i = 0; i < _g_loop_times; i++) {
		lseek(fd1, 0L, SEEK_SET);
		memset(&res, 0, sizeof(res));
		pos = 0;

		n = read(fd1, buf, _BUF_MAX);
		while (n > 0) {
			if (http_parse_response(&res, buf, n, pos))
				return -1;
			
			pos += n;
			n = read(fd1, buf, _BUF_MAX);
		}

		/* add "Content-Length" and "\r\n\r\n" to end of header */
		if (http_parse_response(&res, _g_split_str, strlen(_g_split_str), pos))
			return -1;

		if (fd2 < 0)
			continue;

		/* parse body */
		pos += strlen(_g_split_str);
		lseek(fd2, 0L, SEEK_SET);
		n = read(fd2, buf, _BUF_MAX);
		while (n > 0) {
			if (http_parse_response(&res, buf, n, pos))
				return -1;
			
			pos += n;
			n = read(fd2, buf, _BUF_MAX);
		}
	}

	printf("parse http response %d times success\n", _g_loop_times);
	
	close(fd1);
	close(fd2);

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
		_release();
		return -1;
	}

	if (_g_request)
		_do_request();
	else
		_do_response();

	_release();

	return 0;
}

