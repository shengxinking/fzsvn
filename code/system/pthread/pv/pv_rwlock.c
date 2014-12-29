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

//#define	__USE_UNIX98

#include <pthread.h>
#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

static u_int32_t	g_cookie_cnt = 0;
static u_int32_t	g_cookie_max = 5;
static pthread_rwlock_t	g_cookie_lock;
static u_int32_t	g_consumer_cnt = 1;
static volatile int	g_stop = 0;

#define	PV_ERRSTR	strerror(errno)

#define	PV_RDLOCK()					\
({							\
 	if (pthread_rwlock_rdlock(&g_cookie_lock))		\
 		printf("rdlock failed: %s\n", PV_ERRSTR);	\
})

#define	PV_WRLOCK()					\
({							\
 	if (pthread_rwlock_wrlock(&g_cookie_lock))		\
 		printf("wrlock failed: %s\n", PV_ERRSTR);	\
})

#define	PV_UNLOCK()					\
({							\
 	if (pthread_rwlock_unlock(&g_cookie_lock))		\
 		printf("unlock failed: %s\n", PV_ERRSTR);	\
})

static void 
_usage()
{
	printf("pv_mutex [options]\n");
	printf("\t-n\tmax cookie number\n");
	printf("\t-c\tconsumer number\n");
}

static int 
_parse_cmd(int argc, char **argv)
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

static void 
_sig_int(int signo)
{
	printf("%lu receive a SIGINT\n", pthread_self());
	g_stop = 1;
}

static int 
_initiate(void)
{
	struct sigaction act;
	pthread_rwlockattr_t attr;

	/* set stop signal */
	act.sa_handler = _sig_int;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_restorer = NULL;
	if (sigaction(SIGINT, &act, NULL))
		return -1;

	pthread_rwlockattr_init(&attr);
	//pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_READER_NP);
	//pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NP);
	pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
	pthread_rwlock_init(&g_cookie_lock, &attr);

	return 0;
}

static int 
_release(void)
{
	return 0;
}

static void *
_producer_run(void *args)
{
	sleep(1);
	printf("producer started\n");

	sleep(2);
	PV_RDLOCK();
	printf("producer rdlocked\n");
	sleep(2);

	PV_RDLOCK();
	printf("producer rdlocked\n");
	sleep(2);

	PV_UNLOCK();
	printf("producer unlocked\n");
	sleep(2);

	PV_UNLOCK();
	printf("producer unlocked\n");
	sleep(2);

	printf("producer stopped\n");

	return NULL;
}

static void *
_consumer_run(void *args)
{
	sleep(1);
	printf("consumer started\n");

	sleep(3);
	//PV_WRLOCK();
	//printf("consumer wrlocked\n");
	sleep(2);

	//PV_UNLOCK();
	//printf("consumer unlocked\n");

	//PV_UNLOCK();
	//printf("consumer unlocked\n");

	printf("consumer stopped\n");
	
	return NULL;
}

int 
main(int argc, char **argv)
{
	pthread_t ptid;
	pthread_t *ctids;
	int i;

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_initiate()) {
		return -1;
	}

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
	printf("create producer %lu\n", ptid);

	for (i = 0; i < g_consumer_cnt; i++) {
		if (pthread_create(&ctids[i], NULL, _consumer_run, NULL)) {
			printf("can't create thread for consumer %d\n", i);
			return -1;
		}
		printf("create consumer %lu\n", ctids[i]);
	}

	while(g_stop == 0) {
		sleep(1);
	}

	_release();

	return 0;
}

