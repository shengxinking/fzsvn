/**
 *	@file	process_util.c
 *
 *	@brief	Linux process functions.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2012-07-23
 */

#define	_GNU_SOURCE

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <dirent.h>
#include <signal.h>
#include <sched.h>

#include "cpu_util.h"
#include "process_util.h"

extern char **environ;

int 
process_rename(const char *newname, char **argv)
{
	char *new_env;
	char *argv_last;
	int size = 0;
	int i;

	if (!newname || !argv)
		return -1;

	for (i = 0; environ[i]; i++) {
		size += strlen(environ[i]) + 1;
	}
	
	printf("size is %d\n", size);

	/* alloc memory for new environ */
	new_env = malloc(size);
	if (!new_env) {
		printf("malloc failed\n");
		return -1;
	}
	
	for (i = 0; argv[i]; i++) {
		argv_last = argv[i] + strlen(argv[i]) + 1;
	}

	/* copy environ to new memory, set environ[i] address */
	for (i = 0; environ[i]; i++) {
		if (argv_last == environ[i]) {
			size = strlen(environ[i]) + 1;
			argv_last = environ[i] + size;
			strncpy(new_env, environ[i], size);
			environ[i] = new_env;
			new_env += size;
		}
		else {
			printf("the environ is not after argv\n");
			free(new_env);
			return -1;
		}
	}

	argv_last--;

	/* copy procname to memory argv[0]->argv_last */
	size = argv_last - argv[0];
	argv[1] = NULL;
	memset(argv[0], 0, size);
	
	strncpy(argv[0], newname, size);
	
	return 0;
}

u_int64_t  
process_get_rss(void)
{
	struct rusage ru;
	
	if (getrusage(RUSAGE_SELF, &ru)) {
		return -1;
	}

	return ru.ru_maxrss;
}

static int 
_proc_filter(const struct dirent *dir)
{
	const char *ptr;

	if (!dir)
		return 0;

	ptr = dir->d_name;
	while (*ptr++)
		if (!isdigit(*ptr))
			return 0;

	return 1;
}

pid_t * 
process_find(const char *name)
{
	pid_t *pids = NULL;
	struct dirent **items = NULL;
	int n;
	int i;

	n = scandir("/proc", &items, _proc_filter, alphasort);
	if (n < 0 || items == NULL)
		return NULL;

	pids = malloc(sizeof(pid_t) * (n + 1));
	if (!pids)
		return NULL;

	for (i = 0; i < n; i++)
		pids[i] = atoi(items[n]->d_name);

	pids[n] = 0;

	free(items);
	
	return pids;
}

int 
process_is_exist(pid_t pid)
{
	if (pid < 0)
		return 0;

	if (kill(pid, 0))
		return 0;

	return 1;
} 

int 
process_bind_cpu(pid_t pid, int cpu)
{
	int ncpu;
	cpu_set_t mask;

	if (cpu < 0)
		return -1;

	/* get cpu number */
	ncpu = cpu_get_number();
	if (ncpu < 2)
		return 0;
	
	/* bind CPU */
	cpu = cpu % ncpu;
	CPU_ZERO(&mask);
	CPU_SET(cpu, &mask);
	if (sched_setaffinity(pid, sizeof(mask), &mask))
		return -1;

	return 0;
}



