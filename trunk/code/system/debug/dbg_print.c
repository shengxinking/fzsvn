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


static void 
_dbg_output(const char *dev, const char *str)
{
	int fd;
	
	if (!dev)
		return;

	fd = open(dev, O_WRONLY | O_NOCTTY | O_NDELAY);
	if (fd >= 0) {
		dprintf(fd, "%s", str);
		close(fd);
	}
}


static void 
_dbg_pts(const char *str)
{
	struct dirent **namelist;
	int n,i;
	char dev[60]={0};

	n = scandir("/dev/pts", &namelist, _dbg_file_filter, NULL );
	for(i = 0; i < n; i++) {
		sprintf(dev,"/dev/pts/%s", namelist[i]->d_name);
		if (!dbg_pts_is_enabled(dev)) 
			continue;
		
		_dbg_output(dev, str);
	}

	if (n > 0 && namelist) {
		while (n--) 
			free(namelist[n]);
		free(namelist);
	}
}


void 
dbg_print(const char *fmt, ...)
{
	va_list ap;
	char buf[DBG_LINE_LEN];

	va_start(ap, fmt);
	vsnprintf(buf, DBG_LINE_LEN - 1, fmt, ap);
	va_end(ap);

	if (dbg_console_is_enabled())
		_dbg_output("/dev/console", buf);

	_dbg_pts(buf);
}


int 
dbg_enable_pts(const char *pts)
{
	struct stat st;
	int ret;
	int mode;

	if (!pts)
		return -1;

	ret = stat(pts, &st);
	if (ret) 
		return -1;

	mode = st.st_mode;

	if (mode & S_IXOTH)
		return 0;

	mode = mode | S_IXOTH;
	
	if (chmod(pts, mode))
		return -1;

	return 0;
}

int 
dbg_disable_pts(const char *pts)
{
	struct stat st;
	int ret;
	int mode;

	if (!pts)
		return 0;

	ret = stat(pts, &st);
	if (ret) 
		return -1;

	mode = st.st_mode;

	if (!(mode & S_IXOTH))
		return 0;

	mode = mode & (~S_IXOTH);
	
	if (chmod(pts, mode))
		return -1;

	return 0;
}

int 
dbg_pts_is_enabled(const char *pts)
{
	struct stat st;
	int ret;

	if (!pts)
		return 0;

	ret = stat(pts, &st);
	if (ret) 
		return 0;

	if (st.st_mode & S_IXOTH) 
		return 1;

	return 0;
}

int 
dbg_enable_console(void)
{
	struct stat st;
	int ret;
	int mode;
	char file[128];

	snprintf(file, sizeof(file) - 1, "/dev/console");

	ret = stat(file, &st);
	if (ret) 
		return -1;

	mode = st.st_mode;

	if (mode & S_IXOTH)
		return 0;

	mode = mode | S_IXOTH;
	
	if (chmod(file, mode))
		return -1;

	return 0;
}

int 
dbg_disable_console(void)
{
	struct stat st;
	int ret;
	int mode;
	char file[128];

	snprintf(file, sizeof(file) - 1, "/dev/console");

	ret = stat(file, &st);
	if (ret) 
		return -1;

	mode = st.st_mode;

	if (!(mode & S_IXOTH))
		return 0;

	mode = mode & (~S_IXOTH);
	
	if (chmod(file, mode))
		return -1;

	return 0;
}

int 
dbg_console_is_enabled(void)
{
	struct stat st;
	int ret;
	char file[128];
	
	snprintf(file, sizeof(file) - 1, "/dev/console");

	ret = stat(file, &st);
	if (ret) 
		return 0;

	if (st.st_mode & S_IXOTH) 
		return 1;

	return 0;
}


