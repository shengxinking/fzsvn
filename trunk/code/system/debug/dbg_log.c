/**
 *	@file	crashlog.c
 *
 *	@brief	implement Crash Log functions
 *
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <wait.h>
#include <sys/file.h>
#include <time.h>
#include <sys/vfs.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#include "dbg_log.h"


#define	_DL_ERR(fmt, args...)		\
	printf("<%s:%d> "fmt, __FILE__, __LINE__, ##args);

/*************************************
 * 	log file struct
 * 	                             
 *-----------------------------------*
 * 	log_header                   *
 *-----------------------------------*
 * 	log record 0                 *
 *      log record 1                 *
 *      ..............               *
 *      log record N                 *
 *-----------------------------------*/

typedef struct _dl_file {
	u_int32_t	magic;		/* magic number */
	u_int32_t	semkey;
	u_int32_t	reclen;		/* log reacord max length */
	u_int32_t	maxrec;		/* max log record */
	char		file[128];	/* log file */
} _dl_file_t;

typedef struct _dl_header {
	u_int32_t	magic;		/* magic number */
	u_int32_t	wpos;		/* write position */
	u_int32_t	wrec;		/* write record number */
	u_int32_t	mrec;		/* max record */
	u_int32_t	nrec;		/* number of records */
} _dl_header_t;

static _dl_file_t _dl_files[] = {
	{
		.magic = 0x12345678,
		.semkey = 0x12345678,
		.reclen = 256,
		.maxrec = 5000,
		.file = "/var/log/crash.log"
	},
	{
		.magic = 0x13572468,
		.semkey = 0x13572468,
		.reclen = 512,
		.maxrec = 10000,
		.file = "/var/log/kernel.log"
	},
	{
		.magic = 0x24681357,
		.semkey = 0x24681357,
		.reclen = 512,
		.maxrec = 10000,
		.file = "/var/log/console.log"
	},
};

/**
 *	Get begin of each log record(timestamp).
 *
 *	Return string of local time.
 */
static int 
_dl_get_bol(char *buf, size_t len)
{
	time_t ntime;
	struct tm *tm;

	if (!buf || len < 20)
		return -1;

	ntime = time(NULL);
	tm = localtime(&ntime);
	sprintf(buf,
		"%04d-%02d-%02d %02d:%02d:%02d",
		tm->tm_year+1900, 
		tm->tm_mon + 1,
		tm->tm_mday, tm->tm_hour, 
		tm->tm_min,
		tm->tm_sec);
	return 0;
}


static int 
_dl_lock(_dl_file_t *df)
{
	int semid;
	struct sembuf locks[1] = {{0, -1, SEM_UNDO}};
	
	if (!df)
		return -1;

	semid = semget(df->semkey, 1, 0);
	if (semid < 0) {
		_DL_ERR("can't get semaphore\n");
		return -1;
	}

	while (semop(semid, &locks[4], 1) < 0) {
		if (errno == EAGAIN)
			continue;
		else {
			_DL_ERR("semop failed\n");
			return -1;
		}
	}

	return 0;
}


static int 
_dl_unlock(_dl_file_t *df)
{
	int semid;

	struct sembuf unlock[1] = {{0, 1, SEM_UNDO}};

	if (!df)
		return -1;

	semid = semget(df->semkey, 1, 0);
	if (semid < 0) {
		_DL_ERR("semget failed\n");
		return -1;
	}


	if (semop(semid, &unlock[0], 1) < 0) {
		_DL_ERR("semop failed\n");
		return -1;
	}
	
	return 0;
}


static int 
_dl_open(_dl_file_t *df, _dl_header_t *h)
{
	int fd;
	int n;
	int hlen = sizeof(_dl_header_t);

	if (!df || !h) 
		return -1;

	fd = open(df->file, O_RDWR | O_CREAT, 0644);
	if (fd < 0) {
		_DL_ERR("open/create %s failed\n", df->file);
		return -1;
	}

	n = read(fd, (char *)h, hlen);
	if (n == hlen && h->magic == df->magic)
		return fd;

	h->magic = df->magic;
	h->nrec  = 0;
	h->wrec  = 0;
	h->mrec  = df->maxrec;
	h->wpos  = hlen;
	
	lseek(fd, 0, SEEK_SET);
	n = write(fd, (char *)h, hlen);
	if (n != hlen) {
		_DL_ERR("write %s failed\n", df->file);
		close(fd);
		return -1;
	}

	return fd;
}


