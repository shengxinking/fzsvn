/**
 *	@file	mem_usage.c
 *
 *	@brief	memory APIs.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2012-07-23
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>

//#include "sysutil.h"

/**
 *	using rdtsc instruction to get CPU frequency. 	
 *
 */
static int64_t 
_rdtsc(void)
{
	u_int32_t i, j;
	
	asm volatile("rdtsc" : "=a"(i), "=d"(j):);
	
	return ((int64_t) j << 32) + (int64_t)i;
}


int 
cpu_freq(void)
{
	int64_t tsc_start, tsc_end;
	struct timeval tv_start, tv_end;
	int intval;
	int freq;
	
	tsc_start = _rdtsc();
	gettimeofday(&tv_start, NULL);
	usleep(100000);
	tsc_end = _rdtsc();
	gettimeofday(&tv_end, NULL);
	intval = 1000000 * (tv_end.tv_sec - tv_start.tv_sec);
	intval += tv_end.tv_usec - tv_start.tv_usec;
	
	freq = (tsc_end - tsc_start)/intval;

	return freq;
}


int 
cpu_number()
{
	int ncpu;
	
	ncpu = sysconf(_SC_NPROCESSORS_CONF);
	
	return ncpu;
}


typedef struct _cpu_stat {
	char		name[16];
	long long	user;
	long long	nice;
	long long	kern;
	long long	idle;
} _cpu_stat_t;


static int 
_cpu_get_stat(_cpu_stat_t *cpus, int ncpu) 
{
	FILE *fp = NULL;
	char line[1024];
	int i;

	memset(cpus, 0, sizeof(_cpu_stat_t) * ncpu);
	fp = fopen("/proc/stat", "r");
	if (fp == NULL)
		return -1;

	for (i = 0; i < ncpu; i++) {
		if (fgets(line, 1023, fp) == NULL) {
			fclose(fp);
			return -1;
		}

		sscanf(line, "%s %lld %lld %lld %lld", 
		       cpus[i].name, &(cpus[i].user), &(cpus[i].nice), 
		       &(cpus[i].kern), &(cpus[i].idle));

//		printf("name %s user %lld nice %lld kern %lld idle %lld\n", 
//		       cpus[i].name, cpus[i].user, cpus[i].nice), 
//		       cpus[i].kern), cpus[i].idle);
	}

	fclose(fp);

	return 0;
}

int 
cpu_total_usage(void)
{
	_cpu_stat_t cpu_begin, cpu_end;
	long long kern, user, nice, idle, total;
	int usage;

	if (_cpu_get_stat(&cpu_begin, 1)) {
		return -1;
	}
	
	usleep(10000);

	if (_cpu_get_stat(&cpu_end, 1)) {
		return -1;
	}

	user = cpu_end.user - cpu_begin.user;
	nice = cpu_end.nice - cpu_begin.nice;
	kern = cpu_end.kern - cpu_begin.kern;
	idle = cpu_end.idle - cpu_begin.idle;

	total = kern + user + nice + idle;
	usage = (user + nice + kern) * 100 / total;

	return usage;
}

int cpu_all_usage(int *cpus, int ncpu)
{
	int n = 0;
	int i;
	int len = 0;
	_cpu_stat_t *cpu_begin, *cpu_end;
	long long user, nice, kern, idle, total;

	memset(cpus, 0, sizeof(int) * ncpu);

	n = cpu_number() + 1;
	if (n < 1)
		return -1;

	n = n > ncpu ? ncpu : n;
	len = n * sizeof(_cpu_stat_t);

	cpu_begin = malloc(len);
	if (!cpu_begin)
		return -1;

	cpu_end = malloc(len);
	if (!cpu_end) {
		free(cpu_begin);
		return -1;
	}

	if (_cpu_get_stat(cpu_begin, n)) {
		free(cpu_begin);
		free(cpu_end);
		return -1;
	}
	usleep(500000);
	if (_cpu_get_stat(cpu_end, n)) {
		free(cpu_begin);
		free(cpu_end);
		return -1;
	}

	for (i = 1; i < n; i++) {
		user = cpu_end[i].user - cpu_begin[i].user;
		nice = cpu_end[i].nice - cpu_begin[i].nice;
		kern = cpu_end[i].kern - cpu_begin[i].kern;
		idle = cpu_end[i].idle - cpu_begin[i].idle;
		total = user + nice + kern + idle;
		if (total < 1)
			total = 1;
		cpus[i] = ((user + nice + kern) * 100) / total;
	}

	free(cpu_begin);
	free(cpu_end);

	return 0;
}






