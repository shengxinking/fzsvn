/*
 * 	at_exit.c:	test at_exit can use in thread
 *
 * 	author:		forrest.zhang
 */

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static void _thread_exit(void)
{
	printf("pid %d, tid %lu exit\n", getpid(), pthread_self());
}

void * _thread_main(void* data)
{
	int ret = 0;
	atexit(_thread_exit);
	pthread_detach(pthread_self());
	printf("child pid %d, tid %lu begin...\n", getpid(), pthread_self());
	sleep(4);
	printf("child pid %d, tid %lu end...\n", getpid(), pthread_self());
	pthread_exit(&ret);
}

int main(void)
{
	pthread_t tid;
	int remain = 0;
	void *retval;

	printf("father pid %d, tid %lu begin...\n", getpid(), pthread_self());

	if (pthread_create(&tid, NULL, _thread_main, NULL)) {
		printf("create thread error\n");
		return -1;
	}

	atexit(_thread_exit);

	remain = sleep(1);
	if (remain > 0)
		printf("sleep is interrupt: %s\n", strerror(errno));
	
	printf("father pid %d, tid %lu end...\n", getpid(), pthread_self());

	return 0;
}

