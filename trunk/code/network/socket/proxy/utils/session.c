/**
 *	@file	sesspool.c
 *
 *	@brief	The session pool implement.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2010-02-05
 */

#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h>

#include "session.h"
#include "mempool.h"


#define _SESSTBL_MAGIC		0x87654321

/**
 *	Define debug MACRO to print debug information
 */
#define _SESSION_DBG	1
#ifdef _SESSION_DBG
#define _SSN_ERR(fmt, args...)	fprintf(stderr, "%s:%d: "fmt,\
					__FILE__, __LINE__, ##args)
#else
#define _SSN_ERR(fmt, args...)
#endif

/**
 *	convert IP address(uint32_t) to dot-decimal string
 *	like xxx.xxx.xxx.xxx 
 */
#ifndef NIPQUAD
#define NIPQUAD(addr) \
        ((unsigned char *)&addr)[0], \
        ((unsigned char *)&addr)[1], \
        ((unsigned char *)&addr)[2], \
        ((unsigned char *)&addr)[3]
#define NIPQUAD_FMT "%u.%u.%u.%u"
#endif

/**
 * 	Find a free id in session table @st. and update the @st->freeid
 * 	to next position of find id.
 *
 *	Return the id if success, -1 on error.
 */
static int 
_st_get_free_id(session_table_t *st)
{
	int id;
	
	assert(st);

	id = st->freeid;
	do {
		if (id >= st->capacity)
			id = 0;

		if (st->table[id] == NULL) {
			st->freeid = id + 1;
			return id;
		}
		
		id++;
	} while (id != st->freeid);

	return -1;
}



int 
session_tup_compare(session_tup_t *tup1, session_tup_t *tup2)
{
	if (!tup1 || !tup2)
		return -1;

	/* inline mode */
	if (tup1->clifd > 0 && tup1->clifd > 0) {
		return tup1->id - tup2->id;
	}
	
	/* offline mode */
	if (tup1->sip == tup2->sip &&
	    tup1->dip == tup2->dip &&
	    tup1->sport == tup2->sport &&
	    tup1->dport == tup2->dport)
		return 0;
	else
		return -1;

}


session_table_t *
session_table_alloc(int capacity)
{
	session_table_t *t = NULL;

	if (capacity < 1) {
		_SSN_ERR("invalid argument capacity %d\n", capacity);
		return NULL;
	}

	/* alloc sesspool object */
	t = malloc(sizeof(session_table_t));
	if (!t) {
		_SSN_ERR("malloc memory failed: %s\n", strerror(errno));
		return NULL;
	}
	memset(t, 0, sizeof(session_table_t));

	/* alloc session map */
	t->table = malloc(sizeof(session_t *) * capacity);
	if (!t->table) {
		_SSN_ERR("malloc memory failed: %s\n", strerror(errno));
		free(t);
		return NULL;
	}
	memset(t->table, 0, sizeof(session_t *) * capacity);

	
	t->capacity = capacity;
	t->nfreed = capacity;
	pthread_mutex_init(&t->lock, NULL);

	return t;
}


void 
session_table_free(session_table_t *t)
{
	if (!t)
		return;

	if (t->table) {
		free(t->table);
	}

	free(t);
}


void 
session_table_lock(session_table_t *t)
{
	if (!t)
		return;

	if (pthread_mutex_lock(&t->lock)) {
		_SSN_ERR("lock session table failed\n");
	}
}


void 
session_table_unlock(session_table_t *t)
{
	if (!t)
		return;

	if (pthread_mutex_unlock(&t->lock)) {
		_SSN_ERR("unlock session table failed\n");
	}
}


int  
session_table_add(session_table_t *t, session_t *s)
{
	int id;

	if (!t || !t->table || !s) {
		return -1;
	}
	
	session_table_lock(t);

	if (t->nfreed < 1) {
		session_table_unlock(t);
		return -1;
	}

	id = _st_get_free_id(t);
	if (id < 0) {
		session_table_unlock(t);
		_SSN_ERR("can't find a free id\n");
		return -1;
	}

	s->id = id;
	s->magic = SESSION_MAGIC;
	t->table[id] = s;
	t->nfreed--;

	session_table_unlock(t);

	return 0;
}


session_t * 
session_table_find(session_table_t *t, int id)
{
	session_t *s = NULL;

	if (!t || !t->table) {
		_SSN_ERR("invalid parameter\n");
		return NULL;
	}

	if (id < 0 || id >= t->capacity) {
		_SSN_ERR("invalid parameter\n");
		return NULL;
	}

	s = t->table[id];
	if (!s) {
		_SSN_ERR("invalid map %d, session is NULL\n", id);
		return NULL;
	}

	assert(s->magic == SESSION_MAGIC);

	return s;
}


int 
session_table_del(session_table_t *t, int id)
{
	session_t *s = NULL;
	
	if (!t || !t->table || id < 0 || id >= t->capacity) {
		_SSN_ERR("invalid parameter\n");
		return -1;
	}

	/* lock the hash entry */
	session_table_lock(t);
	
	s = t->table[id];
	if (!s) {
		session_table_unlock(t);
		_SSN_ERR("free a not exist session %d\n", id);
		return -1;
	}
	
	t->table[id] = NULL;
	t->nfreed++;

	session_table_unlock(t);

	return 0;
}


void 
session_table_print(session_table_t *t)
{
	int i;

	if (!t)
		return;

	pthread_mutex_lock(&t->lock);
		
	printf("sesstbl(%p) table %p capacity %u, nfreed %d\n", 
	       t, t->table, t->capacity, t->nfreed);
	
	for (i = 0; i < t->capacity; i++) {
		if (t->table[i]) 
			printf("%d:", i);
			session_print(t->table[i]);
	}

	pthread_mutex_unlock(&t->lock);
}


void 
session_print(session_t *s)
{
	if (!s)
		return;

	printf("\tsession(%p) id %d, clifd %d, svrfd %d\n", 
	       s, s->id, s->clifd, s->svrfd);
	printf("\t\t%u.%u.%u.%u:%u->%u.%u.%u.%u:%u\n", 
	       NIPQUAD(s->sip), ntohs(s->sport), 
	       NIPQUAD(s->dip), ntohs(s->dport));
}




