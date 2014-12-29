/*
 *	exit.c:		test when the main thread exit, other thread will exit or not
 *
 * 	author:		forrest.zhang
 */

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#define THREAD_MAX	5

static void _usage(void)
{
	printf("exit\n");
}

static int _parse_cmd(int argc, char **argv)
{
	if (argc > 1)
		return -1;

	return 0;
}

static void *_thread_run(void *arg)
{
	pthread_detach(pthread_self());
	sleep(5);
	
	printf("%lu exit\n", pthread_self());
	pthread_exit(NULL);
}

int main(int argc, char **argv)
{
	pthread_t tids[THREAD_MAX];
	int i;

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	for (i = 0; i < THREAD_MAX; i++) {
		if (pthread_create(&tids[i], NULL, _thread_run, NULL)) {
			printf("create thread error\n");
			return -1;
		}

		printf("create thread %lu\n", tids[i]);
	}

	pthread_detach(pthread_self());

	printf("%lu exit\n", pthread_self());
	pthread_exit(NULL);
}


