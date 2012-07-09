/**
 *	@file	dbg_file.c
 *
 *	@brief	log the debug information into file
 *	
 *	@author	
 *
 *	@date	2010-07-19
 */
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "dbg_file.h"

/**
 *	Using static fd, it's urgely but safe.
 */
static int _dbg_abnormal_fd = -1;
static int _dbg_deadlock_fd = -1;
static int _dbg_infloop_fd = -1;
static char _dbg_progname[DBG_PROGNAME_MAX] = {0};

/**
 *	Create debug directory
 *
 *	Return 0 if success.
 */
static int 
_dbg_creat_dir(void)
{
	struct stat st;

	/* check the path exist and it's a directory */
	if (access(DBG_FILE_DIR, F_OK) == 0 &&
	    stat(DBG_FILE_DIR, &st) == 0) {
		if (S_ISDIR(st.st_mode)) {
			return 0;
		}
		else {
			unlink(DBG_FILE_DIR);
		}
	}
	
	/* create directory */
	if (mkdir(DBG_FILE_DIR, 0755)) {
		return -1;
	}

	return 0;
}

/**
 *	init the dbg_file APIs.
 *
 *	No return.
 */
void 
dbg_file_init(const char *progname)
{
	if (!progname)
		return;

	snprintf(_dbg_progname, DBG_PROGNAME_MAX - 1, "%s", progname);

	if (_dbg_creat_dir())
		return;

	/* create abnormal file */
	if (access(DBG_ABNORMAL_FILE, F_OK))
		_dbg_abnormal_fd = creat(DBG_ABNORMAL_FILE, 0644);
	else
		_dbg_abnormal_fd = open(DBG_ABNORMAL_FILE, O_WRONLY);
	if (_dbg_abnormal_fd < 0)
		return;

	/* open infloop file */
	if (access(DBG_INFLOOP_FILE, F_OK))
		_dbg_infloop_fd = creat(DBG_INFLOOP_FILE, 0644);
	else
		_dbg_infloop_fd = open(DBG_INFLOOP_FILE, O_WRONLY);
	if (_dbg_infloop_fd < 0)
		return;

	/* open deadlock file */
	if (access(DBG_DEADLOCK_FILE, F_OK))
		_dbg_deadlock_fd = creat(DBG_DEADLOCK_FILE, 0644);
	else
		_dbg_deadlock_fd = open(DBG_DEADLOCK_FILE, O_WRONLY);
	if (_dbg_deadlock_fd < 0)
		return;
}

/**
 *	Log the abnormal information into file.
 *
 *	No return.
 */
void 
dbg_abnormal(const char *fmt, ...)
{
	va_list ap;
	char buf[DBG_LINE_MAX];
	char buf1[DBG_LINE_MAX];

	if (_dbg_abnormal_fd < 0) {
		return;
	}

	va_start(ap, fmt);
	vsnprintf(buf1, DBG_LINE_MAX - 1, fmt, ap);
	va_end(ap);
	
	snprintf(buf, DBG_LINE_MAX - 1, "[%s]: %s", _dbg_progname, buf1);
	buf[DBG_LINE_MAX - 1] = 0;
	write(_dbg_abnormal_fd, buf, strlen(buf));
}


/**
 *	Log the infinite loop into file.
 *
 *	No return.
 */
void 
dbg_infloop(const char *fmt, ...)
{
	va_list ap;
	char buf[DBG_LINE_MAX];
	char buf1[DBG_LINE_MAX];

	if (_dbg_infloop_fd < 0) {
		return;
	}

	va_start(ap, fmt);
	vsnprintf(buf1, DBG_LINE_MAX -1, fmt, ap);
	va_end(ap);
	
	snprintf(buf, DBG_LINE_MAX - 1, "[%s]: %s", _dbg_progname, buf1);
	buf[DBG_LINE_MAX - 1] = 0;
	write(_dbg_infloop_fd, buf, strlen(buf));
}


/**
 *	Log the dead lock into file
 *
 *	No return.
 */
void 
dbg_deadlock(const char *fmt, ...)
{
	va_list ap;
	char buf[DBG_LINE_MAX];
	char buf1[DBG_LINE_MAX];

	if (_dbg_deadlock_fd < 0) {
		return;
	}

	va_start(ap, fmt);
	vsnprintf(buf1, DBG_LINE_MAX -1, fmt, ap);
	va_end(ap);
	
	snprintf(buf, DBG_LINE_MAX - 1, "[%s]: %s", _dbg_progname, buf1);
	buf[DBG_LINE_MAX - 1] = 0;
	write(_dbg_deadlock_fd, buf, strlen(buf));
}

/**
 *	Release resource alloc by dbg_file_init
 *
 *	No return.
 */
void 
dbg_file_release(void)
{
	if (_dbg_abnormal_fd >= 0) {
		close(_dbg_abnormal_fd);
		_dbg_abnormal_fd = -1;
	}

	if (_dbg_infloop_fd >= 0) {
		close(_dbg_abnormal_fd);
		_dbg_abnormal_fd = -1;
	}

	if (_dbg_deadlock_fd >= 0) {
		close(_dbg_abnormal_fd);
		_dbg_abnormal_fd = -1;
	}
	
	memset(_dbg_progname, 0, DBG_PROGNAME_MAX);
}
