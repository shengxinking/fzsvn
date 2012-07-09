/**
 *	pv_mutex.c:	a simple produce-consume program, it only use pthread 
 *			mutex. It is a PV problem.
 *
 *			problem descript:
 *			There have one producer, one consume, a dish to 
 *			contain N cookies.
 *			1. at first, the dish is empty, no cookies.
 *			2. the producer produce a cookie one time, only when 
 *			   the dish is not full.
 *			3. the consumer consume a cookie one time, only when 
 *			   the dish is not empty.
 *			4. producer and consume can't acquire the dish at 
 *			   same time, if one get dish the other must wait.
 */

#include <pthread.h>
#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

static unsigned int g_cookie_cnt = 0;
static unsigned int g_cookie_max = 5;
static unsigned int g_consumer_cnt = 1;
static pthread_mutex_t g_cookie_lock = PTHREAD_MUTEX_INITIALIZER;

static void _usage()
{
	printf("pv_mutex [options]\n");
	printf("\t-n\tmax cookie number\n");
	printf("\t-c\tconsumer number\n");
}

static int _parse_cmd(int argc, char **argv)
{
	char c;
	char optstr[] = "n:c:";
	int cnt = 0, max = 0;	

	opterr = 0;
	while ( (c = getopt(argc, argv, optstr)) >= 0) {
		switch (c) {
		
		case 'n':
			max = atoi(optarg);
			if (max <= 0)
				return -1;

			g_cookie_max = max;
			break;

		case 'c':
			cnt = atoi(optarg);
			if (cnt <= 0)
				return -1;

			g_consumer_cnt = cnt;
			break;
		case ':':
			printf("option %c need parameter\n", optopt);
			return -1;			

		case '?':
			printf("unkowned option %c\n", optopt);
			return -1;
		}
	}

	if (optind != argc) {
		return -1;
	}
	

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
	sigsetempty(&act.sa_mask);
	sa.sa_flags = 0;
	sa.sa_restore = NULL;

	sigaction(SIGINT, &act, NULL);

	return 0;
}

static void *_consumer_run(void *args)
{
	return NULL;
}

static void *_producer_run(void *args)
{
	return NULL;
}

int main(int argc, char **argv)
{
	pthread_t ptid;
	pthread_t *ctids;
	int i;

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	_init();

	ctids = malloc(g_consumer_cnt * sizeof(pthread_t));
	if (!ctids) {
		printf("can't alloc memory for consumer thread id\n");
		return -1;
	}

	/* create producer */
	if (pthread_create(&ptid, NULL, _producer_run, NULL)) {
		printf("can't create thread for producer\n");
		return -1;
	}
	printf("create producer %lu\n", producer_tid);

	for (i = 0; i < g_consumer_cnt; i++) {
		if (pthread_create(&ctids[i], NULL, _consumer_run, NULL)) {
			printf("can't create thread for consumer %d\n", i);
			return -1;
		}
		printf("create consumer %lu\n", ctids[i]);
	}

	while(1) {
		sleep(1);
	}

	

	return 0;
}

