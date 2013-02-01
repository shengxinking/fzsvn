/**
 *	@file	procfile_util.c
 *
 *	@brief	the function get/set information in /proc file.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "sysutil.h"


int 
procfile_get_rss(pid_t pid)
{
	char procfile[PROC_FILE_LEN];
	FILE *fp;
	char buf[1024] = {0};
	int rss = 0;

	if (pid < 1)
		return -1;

	snprintf(procfile, PROC_FILE_LEN, "/proc/%d/status", pid);
	if (access(procfile, F_OK))
		return -1;

	fp = fopen(procfile, "r");
	if (!fp)
		return -1;

	while (fgets(buf, sizeof(buf), fp)) {
		if (strncmp(buf, "VmRSS:", 6) == 0) {
			sscanf(buf, "VmRSS:  %d", &rss);
			break;
		}
	}
	fclose(fp);

	return rss;
}



