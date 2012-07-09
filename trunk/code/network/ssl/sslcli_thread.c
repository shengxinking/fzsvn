/*
 *	ssl_client.c:	a simple SSL client
 *
 *	author:		forrest.zhang
 */

#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/bio.h>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <pthread.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "sslsock.h"

#define PKGLEN		20480

typedef struct thread {
	pthread_t	tid;
	int		nconn;
	long long	nsend;
	long long       nrecv;
	int		begin;
	int		end;
} thread_t;

static sslsock_ctx_t	*_g_sslctx;
static u_int16_t	_g_port = 8433;
static u_int32_t	_g_ip = 0; 
static thread_t 	*_g_threads = NULL;
static int		_g_nthread = 1;
static char		_g_senddata[PKGLEN];
static int		_g_sendsize = 1024;
static int		_g_stop = 0;

static void 
_usage(void)
{
	printf("ssl_client [options]\n");
	printf("-i\tserver IP address\n");
	printf("-p\tserver port\n");
	printf("-s\tsend data size\n");
	printf("-n\tthread number\n");
	printf("-h\tshow help info\n");
}

static int 
_parse_cmd(int argc, char **argv)
{
	char c;
	const char optstr[] = ":i:p:s:n:h";
	int port;
	int size;
	int num;

	opterr = 0;
	while ( (c = getopt(argc, argv, optstr)) != -1) {
		
		switch (c) {

		case 'a':
			_g_ip = inet_addr(optarg);
			break;

		case 'p':
			port = atoi(optarg);
			if (port <= 0) {
				printf("-p <1-65535>\n");
				return -1;
			}
			_g_port = port;
			break;

		case 'n':
			num = atoi(optarg);
			if (num <= 0) {
				printf("-n <positive integer>\n");
				return -1;
			}
			_g_nthread = num;
			break;
		
		case 's':
			size = atoi(optarg);
			if (size <= 0 || size > 1024) {
				printf("-s <1 - 1024>\n");
				return -1;
			}
			_g_sendsize = size;
			break;

		case 'h':
			_usage();
			exit(0);
		
		case ':':
			printf("%c miss argument\n", optopt);
			return -1;
		
		case '?':
			printf("unknow option %c\n", optopt);
			return -1;
		}

	}

	if (optind != argc)
		return -1;

	if (_g_ip == 0)
		return -1;

	return 0;
}

static void 
_sig_int(int signo) 
{
	if (signo == SIGINT) {
		printf("user stopped!\n");
		_g_stop = 1;
	}
}

static int 
_initiate(void)
{
	sigset_t mask;
	struct sigaction act;
	int i;

	_g_sslctx = ss_ctx_alloc(SS_CLIENT, SS_VERIFY_NONE);
	if (!_g_sslctx)
		return -1;

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	if (sigprocmask(SIG_BLOCK, &mask, NULL)) {
		printf("sigprocmask error: %s\n", strerror(errno));
		return -1;
	}

	memset(&act, 0, sizeof(act));
	act.sa_handler = _sig_int;
	act.sa_flags = 0;
	act.sa_restorer = NULL;
	sigemptyset(&act.sa_mask);
	if (sigaction(SIGINT, &act, NULL)) {
		printf("sigaction error: %s\n", strerror(errno));
		return -1;
	}

	for (i = 0; i < PKGLEN; i++) 
		_g_senddata[i] = 'a' + rand() % 26;

	_g_threads = malloc(_g_nthread * sizeof(thread_t));
	if (!_g_threads) {
		printf("malloc error: %s\n", strerror(errno));
		return -1;
	}

	memset(_g_threads, 0, _g_nthread * sizeof(thread_t));

	return 0;
}


static void 
_release(void)
{
	if (_g_sslctx)
		ss_ctx_free(_g_sslctx);
	_g_sslctx = NULL;
}


static int 
_do_request(thread_t *info)
{
	int n, m;
	char buf[PKGLEN];
	sslsock_t *ss = NULL;
	int closed = 0;

	if (!info)
		return -1;
	
	ss = ss_client(_g_sslctx, _g_ip, _g_port);

	if ( (n = ss_send(ss, _g_senddata, _g_sendsize)) <= 0) {
		printf("write data to SSL error\n");
		ss_free(ss);
		return -1;
	}

	info->nsend += n;

	m = ss_recv(ss, buf, n, &closed);
	if ( m < 0) {
		printf("recv data from SSL error\n");
		ss_free(ss);
		return -1;
	}
	info->nrecv += m;
	
	ss_free(ss);
	
	return 0;
}


static void *_thread_run(void *args)
{
	thread_t *info;

	info = (thread_t*)args;

	pthread_detach(info->tid);

	info -> begin = time(NULL);
	while (!_g_stop) {
		if (_do_request(info))
			break;
	}

	info->end = time(NULL);

	pthread_exit(0);
}


static void 
_thread_print(void)
{
	int i;
	int intval;
	thread_t *info;
	int nconns = 0;
	int nsecs = 0;
	long long nsends = 0;
	long long nrecvs = 0;

	for (i = 0; i < _g_nthread; i++) {
		info = &_g_threads[i];
		intval = info->end - info->begin;
		nconns += info->nconn;
		nsecs += intval;
		nsends += info->nsend;
		nrecvs += info->nrecv;

		printf("thread %lu info:\n", info->tid);
		printf("spend %d seconds\n", intval);
		printf("%d\tconnections\n",  info->nconn);
		printf("%lld\tbytes send\n", info->nsend);
		printf("%lld\tbytes recv\n", info->nrecv);
	}

	printf("total info:\n");
	printf("spend %d seconds\n", nsecs / _g_nthread);
	printf("%d connections\n",   nconns);
	printf("%llu bytes send\n",  nsends);
	printf("%llu bytes recv\n",  nrecvs);
}


int 
main(int argc, char **argv)
{
	int i;
	sigset_t mask;

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}	

	if (_initiate())
		return -1;

	for (i = 0; i < _g_nthread; i++) {
		if (pthread_create(&_g_threads[i].tid, NULL, _thread_run, &_g_threads[i])) 
		{
			printf("pthread_create error: %s\n", strerror(errno));
			break;
		}

		printf("thread %lu created!\n", _g_threads[i].tid);
	}

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	if (sigprocmask(SIG_UNBLOCK, &mask, NULL)) {
		printf("sigprocmask error: %s\n", strerror(errno));
		return -1;
	}

	_thread_print();

	pause();

	sleep(1);

	_release();

	return 0;
}











