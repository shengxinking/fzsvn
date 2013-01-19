/**
 *	@file	childproc.c
 *
 *	
 *
 */

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/wait.h>
#include <errno.h>

#include "childproc.h"
#include "proxy.h"

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
