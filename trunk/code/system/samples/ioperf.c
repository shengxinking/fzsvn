/*
 *	@file	read_perf
 *
 *	@brief	see linux read performance using /dev/null device
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/resource.h>
#include <getopt.h>

#define BUF_MAX		64

static long long _g_count = 100000000;


static void _usage(void)
{
	printf("ioperf: test the read/write times in system\n");
	printf("\t-h\tshow help message\n");
}

static int _parse_cmd(int argc, char **argv)
{
	char optstr[] = ":h";
	char opt;

	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
	}

	if (optind != argc)
		return -1;
	return 0;
}

static int _init(void)
{
	return 0;
}

int main(int argc, char **argv)
{
	int rfd, wfd;
	char buf[BUF_MAX];
	int i = 0;
	int n;
	struct timeval begin, end;
	int msec, sec;

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	_init();

	rfd = open("/dev/zero", O_RDONLY);
	if (rfd < 0) {
		printf("open /dev/zero error: %s\n", strerror(errno));
		return -1;
	}
	wfd = open("/dev/null", O_WRONLY);
	if (wfd < 0) {
		printf("open /dev/zero error: %s\n", strerror(errno));
		return -1;
	}

	gettimeofday(&begin, NULL);
	while (_g_count > 0) {
		n = read(rfd, buf, BUF_MAX);
		if (n != BUF_MAX) {
			printf("read error: %s\n", strerror(errno));
			continue;
		}
		n = write(wfd, buf, BUF_MAX);
		if (n != BUF_MAX) {
			printf("read error: %s\n", strerror(errno));
			continue;
		}		
		_g_count--;
		i++;
	}
	gettimeofday(&end, NULL);

	if (end.tv_usec < begin.tv_usec) {
		msec = 1000000 + end.tv_usec - begin.tv_usec;
		end.tv_sec -= 1;
	}
	else {
		msec = end.tv_usec - begin.tv_usec;
	}
	sec = end.tv_sec - begin.tv_sec;
	
	printf("loop %d times using %d second, %d microsend\n", i, sec, msec);

	return 0;
}

