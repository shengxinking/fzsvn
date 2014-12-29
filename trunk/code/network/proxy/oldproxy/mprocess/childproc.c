/**
 *	@file	childproc.c
 *
 *	
 *
 */

#define _GNU_SOURCE

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/wait.h>
#include <errno.h>
#include <sched.h>

#include "childproc.h"
#include "proxy.h"
#include "cpu_util.h"

int 
cproc_create(cproc_t *cp, proxy_arg_t *arg)
{
	pid_t pid;

	if (!cp || !arg || index < 0)
		return -1;

	pid = fork();
	if (pid < 0) {
		printf("fork failed %s\n", strerror(errno));
		return -1;
	}
	else if (pid == 0) {
		proxy_main(arg);
		exit(0);
	}
	
	cp->pid = pid;
	cp->index = arg->index;

	return 0;
}

int 
cproc_destroy(cproc_t *cp) 
{
	int status;
	int ret;
	pid_t pid;

	if (!cp)
		return -1;

	if (cp->pid < 2)
		return -1;

	ret = kill(cp->pid, SIGUSR1);
	if (ret)
		return -1;

	pid = waitpid(cp->pid, &status, 0);
	if (pid == cp->pid)
		return 0;

	return -1;
}

int 
cproc_bind_cpu(pid_t pid, int index, int algo, int ht)
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

	if (ht == CPROC_HT_FULL) {
		start = 0;
		mod = ncpu;
	}
	else if (ht == CPROC_HT_LOW) {
		start = 0;
		mod = ncpu / 2;
	}
	else if (ht == CPROC_HT_HIGH) {
		start = ncpu / 2;
		mod = ncpu / 2;
	}
	else {
		return -1;
	}

	switch(algo) {
		case CPROC_BIND_RR:
			cpu = (index % mod) + start;
			break;
		case CPROC_BIND_ODD:
			cpu = ((index * 2) % mod) + start;
			break;
		case CPROC_BIND_EVEN:
			cpu = (((index * 2) + 1) % mod) + start;
			break;
		default:
			return -1;
	}

	/* bind CPU */
	CPU_ZERO(&mask);
	CPU_SET(cpu, &mask);
	if (sched_setaffinity(pid, sizeof(mask), &mask))
		return -1;

	return cpu;
}

