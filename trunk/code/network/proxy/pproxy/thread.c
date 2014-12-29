/*
 *	thread.c:	provide thread manager
 *
 *	author:		forrest.zhang
 */

#define	_GNU_SOURCE

#include <string.h>
#include <errno.h>
#include <sched.h>

#include "thread.h"
#include "debug.h"
#include "cpu_util.h"

/**
 *	Create a new thread, the new thread function is @func, 
 *	the @func's argument is @ti, the @type see above.
 *
 *	Return 0 if create thread success, -1 on error.
 */
int 
thread_create(thread_t *ti, int index, void *(*func)(void *arg))
{
	if (!ti || !func) {
		ERR("thread_create: invalid parameter\n");
		return -1;
	}
	
	ti->index = index;
	if (pthread_create(&ti->tid, NULL, func, ti)) {
		ERR("pthread_create: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}


/**
 *	Wait a thread to exit.
 *
 *	Return 0 if success, -1 on error.
 */
int 
thread_join(thread_t *ti)
{
	if (!ti || !ti->tid) {
		return -1;
	}
	
	if (pthread_join(ti->tid, NULL)) {
		ERR("pthread_join: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}

int 
thread_bind_cpu(pthread_t tid, int index, int algo, int ht)
{
	int ncpu;
	cpu_set_t mask;
	int cpu;
	int start;
	int mod;

	if (index < 0)
		return -1;

	/* get cpu number */
	ncpu = cpu_get_number();
	if (ncpu < 2)
		return 0;

	if (ht == THREAD_HT_FULL) {
		start = 0;
		mod = ncpu;
	}
	else if (ht == THREAD_HT_LOW) {
		start = 0;
		mod = ncpu / 2;
	}
	else if (ht == THREAD_HT_HIGH) {
		start = ncpu / 2;
		mod = ncpu / 2;
	}
	else {
		return -1;
	}
	
	switch(algo) {
	case THREAD_BIND_RR:
		cpu = (index % mod) + start;
		break;
	case THREAD_BIND_ODD:
		cpu = ((index * 2) % mod) + start;
		break;
	case THREAD_BIND_EVEN:
		cpu = (((index * 2) + 1) % mod) + start;
		break;
	default:
		return -1;
	}

	/* bind CPU */
	CPU_ZERO(&mask);
	CPU_SET(cpu, &mask);
	if (pthread_setaffinity_np(tid, sizeof(mask), &mask))
		return -1;

	return cpu;
}



