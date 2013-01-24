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

#include "stdext.h"
#include "fdisk.h"
#include "fdebug.h"


/**
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

/**
 *	Get localtime.
 *
 *	Return string of local time.
 */
static const char * 
_dbg_log_ltime_stamp()
{
	static char stamp[32];
	time_t ntime;
	struct tm *tm;

	ntime = time(NULL);
	tm = localtime(&ntime);
	sprintf(stamp,
		"%04d-%02d-%02d %02d:%02d:%02d",
		tm->tm_year+1900, 
		tm->tm_mon + 1,
		tm->tm_mday, tm->tm_hour, 
		tm->tm_min,
		tm->tm_sec);
	return stamp;
}


static int 
_dbg_log_write(char *buf)
{
	crashlog_header_t h;
	off_t off1, off2;
	int fd;
	int n;

	fd = crashlog_open(&h);
	if (fd < 0) {
		return -1;
	}

	off1 = h.wpos;
	off2 = lseek(fd, off1, SEEK_SET);
	if (off2 != off1) {
		close(fd);
		return -1;
	}

	n = write(fd, buf, CRASHLOG_BUFLEN);
	if (n != CRASHLOG_BUFLEN) {
		close(fd);
		return -1;
	}

	h.wrec ++;
	h.wpos += CRASHLOG_BUFLEN;
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
_dbg_log_print_header(crashlog_header_t *h)
{
	if (!h)
		return;

	printf("magic: 0x%x\n", h->magic);
	printf("wrec: %u\n", h->wrec);
	printf("wpos: %lu\n", h->wpos);
	printf("nrec: %u\n", h->nrec);
	printf("mrec: %u\n", h->mrec);
}

int 
dbg_log_init(void)
{
	int semid;
	union semun {
		int              val;    /* Value for SETVAL */
		struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
		unsigned short  *array;  /* Array for GETALL, SETALL */
		struct seminfo  *__buf;  /* Buffer for IPC_INFO
		(Linux-specific) */
	} arg;

	semid = semget(CRASHLOG_SEMKEY, 1, 0666|IPC_CREAT);
	if (semid < 0) {
		message("%s create semaphore failed\n", __func__);
		return -1;
	}

	arg.val=1;
	if (semctl(semid, 0, SETVAL, arg) < 0) {
		message("%s semctl failed\n", __func__);
		return -1;
	}

	/* make symlink */
	if (!hd_logdisk_mounted()) {
		unlink(CRASHLOG_FILE);
		symlink(CRASHLOG_FILE2, CRASHLOG_FILE);
	}

	return 0;
}


dbg_log_lock(void)
{
	int semid;
	struct sembuf locks[1] = {{0, -1, SEM_UNDO}};

	semid = semget(CRASHLOG_SEMKEY, 1, 0);
	if (semid < 0) {
		message("%s: can't get semaphore\n", __func__);
		return -1;
	}

	while (semop(semid, &locks[4], 1) < 0) {
		if (errno == EAGAIN)
			continue;
		else {
			message("%s: semop failed\n", __func__);
			return -1;
		}
	}

	return 0;
}


int 
dbg_log_unlock(void)
{
	int semid;

	struct sembuf unlock[1] = {{0, 1, SEM_UNDO}};

	semid = semget(CRASHLOG_SEMKEY, 1, 0);
	if (semid < 0) {
		message("%s: semget failed\n", __func__);
		return -1;
	}


	if (semop(semid, &unlock[0], 1) < 0) {
		message("%s: semop failed\n", __func__);
		return -1;
	}
	
	return 0;
}


int 
dbg_log_open(crashlog_header_t *h)
{
	int fd;
	int n;
	int hlen = sizeof(crashlog_header_t);

	if (!h) 
		return -1;

	fd = open(CRASHLOG_FILE, O_RDWR | O_CREAT, 0777);
	if (fd < 0) {
		message("%s: open/create %s failed\n", 
			__func__, CRASHLOG_FILE);
		return -1;
	}

	n = read(fd, (char *)h, hlen);
	if (n == hlen && h->magic == CRASHLOG_MAGIC)
		return fd;

	h->magic = CRASHLOG_MAGIC;
	h->nrec  = 0;
	h->wrec  = 0;
	h->mrec  = CRASHLOG_MAX;
	h->wpos  = hlen;
	
	lseek(fd, 0, SEEK_SET);
	n = write(fd, (char *)h, hlen);
	if (n != hlen) {
		close(fd);
		return -1;
	}

	return fd;
}


int 
dbg_log_clear(void)
{
	crashlog_header_t h;
	int fd;
	int n;
	int ret = 0;

	if (crashlog_lock()) 
		return -1;

	fd = open(CRASHLOG_FILE, O_RDWR | O_CREAT, 0777);
	if (fd < 0) {
		message("%s: open/create %s failed\n", 
			__func__, CRASHLOG_FILE);
		crashlog_unlock();
		return -1;
	}

	h.magic = CRASHLOG_MAGIC;
	h.mrec  = CRASHLOG_MAX;
	h.wrec  = 0;
	h.nrec  = 0;
	h.wpos  = sizeof(h);
	
	lseek(fd, 0, SEEK_SET);
	n = write(fd, (char *)&h, sizeof(h));
	if (n != sizeof(h)) {
		message("%s: write header to %s failed\n", 
			__func__, CRASHLOG_FILE);
		crashlog_unlock();
		ret = -1;
	}

	close(fd);

	if (crashlog_unlock())
		ret = -1;

	return ret;
}

dbg_log_lock(void)
{

}


dbg_log_unlock(void)
{

}

void 
dbg_log_print(const char *fmt, ...)
{
	char buf[CRASHLOG_BUFLEN];
	char msg[CRASHLOG_BUFLEN];
	const char *s;
	va_list arg;

	memset(msg, 0, CRASHLOG_BUFLEN);
	va_start(arg, fmt);
	vsnprintf(msg, CRASHLOG_BUFLEN, fmt, arg);
	va_end(arg);

	msg[CRASHLOG_BUFLEN - 1] = '\0';
	memset(buf, 0, CRASHLOG_BUFLEN);
	s = _cl_ltime_stamp();
	snprintf(buf, CRASHLOG_BUFLEN - 1, "%s %s\n", s, msg);

	if (crashlog_lock())
		return;

	_cl_write(buf);

	crashlog_unlock();
}


int 
dbg_log_read(int index, char *buf, size_t len)
{
	off_t off1, off2;
	crashlog_header_t h;
	int srec = 0;
	int n;
	int fd;

	if (!buf || len < 0)
		return -1;

	if (len > CRASHLOG_BUFLEN)
		len = CRASHLOG_BUFLEN;
	
	if (crashlog_lock())
		return -1;

	fd = crashlog_open(&h);
	if (fd < 0) {
		crashlog_unlock();
		return -1;
	}
	
	if (index < 0 || index >= h.nrec) {
		crashlog_unlock();
		return -1;
	}

	if (h.nrec == h.mrec) {
		srec = h.wrec;
	}

	srec = (srec + index) % h.mrec;
	off1 = sizeof(h) + srec * CRASHLOG_BUFLEN;

	off2 = lseek(fd, off1, SEEK_SET);
	if (off2 != off1) {
		crashlog_unlock();
		return -1;
	}

	n = read(fd, buf, len);
	if (n != len) {
		crashlog_unlock();
		return -1;
	}

	crashlog_unlock();
	close(fd);

	return 0;
}



