/**
 *	@file	debug.c
 *
 *	@brief	The debug API for program.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2009-10-28
 */

#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>

#include "debug.h"


/**
 *	Alloc a dbg_db_t object.
 *
 *	Return 0 if success, -1 on error.
 */
dbg_db_t *
dbg_alloc_db(int capacity)
{
	int i;
	dbg_db_t *db;
	dbg_msg_t *msg;

	db = malloc(sizeof(dbg_db_t));
	if (!db) {
		return NULL;
	}
	memset(db, 0, sizeof(dbg_db_t));

	db->pool = malloc(capacity * DBG_MSG_LEN);
	if (!db->pool) {
		dbg_free_db(db);
		return NULL;
	}
	memset(db->pool, 0, capacity * DBG_MSG_LEN);

	for (i = 0; i < capacity; i++) {
		msg = (dbg_msg_t *)(db->pool + (DBG_MSG_LEN * i));
		if (i == (capacity - 1) )
			msg->next = NULL;
		else 
			msg->next = (db->pool + (DBG_MSG_LEN * (i + 1) ) );
	}
	db->freed = db->pool;
	db->capacity = capacity;

	return db;
}


/**
 *	Free a dbg_db_t object.
 *
 *	No return.
 */
void
dbg_free_db(dbg_db_t *db)
{
	if (!db)
		return;

	if (db->pool)
		free(db->pool);
	
	free(db);
}


static dbg_msg_t *
_dbg_get_free_msg(dbg_db_t *db)
{
	dbg_msg_t *msg;

	if (!db)
		return NULL;

	if (db->nmsgs > db->capacity ||
	    db->nmsgs < 0) {
		printf("error, db overflow(%d:%d)\n", 
		       db->nmsgs, db->capacity);
		return NULL;
	}

	if (db->nmsgs == db->capacity) {
		msg = db->head;
		if (!msg) {
			printf("error, db size is error\n");
			return NULL;
		}
		db->head = msg->next;
		db->nmsgs --;
	}
	else {
		msg = db->freed;
		if (!msg) {
			printf("error, db->capacity is error\n");
			return NULL;
		}
		db->freed = msg->next;		
	}
	memset(msg, 0, sizeof(dbg_msg_t));

	return msg;
}


/**
 *	Add a log message into @db.
 *
 *	No return.
 */
extern void 
dbg_log_db(dbg_db_t *db, const char *fmt, ...)
{
	dbg_msg_t *msg;
	va_list ap;

	msg = _dbg_get_free_msg(db);
	if (!msg)
		return;

	va_start(ap, fmt);

	vsnprintf(msg->msg, DBG_STR_LEN, fmt, ap);
	msg->len = strlen(msg->msg);

	va_end(ap);

	if (db->tail)
		db->tail->next = msg;
	else
		db->head = msg;
	db->tail = msg;

	db->nmsgs++;
}


/**
 *	Save log database @db into file @file according 
 *	log sequence, one line for one log.
 *
 *	No return.
 */
void 
dbg_save_db(dbg_db_t *db, const char *file)
{
	dbg_msg_t *msg;
	int fd;
	
	if (!db || db->nmsgs < 1)
		return;

	fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0) {
		printf("open file %s failed\n", file);
		return;
	}

	msg = db->head;
	while (msg) {
		write(fd, msg->msg, msg->len);
		if (msg->msg[msg->len - 1] != '\n')
			write(fd, "\n", 1);

		msg = msg->next;
	}

	close(fd);
}


/**
 *	Add a log into fd, the fd is file descript.
 *	The first 4 byte is the lenth of message.
 *
 *	No return.
 */
void 
dbg_log_fd(int fd, const char *fmt, ...)
{
	char buf[DBG_STR_LEN + 1] = {0};
	va_list ap;
	int len = 0;
	char lbuf[5] = {0};
	int n;

	if (fd < 0)
		return;

	va_start(ap, fmt);

	vsnprintf(buf, DBG_STR_LEN, fmt, ap);
	len = strlen(buf);

	va_end(ap);

	snprintf(lbuf, 5, "%.4d", len);
	
	n = write(fd, lbuf, 4);
	if (n != 4) {
		printf("write error\n");
		return;
	}

	n = write(fd, buf, len);
	if (n != len) {
		printf("write error\n");
		return;
	}
}


/**
 *	Recv log from fd and add it to db
 *
 *	No return.
 */
void
dbg_log_recv(dbg_db_t *db, int fd)
{
	int n, total = 0, left;
	char len[5];
	char buf[DBG_MSG_LEN];
	char *ptr;
	dbg_msg_t *msg;
	
	if (fd < 0)
		return;
	
	n = read(fd, len, 4);
	if (n != 4)
		return;

	len[4] = 0;
	total = atoi(len);
	if (total < 1)
		return;

	left = total;
	ptr = buf;
	while (left) {
		n = read(fd, ptr, left);
		if (n < 0)
			return;
		if (n == 0 && errno != EAGAIN)
			return;

		ptr += n;
		left -= n;
	}

	buf[total] = 0;

	msg = _dbg_get_free_msg(db);
	if (!msg)
		return;

	msg->len = total;
	strncpy(msg->msg, buf, DBG_STR_LEN);

	if (db->tail)
		db->tail->next = msg;
	else
		db->head = msg;
	db->tail = msg;

	db->nmsgs++;

	printf("now have %d messages\n", db->nmsgs);
}


/**
 *	Print the stack of crashed program.
 *
 *	No return.
 */
void
dbg_stack(void)
{


}
