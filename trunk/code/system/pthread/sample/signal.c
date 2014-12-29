/*
 *	signal.c:	test signal behave for linux thread
 *
 * 	author:		forrest.zhang
 */

#include <pthread.h>
#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#define THREAD_NUM	5

static void _usage()
{
	printf("signal\n");
}

static int _parse_cmd(int argc, char **argv)
{
	if (argc > 1)
		return -1;
	return 0;
}

static void _sig_int(int signo)
{
	printf("%lu receive a SIGINT\n", pthread_self());
}

static int _init(void)
{
	struct sigaction act;
	
	act.sa_handler = _sig_int;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_restorer = NULL;

	sigaction(SIGINT, &act, NULL);

	return 0;
}

static int _thread_block_sig(int signo)
{
	sigset_t mask, old;
	
	sigemptyset(&mask);
	sigaddset(&mask, signo);	

	if (pthread_sigmask(SIG_BLOCK, &mask, &old)) {
		printf("can't block signal %d\n", signo);
		return -1;
	}

	return 0;
}

static void *_thread_run(void *arg)
{
	int i;

	for (i = 0; i < 10; i++)
		sleep(1);
	return 0;
}

int main(int argc, char **argv)
{
	int i;
	pthread_t tids[THREAD_NUM];

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	_init();

	printf("father %lu begin\n", pthread_self());

	for (i = 0; i < THREAD_NUM; i++) {
		if (pthread_create(&tids[i], NULL, _thread_run, NULL)) {
			printf("create thread error\n");
			return -1;
		}
		printf("create thread %lu\n", tids[i]);
	}

	_thread_block_sig(SIGINT);
	
	sleep(10);

	return 0;
}





