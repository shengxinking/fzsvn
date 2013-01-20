/*
 *	@file	msgcli.c
 *
 *	@brief	A simple client of message queue, it send the string to msgecho, 
 *		then recv the reply.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date
 */

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define _KEY_MAX	64
#define _MSG_MAX	4096
#define _QNUM_MAX	64

static key_t _g_msgkey;
static int _g_msgid;
static int _g_msgsize = 64;
static int _g_times = 1;
static char _g_buf[_MSG_MAX];

typedef struct msgbuf {
	long type;
	char text[0];
} msgbuf_t;

static void
_usage(void)
{
	printf("echomsg <options>\n");
	printf("\t-k\tthe message key\n");
	printf("\t-s\tthe message size\n");
	printf("\t-t\tthe loop time\n");
	printf("\t-h\tshow help message\n");
}


static int 
_parse_cmd(int argc, char** argv)
{
	char optstr[] = ":k:t:s:h";
	char opt;

	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {

		switch (opt) {

		case 'k':
			_g_msgkey = atoi(optarg);
			break;

		case 't':
			_g_times = atoi(optarg);
			if (_g_times < 1)
				return -1;
			break;

		case 's':
			_g_msgsize = atoi(optarg);
			if (_g_msgsize < 1 
			    || _g_msgsize > _MSG_MAX - sizeof(msgbuf_t))
				return -1;
			break;

		case 'h':
			return -1;

		case ':':
			printf("option %c missing argument\n", opt);
			return -1;

		case '?':
			printf("unknowed option %c\n", opt);
			return -1;
		}
	}

	if (optind != argc)
		return -1;

	if (_g_msgkey < 1)
		return -1;

	return 0;
}


static int 
_initiate(void)
{
	msgbuf_t *buf;

	_g_msgid = msgget(_g_msgkey, 0);
	if (_g_msgid < 0) {
		printf("msgget error: %s\n",
		       strerror(errno));
		return -1;
	}
	printf("msgid is %d\n", _g_msgid);

	buf = (struct msgbuf *)_g_buf;
	buf->type = 1;
	memset(&(buf->text), 'A', _g_msgsize);
	buf->text[_g_msgsize] = 0;

	return 0;
}

static void
_release(void)
{

}

static int 
_do_loop(void)
{
	long long nrecvs = 0;
	long long nsends = 0;
	int n, m;
	struct timeval tv1, tv2;

	char buf[_MSG_MAX];

	gettimeofday(&tv1, NULL);
	while (_g_times > 0) {

		n = msgsnd(_g_msgid, _g_buf, _g_msgsize + sizeof(msgbuf_t), 0);
		if (n < 0) {
			if (errno == EINTR
			    || errno == E2BIG)
				break;
			printf("msgsnd error: %s\n",
			       strerror(errno));

			break;
		}
		printf("send: %s\n", _g_buf);

		n = msgsnd(_g_msgid, _g_buf, _g_msgsize + sizeof(msgbuf_t), 0);
		if (n < 0) {
			if (errno == EINTR
			    || errno == E2BIG)
				break;
			printf("msgsnd error: %s\n",
			       strerror(errno));

			break;
		}
		printf("send: %s\n", _g_buf);

		nsends += _g_msgsize;

		m = msgrcv(_g_msgid, buf, _MSG_MAX, 2, 0);
		if (m < 0) {
			if (errno == EINTR 
			    || errno == EAGAIN)
				break;
			printf("msgrcv error: %s\n",
			       strerror(errno));

			break;
		}

		nrecvs += _g_msgsize;

		_g_times--;
	}
	gettimeofday(&tv2, NULL);

	printf("send %lld bytes, recv %lld bytes, spend %ld seconds, %ld microseconds\n",
	       nrecvs, nsends, tv2.tv_sec - tv1.tv_sec, tv2.tv_usec - tv1.tv_usec);
	
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




