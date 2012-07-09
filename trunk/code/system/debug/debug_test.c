/**
 *	@file	
 *
 *	@brief
 *
 *	@author	Forrest.zhang
 *	
 *	@date
 */

#define _GNU_SOURCE

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "debug.h"

static dbg_db_t		*_g_log_db = NULL;
static char		_g_log_file[1024];
static int		_g_child_stop = 0;


/**
 *	Show help message	
 *	
 *	No return.
 */
static void 
_usage(void)
{
	printf("debug_test <options>\n");
	printf("\t-f\tthe log file\n");
	printf("\t-h\tshow help message\n");
}

/**
 *	Parse command line argument.	
 *
 * 	Return 0 if parse success, -1 on error.
 */
static int 
_parse_cmd(int argc, char **argv)
{
	char opt;
	char optstr[] = ":f:h";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {
			
		case 'f':
			strncpy(_g_log_file, optarg, 1023);
			break;

		case 'h':
			return -1;

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

	return 0;
}

static void 
_sig_child(int signo)
{
	pid_t pid = 0;

	pid = waitpid(-1, NULL, 0);
	if (pid > 0) {
		printf("child %d stopped\n", pid);
	}

	_g_child_stop = 1;
}


/**
 *	Init some global resource used in program.	
 *
 * 	Return 0 if success, -1 on error.
 */
static int 
_initiate(void)
{
	signal(SIGCHLD, _sig_child);

	_g_log_db = dbg_alloc_db(10);
	if (!_g_log_db)
		return -1;

	return 0;
}


/**
 *	Release global resource alloced by _initiate().	
 *
 * 	No Return.
 */
static void 
_release(void)
{
	if (strlen(_g_log_file) > 0)
		dbg_save_db(_g_log_db, _g_log_file);

	if (_g_log_db)
		dbg_free_db(_g_log_db);
}


static int 
_child_run(int fd) 
{
	int i;
	if (fd < 0)
		return -1;

	for (i = 0; i < 80; i++) {
		dbg_log_fd(fd, "%d: start-process\n", i);
		usleep(1000);
		dbg_log_fd(fd, "%d, the step 1\n", i);
		usleep(1000);
		dbg_log_fd(fd, "%d, the step 2\n", i);
		usleep(1000);
		dbg_log_fd(fd, "%d, end-process\n", i);
	}

	return 0;
}



static int 
_do_process()
{
	pid_t pid;
	int fd2[2];
	int fd;

	if (pipe2(fd2, O_NONBLOCK)) {
		printf("pipe failed\n");
		return -1;
	}	

	pid = fork();
	if (pid < 0) {
		printf("fork failed\n");
		return -1;
	}
	else if (pid == 0) {
		fd = fd2[1];
		_child_run(fd);
		exit(0);
	}

	fd = fd2[0];

	while (!_g_child_stop) {		
		dbg_log_recv(_g_log_db, fd);		
	}

	return 0;
}


/**
 *	The main entry of program.	
 *
 * 	Return 0 if success, other value on error.
 */
int 
main(int argc, char **argv)
{
	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_initiate()) {
		return -1;
	}

	_do_process();
	
	_release();

	return 0;
}



