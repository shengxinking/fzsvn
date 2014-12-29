/*
 *	@file	format.c
 *
 *	@brief	format disk APIs
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2008-10-07
 */

/**
 *	Create a ext3 filesystem on disk @disk.
 *
 *	Return 1 if success, -1 on error.
 */
int mkext3fs(const char *disk)
{
	char *args[9];
	int st = 1;
	pid_t pid;
	static char *envp[3];

	if (!disk)
		return -1;

	args[0]= "/bin/mke2fs";
	args[1]= "-j";		/* create journel, it's ext3 */
	args[2]= "-F";
	args[3] = "-O";
	args[4] = "dir_index";
	args[5] = "-b";
	args[6] = "4096";    
	args[7]= (char *)disk;
	args[8]= NULL;

	envp[0] = "TERM=vt100";
	envp[1] = "PATH=/bin:/sbin";
	envp[2] = NULL;
	pid = 0;

	printf("\nFormat log disk %s ...\n", disk);

	/* run mke2fs to format disk */
	if (disk_valid(disk)) {
		if ( (pid=fork()) ==0 ) {
			execve("/bin/mke2fs", args, envp);
			exit(-1);
		}		
		wait(&st);
	}

	if (st == 0) {
		message("Format log disk successful!\n");
	}
	else  {
		message("Format log disk failed! status=%d\n",st);
		return -1;
	}

	fflush(NULL);
	return 0;	
}





