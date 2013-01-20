/*
 *	@file	echomsg.c
 *
 *	@brief	An echo server which recv string from message queue, and send
 *		the message back. 
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2008-07-08
 */

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define _KEY_MAX	64
#define _MSG_MAX	4096
#define _QNUM_MAX	64

static key_t _g_msgkey;
static int _g_msgid;

typedef struct msgbuf {
	long	type;
	char	text[0];
} msgbuf_t;

static void
_usage(void)
{
	printf("echomsg <options>\n");
	printf("\t-k\tthe message key\n");
	printf("\t-h\tshow help message\n");
}


static int 
_parse_cmd(int argc, char** argv)
{
	char optstr[] = ":k:h";
	char opt;

	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {

		switch (opt) {

		case 'k':
			_g_msgkey = atoi(optarg);
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
	struct msqid_ds ds;

	_g_msgid = msgget(_g_msgkey, IPC_CREAT | IPC_EXCL | 0777);
	if (_g_msgid < 0) {
		if (errno == EEXIST) {
			_g_msgid = msgget(_g_msgkey, O_RDWR);
			return 0;
		}
		printf("msgget error(IPC_CREATE): %s\n",
		       strerror(errno));
		return -1;
	}
	printf("create msg %d\n", _g_msgid);

	/* set large max number of bytes allowed in queue */
	if (msgctl(_g_msgid, IPC_STAT, &ds)) {
		printf("msgctl(IPC_STAT) error: %s\n",
		       strerror(errno));
		return -1;
	}

	ds.msg_qbytes = _MSG_MAX * _QNUM_MAX;
	if (msgctl(_g_msgid, IPC_STAT, &ds)) {
		printf("msgctl(IPC_SET) error: %s\n",
		       strerror(errno));
		return -1;
	}

	return 0;
}

static void
_release(void)
{
	if (msgctl(_g_msgid, IPC_RMID, NULL)) {
		printf("msgctl error(IPC_RMID): %s\n",
		       strerror(errno));
	}

	printf("release msg %d\n", _g_msgid);
}

static int 
_do_loop(void)
{
	long long nrecvs = 0;
	long long nsends = 0;
	int n, m, i;
	char buf[_MSG_MAX];
	msgbuf_t *msg;
	
	msg = (msgbuf_t *)buf;

	while (1) {
		
		n = msgrcv(_g_msgid, buf, _MSG_MAX, 1, 0);
		if (n < 0) {
			if (errno == EINTR
			    || errno == E2BIG)
				break;
			printf("msgrcv error: %s\n",
			       strerror(errno));

			break;
		}
		printf("recv %s\n", buf);
		
		nrecvs += n;

		for (i = 0; i < n - sizeof(msgbuf_t); i++)
			msg->text[i]++;
		msg->type = 2;

		m = msgsnd(_g_msgid, buf, n, 0);
		if (m < 0) {
			if (errno == EINTR 
			    || errno == EAGAIN)
				break;
			printf("msgsnd error: %s\n",
			       strerror(errno));

			break;
		}
	}

	printf("recv %lld bytes, send %lld bytes\n",
	       nrecvs, nsends);

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







