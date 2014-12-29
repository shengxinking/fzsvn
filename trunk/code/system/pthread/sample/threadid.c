/**
 *
 *
 *
 */

#define	_GNU_SOURCE

#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/syscall.h>


#define error_exit(en, msg) \
	do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

static void _usage(void)
{
	printf("threadid\n");
}

static int _parse_cmd(int argc, char **argv)
{
	if (argc > 1)
		return -1;

	return 0;
}

static int _init(void)
{
	return 0;
}

static int _release(void)
{
	return 0;
}

static void 
thread_print_attribute(pthread_attr_t *attr, const char *prefix)
{
	int s, i;
	size_t v;
	void *stkaddr;
	struct sched_param sp;

	s = pthread_attr_getdetachstate(attr, &i);
	if (s != 0)
		error_exit(s, "pthread_attr_getdetachstate");
	printf("%sDetach state        = %s\n", prefix,
		(i == PTHREAD_CREATE_DETACHED) ? "PTHREAD_CREATE_DETACHED" :
		(i == PTHREAD_CREATE_JOINABLE) ? "PTHREAD_CREATE_JOINABLE" :
		"???");

	s = pthread_attr_getscope(attr, &i);
	if (s != 0)
		error_exit(s, "pthread_attr_getscope");
	printf("%sScope               = %s\n", prefix,
			(i == PTHREAD_SCOPE_SYSTEM)  ? "PTHREAD_SCOPE_SYSTEM" :
			(i == PTHREAD_SCOPE_PROCESS) ? "PTHREAD_SCOPE_PROCESS" :
			"???");

	s = pthread_attr_getinheritsched(attr, &i);
	if (s != 0)
		error_exit(s, "pthread_attr_getinheritsched");
	printf("%sInherit scheduler   = %s\n", prefix,
			(i == PTHREAD_INHERIT_SCHED)  ? "PTHREAD_INHERIT_SCHED" :
			(i == PTHREAD_EXPLICIT_SCHED) ? "PTHREAD_EXPLICIT_SCHED" :
			"???");
	s = pthread_attr_getschedpolicy(attr, &i);
	if (s != 0)
		error_exit(s, "pthread_attr_getschedpolicy");
	printf("%sScheduling policy   = %s\n", prefix,
			(i == SCHED_OTHER) ? "SCHED_OTHER" :
			(i == SCHED_FIFO)  ? "SCHED_FIFO" :
			(i == SCHED_RR)    ? "SCHED_RR" :
			"???");

	s = pthread_attr_getschedparam(attr, &sp);
	if (s != 0)
		error_exit(s, "pthread_attr_getschedparam");
	printf("%sScheduling priority = %d\n", prefix, sp.sched_priority);

	s = pthread_attr_getguardsize(attr, &v);
	if (s != 0)
		error_exit(s, "pthread_attr_getguardsize");
	printf("%sGuard size          = %lu bytes\n", prefix, v);

	s = pthread_attr_getstack(attr, &stkaddr, &v);
	if (s != 0)
		error_exit(s, "pthread_attr_getstack");
	printf("%sStack address       = %p\n", prefix, stkaddr);
	printf("%sStack size          = %lu Kbytes\n", prefix, v/1024);
}


static void *_thread_run(void *arg)
{
	pthread_attr_t attr;

	printf("pthread_self is %lu\n", pthread_self());

	pthread_detach(pthread_self());
	
	if (pthread_getattr_np(pthread_self(), &attr)) {
		printf("get thread attr failed\n");
		return NULL;
	}

	thread_print_attribute(&attr, "\t");

	return NULL;
}

int main(int argc, char **argv)
{
	pthread_t tid;

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_init())
		return -1;

	if (pthread_create(&tid, NULL, _thread_run, NULL)) {
		printf("pthread_create error: %s\n", strerror(errno));
		return -1;
	}
	printf("create thread %lu\n", tid);

	sleep(1);

	if (_release())
		return -1;

	return 0;
}

