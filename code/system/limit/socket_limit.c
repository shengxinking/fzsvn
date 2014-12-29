/*
 *	filedes_limit.c:	open a file many times until it error, 
 *				to test how many file can open
 *				in a process
 *
 *	author:			forrest.zhang
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <netinet/in.h>


static void _usage(void)
{
	printf("filedes_limit\n");
}

static int _parse_cmd(int argc, char **argv)
{
	if (argc > 1)
		return -1;
	return 0;
}

static int _init(void)
{
	struct rlimit rlim;

	rlim.rlim_cur = 500000;
	rlim.rlim_max = 500000;

	if (setrlimit(RLIMIT_NOFILE, &rlim)) {
		printf("setrlimit error: %s\n", strerror(errno));
		return -1;
	}
	
	return 0;
}

int main(int argc, char **argv)
{
	int fd;
	int i = 0;

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	_init();

	while (1) {
		fd = socket(AF_INET, SOCK_STREAM, 0);
		if (fd < 0) {
			printf("open error: %s\n", strerror(errno));
			break;
		}
		else {
			printf("the fd is %d\n", fd);
		}
		i++;
	}

	printf("open %d file descriptor in one process, fd is %d\n", i, fd);
	
	sleep(4);

	return 0;
}