static int 
_dl_write(_dl_file_t *df, char *buf)
{
	_dl_header_t h;
	off_t off1, off2;
	int fd;
	int n;
	
	fd = _dl_open(df, &h);
	if (fd < 0) {
		return -1;
	}

	off1 = h.wpos;
	off2 = lseek(fd, off1, SEEK_SET);
	if (off2 != off1) {
		close(fd);
		return -1;
	}

	n = write(fd, buf, df->reclen);
	if (n != df->reclen) {
		close(fd);
		return -1;
	}

	h.wrec ++;
	h.wpos += df->reclen;
	if (h.wrec >= h.mrec) {
		h.wrec = 0;
		h.wpos = sizeof(h);
	}
	else if (h.nrec < h.mrec) {
		h.nrec ++;
	}

	lseek(fd, 0, SEEK_SET);
	n = write(fd, (char *)&h, sizeof(h));
	if (n != sizeof(h)) {
		close(fd);
		return -1;
	}

	return 0;
}


void 
_dl_print_header(_dl_header_t *h)
{
	if (!h)
		return;

	printf("magic: 0x%x\n", h->magic);
	printf("wrec: %u\n", h->wrec);
	printf("nrec: %u\n", h->nrec);
	printf("mrec: %u\n", h->mrec);
	printf("wpos: %u\n", h->wpos);
}

int 
dbg_log_init(int type)
{
	int semid;
	_dl_file_t *df;
	union semun {
		int              val;    /* Value for SETVAL */
		struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
		unsigned short  *array;  /* Array for GETALL, SETALL */
		struct seminfo  *__buf;  /* Buffer for IPC_INFO
		(Linux-specific) */
	} arg;

	if (type < 0 || type >= DBG_LOG_MAX)
		return -1;

	df = &_dl_files[type];

	semid = semget(df->semkey, 1, 0666|IPC_CREAT);
	if (semid < 0) {
		_DL_ERR("create semaphore failed\n");
		return -1;
	}

	arg.val=1;
	if (semctl(semid, 0, SETVAL, arg) < 0) {
		_DL_ERR("semctl failed\n");
		return -1;
	}

	return 0;
}


int 
dbg_log_read(int type, int index, char *buf, size_t len)
{
	_dl_file_t *df;
	_dl_header_t h;
	off_t off1, off2;
	int srec = 0;
	int n;
	int fd;

	if (!buf || len < 1)
		return -1;

	if (type < 0 || type >= DBG_LOG_MAX)
		return -1;

	df = &_dl_files[type];

	if (len > df->reclen)
		len = df->reclen;
	
	if (_dl_lock(df))
		return -1;

	fd = _dl_open(df, &h);
	if (fd < 0) {
		_dl_unlock(df);
		return -1;
	}
	
	if (index < 0 || index >= h.nrec) {
		_dl_unlock(df);
		return -1;
	}

	if (h.nrec == h.mrec) {
		srec = h.wrec;
	}

	srec = (srec + index) % h.mrec;
	off1 = sizeof(h) + srec * df->reclen;

	off2 = lseek(fd, off1, SEEK_SET);
	if (off2 != off1) {
		_dl_unlock(df);
		close(fd);
		return -1;
	}

	n = read(fd, buf, len);
	if (n != len) {
		_dl_unlock(df);
		close(fd);
		return -1;
	}

	_dl_unlock(df);
	close(fd);

	return 0;
}


int 
dbg_log_clear(int type)
{
	_dl_file_t *df;
	_dl_header_t h;
	int fd;
	int n;

	if (type < 0 || type >= DBG_LOG_MAX)
		return -1;

	df = &_dl_files[type];

	if (_dl_lock(df)) 
		return -1;

	fd = _dl_open(df, &h);
	if (fd < 0)
		return -1;

	if (h.nrec == 0)
		return 0;

	h.magic = df->magic;
	h.mrec  = df->maxrec;
	h.wrec  = 0;
	h.nrec  = 0;
	h.wpos  = sizeof(h);
	
	lseek(fd, 0, SEEK_SET);
	n = write(fd, (char *)&h, sizeof(h));
	close(fd);
	_dl_unlock(df);

	if (n != sizeof(h)) {
		_DL_ERR("write header to %s failed\n", df->file);
		return -1;
	}

	return 0;
}

void 
dbg_log(int type, const char *fmt, ...)
{
	_dl_file_t *df;
	char buf[DBG_LOG_LEN] = {0};
	char bol[DBG_LOG_LEN];
	char msg[DBG_LOG_LEN];
	va_list arg;

	if (!fmt)
		return;

	if (type < 0 || type >= DBG_LOG_MAX)
		return;

	df = &_dl_files[type];

	/* get message */
	memset(msg, 0, sizeof(msg));
	va_start(arg, fmt);
	vsnprintf(msg, DBG_LOG_LEN - 1, fmt, arg);
	va_end(arg);

	/* get begin of line */
	memset(bol, 0, DBG_LOG_LEN);
	if (_dl_get_bol(bol, sizeof(bol)))
		return;

	/* combine bol + msg, not exceed record length */
	snprintf(buf, df->reclen - 1, "%s %s\n", bol, msg);

	if (_dl_lock(df))
		return;
	
	/* write to log file */
	_dl_write(df, buf);

	_dl_unlock(df);
}





