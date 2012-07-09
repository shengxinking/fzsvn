/*
 *
 *
 *
 */

#include <sys/types.h>
#include <linux/unistd.h>
#include <errno.h>
_syscall0(pid_t, gettid);
pid_t gettid(void);
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

static void _usage(void)
{
	printf("threadid\n");
}

static int _parse_cmd(int argc, char **argv)
{
	if (argc > 1)
		return -1;

	return 0;
}

static int _init(void)
{
	return 0;
}

static int _release(void)
{
	return 0;
}

static void *_thread_run(void *arg)
{
	printf("gettid is %u\n", gettid());
	printf("pthread_self is %lu\n", pthread_self());

	return NULL;
}

int main(int argc, char **argv)
{
	pthread_t tid;
	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_init())
		return -1;

	if (pthread_create(&tid, NULL, _thread_run, NULL)) {
		printf("pthread_create error: %s\n", strerror(errno));
		return -1;
	}
	printf("create thread %lu\n", tid);

	if (_release())
		return -1;

	return 0;
}

