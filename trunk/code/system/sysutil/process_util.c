/**
 *	@file	process_util.c
 *
 *	@brief	Linux process functions.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2012-07-23
 */

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <dirent.h>

extern char **environ;

int 
fz_proc_rename(const char *newname, char **argv)
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
fz_proc_rss(void)
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
	char *ptr;

	if (!dir)
		return 0;

	ptr = dir->dname;
	while (*ptr++)
		if (!isdigit(*ptr))
			return 0;

	return 1;
}


pid_t * 
fz_proc_find(const char *name)
{
	int ret = 0;
	pid_t *pids = NULL;
	char **namelist = NULL:
		
	ret = scandir("proc", &namelist, _proc_filter, alphasort);
	if (ret < 0)
		return NULL;

	while (n--) {
		printf("%s\n", namelist[n]->dname);
	}
	
	return pids;
}


int 
fz_proc_exist(pid_t pid)
{
	if (kill(pid, 0))
		return 0;

	return 1;
} 

