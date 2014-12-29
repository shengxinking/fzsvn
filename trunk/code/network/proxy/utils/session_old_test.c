/**
 *	@file	sesspool_test.c
 *
 *	@brief	A test program to test session pool in sesspool.c
 *	
 *	@date	2010-07-05
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <getopt.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>

#include "session.h"

#define MAX_THREADS		100

#define	OP_MODE_INLINE		0
#define	OP_MODE_OFFLINE		1

static int		_g_thread_num = 1;
static int		_g_opmode = OP_MODE_INLINE;
static int		_g_loop_times = 1;
static int		_g_capacity = 5;
static session_table_t	*_g_sesstbl = NULL;
static pthread_t	_g_threads[MAX_THREADS];
static int		_g_index[MAX_THREADS];

static void 
_usage(void)
{
	printf("sesspool_test <options>\n");
	printf("\t-n\tnumber of thread 1-%d\n", MAX_THREADS);
	printf("\t-l\tloop times\n");
	printf("\t-s\tsession pool size\n");
	printf("\t-m\tthe operation mode: inline/offline\n");
	printf("\t-h\thelp message\n");
}

static int 
_parse_cmd(int argc, char **argv)
{
	char opt;
	char optstr[] = ":n:l:s:m:h";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {
			
		case 'n':
			_g_thread_num = atoi(optarg);
			if (_g_thread_num < 1 || _g_thread_num > MAX_THREADS)
				return -1;
			break;

		case 'l':
			_g_loop_times = atoi(optarg);
			if (_g_loop_times < 1)
				return -1;
			break;

		case 's':
			_g_capacity = atoi(optarg);
			if (_g_capacity < 1)
				return -1;
			break;
			
		case 'm':
			if (strcmp(optarg, "inline") == 0)
				_g_opmode = OP_MODE_INLINE;
			else if (strcmp(optarg, "offline") == 0)
				_g_opmode = OP_MODE_OFFLINE;
			else
				return -1;
			break;

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

static int 
_initiate(void)
{
	_g_sesstbl = session_table_alloc(_g_capacity);
	if (!_g_sesstbl)
		return -1;

	srand(time(NULL));

	return 0;
}


static void
_release(void)
{
	if (_g_sesstbl) {
//		sesspool_print(_g_sesspool);
		printf("free session table\n");
		session_table_free(_g_sesstbl);
	}
}


static int 
_inline_test(int index)
{
	int clifd = -1, svrfd = -1;
	session_t *s = NULL;
//	u_int32_t id = -1;
	int id = -1;
	int ret = -1;

	s = malloc(sizeof(session_t));
	if (!s) {
		printf("malloc memory for session failed\n");
		goto out_free;
	}

	/* simulate (recv/parse first packet) */
	clifd = socket(AF_INET, SOCK_STREAM, 0);
	if (clifd < 0) {
		printf("socket failed\n");
		goto out_free;
	}
	s->clifd = clifd;

//	session_table_print(_g_sesstbl);

	if (session_table_add(_g_sesstbl, s)) {
		printf("[%d]thread(%d) session table add failed\n", __LINE__, index);
		goto out_free;
	}
	id = s->id;
	printf("thread %d id is %d\n", index, id);

	usleep(10);

//	session_table_print(_g_sesstbl);

	/* simulate (recv/parse second packet) */
	s = session_table_find(_g_sesstbl, id);
	if (!s) {
		printf("%d: Not found session id %d\n", __LINE__, id);
		goto out_free;
	}
	
	usleep(10);
	
	/* simulate (send packet to server) */
	svrfd = socket(AF_INET, SOCK_STREAM, 0);
	if (svrfd < 0) {
		printf("socket error\n");
		goto out_free;
	}

	s = NULL;
	s = session_table_find(_g_sesstbl, id);
	if (!s) {
		printf("%d: Not found id %d\n", __LINE__, id);
		goto out_free;
	}
	s->svrfd = svrfd;
	
	usleep(10);

	/* simulate (parse server packet) */
	s = session_table_find(_g_sesstbl, id);
	if (!s) {
		printf("%d: Not found id %d\n", __LINE__, id);
		goto out_free;
	}
	usleep(10);

	s = NULL;
	
	/* add a new session */
	s = session_table_find(_g_sesstbl, id);
	if (!s)	{
		printf("Not found fd %d\n", clifd);
		goto out_free;	
	}

	ret = 0;

out_free:

	if (id >= 0) {
		session_table_del(_g_sesstbl, id);
	}

//	session_table_print(_g_sesstbl);

	if (s)
		free(s);

	if (clifd > 0)
		close(clifd);

	if (svrfd > 0)
		close(svrfd);
	
	return ret;
}


static int 
_offline_test()
{
	int id = -1;
	u_int32_t sip;
	u_int32_t dip;
	u_int16_t sport;
	u_int16_t dport;
	session_t *s = NULL;
	
	sip = rand();
	dip = rand();
	sport = rand();
	dport = 80;

	/* create new session */
	s = malloc(sizeof(session_t));
	if (!s) {
		printf("create new session failed\n");
		goto out_free;
	}
	s->id = id;
	IP_PORT_SET_V4(&s->cliaddr, sip, htons(sport));
	(&s->cliaddr)->family = AF_INET;
	IP_PORT_SET_V4(&s->svraddr, dip, htons(dport));

	if (session_table_add(_g_sesstbl, s)) {
		printf("add session %d failed\n", id);
		goto out_free;
	}

	/* parse data */
	s = session_table_find(_g_sesstbl, id);
	if (!s) {
		printf("find session failed\n");
		goto out_free;
	}
	
	usleep(10);

	/* find session */
	s = session_table_find(_g_sesstbl, id);
	if (!s) {
		printf("find session failed\n");
		return -1;
	}
	
	usleep(10);	

out_free:


	if (id >= 0) {
		session_table_del(_g_sesstbl, id);
	}

	if (s) {
		free(s);
	}

	return 0;
}

void *
_thread_run(void *arg)
{
	int index;
	int i;
	int ret = 0;
	int failed = 0;
	int success = 0;

	index = *(int *)arg;

	printf("thread %d started\n", index);

	usleep(10);

	for (i = 0; i < _g_loop_times; i++) {

		if (_g_opmode == OP_MODE_INLINE) {
			ret = _inline_test(index);
			if (ret)
				failed++;
			else
				success++;
		}
		else {
			ret = _offline_test();
			if (ret)
				failed++;
			else
				success++;
		}
	}

	printf("thread %d end, try %d times, success %d, failed %d\n",
	       index, _g_loop_times, success, failed);

	return NULL;
}

int 
main(int argc, char **argv)
{
	int i = 0;

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_g_opmode == OP_MODE_INLINE)
		printf("run inline test\n");
	else
		printf("run offline test\n");

	if (_initiate()) {
		return -1;
	}

	if (_g_thread_num > 1) {
		for (i = 0; i < _g_thread_num; i++) {
			_g_index[i] = i;
			if (pthread_create(&_g_threads[i], NULL, 
					   _thread_run, &_g_index[i])) 
			{
				printf("pthread_create error\n");
				break;
			}
		}
	}
	else {
		_thread_run(&i);
	}

	if (_g_thread_num > 1) {
		for (i = 0; i < _g_thread_num; i++) {
			if (pthread_join(_g_threads[i], NULL)) {
				printf("pthread_join error\n");
				break;
			}
		}
	}

	_release();

	return 0;
}





