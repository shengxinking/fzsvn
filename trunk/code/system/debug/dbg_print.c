#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <ctype.h>
#include <dirent.h>
#include <time.h>

#include "dbg_print.h"

/**
 *	file select filter, skip digital file
 */
static int 
_dbg_file_filter(const struct dirent *entry)
{
	int i, len;

	len = strlen(entry->d_name);

	for (i = 0; i < len; i++) {
		if (!isdigit(entry->d_name[i])) 
			return 0;
	}

	return 1;
}


static int 
_dbg_pts_enable(const char *filename)
{
	struct stat st;
	int ret;

	ret = stat(filename, &st);
	if (ret) 
		return 0;

	if (st.st_mode & S_IXOTH) 
		return 1;

	return 0;
}


static void 
_dbg_print(const char *fmt, va_list args, int console, int pts)
{
	int olderrno = errno;
	int fd;
	struct dirent **namelist;
	int n,i;
	char dev[60]={0};

	if (console) {
		fd = open("/dev/console", O_WRONLY | O_NOCTTY | O_NDELAY);
		vdprintf(fd, fmt, args);
		close(fd);
	}

	if (!pts) {
		errno = olderrno;
		return;
	}

	n = scandir("/dev/pts", &namelist, _dbg_file_filter, NULL );
	for(i = 0; i < n; i++) {
		sprintf(dev,"/dev/pts/%s", namelist[i]->d_name);
		if (!_dbg_pts_enable(dev)) 
			continue;

		if ((fd = open(dev, O_WRONLY | O_NOCTTY | O_NDELAY)) >= 0) {
			vdprintf(fd, fmt, args);
			close(fd);
		}
	}

	if (n > 0 && namelist) {
		while (n--) 
			free(namelist[n]);
		free(namelist);
	}

	errno = olderrno;
}


void 
dbg_print(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	_dbg_print(fmt, args, 1, 1);
	
	va_end(args);
}


void 
dbg_print_console(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	_dbg_print(fmt, args, 1, 0);
	
	va_end(args);
}


void 
dbg_print_pts(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	_dbg_print(fmt, args, 0, 1);
	
	va_end(args);
}



void 
dbg_enable_pts(int num)
{
  

}
