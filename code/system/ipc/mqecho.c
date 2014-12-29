/*
 *	@file	mqecho.c
 *
 *	@brief	a POSIX message queue echo server. it recv data, and reply it.
 *	
 *	@date	2008-07-09
 */


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <mqueue.h>
#include <getopt.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>

#define _PATH_MAX	1024
#define _MQ_QNUM	10
#define _MQ_MAX		4096

static char _g_mqpath[_PATH_MAX];
static mqd_t _g_mqd = -1;
static int _g_times = 1;
static int _g_mqsize = 64;

static void 
_usage(void)
{
	printf("mqecho <options>\n");
	printf("\t-p\tthe path of message queue\n");
	printf("\t-t\tthe loop times\n");
	printf("\t-s\tthe message size\n");
	printf("\t-h\tshow help message\n");
}

static int 
_parse_cmd(int argc, char **argv)
{
	char opt;
	char optstr[] = ":p:t:s:h";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {
			
		case 'p':
			strncpy(_g_mqpath, optarg, _PATH_MAX - 1);
			break;
			
		case 'h':
			return -1;

		case 't':
			_g_times = atoi(optarg);
			if (_g_times < 1)
				return -1;
			break;

		case 's':
			_g_mqsize = atoi(optarg);
			if (_g_mqsize < 1 || _g_mqsize > _MQ_MAX)
				return -1;
			break;

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

	if (strlen(_g_mqpath) < 1)
		return -1;

	return 0;
}

static int 
_initiate(void)
{
	struct mq_attr attr;

	attr.mq_flags = 0;
	attr.mq_maxmsg = _MQ_QNUM;
	attr.mq_msgsize = _MQ_MAX;
	attr.mq_curmsgs = 0;
	_g_mqd = mq_open(_g_mqpath, O_RDWR | O_CREAT | O_EXCL, 0777, &attr);
	if (_g_mqd == (mqd_t) -1) {
		if (errno == EEXIST) {
			_g_mqd = mq_open(_g_mqpath, O_RDWR);
			if (_g_mqd == (mqd_t) -1) {
				printf("open mqueue %s error: %s\n",
				       _g_mqpath, strerror(errno));
				return -1;
			}
			return 0;
		}
		printf("create mqueue %s error: %s\n",
		       _g_mqpath, strerror(errno));
		return -1;
	}

	

	return 0;
}


static void 
_release(void)
{
	if (_g_mqd > -1) {
		if (mq_unlink(_g_mqpath)) {
			printf("unlink mqueue %s error: %s\n",
			       _g_mqpath, strerror(errno));
		}
	}
}


static int
_do_loop(void)
{
	long long nrecvs = 0;
	long long nsends = 0;
	int m, n;
	char buf[_MQ_MAX];
	unsigned int prio;
	struct timeval tv1, tv2;
	
	gettimeofday(&tv1, NULL);
	while (_g_times) {
		prio = 2;
		memset(buf, 'A', _g_mqsize);
		n = mq_send(_g_mqd, buf, _g_mqsize, prio);
		if (n < 0) {
			printf("mq_send error: %s\n", 
			       strerror(errno));
		}
		nsends += _g_mqsize;

		memset(buf, 0, _MQ_MAX);
		m = mq_receive(_g_mqd, buf, _MQ_MAX, NULL);
		if (m < 0) {
			printf("mq_receive error: %s\n", 
			       strerror(errno));
			sleep(100);
			break;
		}
		nrecvs += m;

		_g_times--;
	}
	gettimeofday(&tv2, NULL);

	printf("sends %lld bytes, recv %lld bytes, spend %ld sec, %ld msec\n",
	       nsends, nrecvs, tv2.tv_sec - tv1.tv_sec, tv2.tv_usec - tv1.tv_usec);

	return 0;
}

int 
main(int argc, char **argv)
{
	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_initiate()) {
		_release();
		return -1;
	}

	_do_loop();

	_release();

	return 0;
}




