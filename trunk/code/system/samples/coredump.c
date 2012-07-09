/*
 *	@file	
 *
 *	@brief
 *	
 *	@date
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <getopt.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>

static void _usage(void)
{
	printf("coredump: generate coredump in linux\n");
}

static int _parse_cmd(int argc, char **argv)
{
	char opt;
	char optstr[] = "h";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {
			
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

int _initiate(void)
{
	
	return 0;
}


int _release(void)
{
	return 0;
}


int _process(void)
{
	struct rlimit rlim;
	void *ptr = NULL;

	memset(&rlim, 0, sizeof(rlim));
	if (getrlimit(RLIMIT_CORE, &rlim)) {
		printf("getrlimit error: %s\n", strerror(errno));
		return -1;
	}
	
	printf("RLIMIT_CORE: cur %lu, max %lu\n", rlim.rlim_cur, rlim.rlim_max);

	rlim.rlim_cur = rlim.rlim_max;
	
	if (setrlimit(RLIMIT_CORE, &rlim)) {
		printf("setrlimit error: %s\n", strerror(errno));
		return -1;
	}

	memset(&rlim, 0, sizeof(rlim));
	if (getrlimit(RLIMIT_CORE, &rlim)) {
		printf("getrlimit error: %s\n", strerror(errno));
		return -1;
	}

	printf("new RLIMIT_CORE: cur %lu, max %lu\n", rlim.rlim_cur, rlim.rlim_max);

	*(char *)ptr = 0;

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

	_process();

	_release();

	return 0;
}




