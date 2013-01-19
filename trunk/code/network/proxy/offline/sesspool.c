/*
 *	@file	session.c
 *
 *	@brief	implement a session table for work thread
 *
 *
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>

#include "sesspool.h"



/**
 *	Map a socket fd to a session id.
 */
typedef struct _sesspool_fdmap {
	int	id;
	int	clifd;
	int	svrfd;
} _sesspool_fdmap_t;


/**
 *	Session hash entry.
 */
typedef struct _sesspool_hash {
	int		size;
	session_t	*head;
} _sesspool_hash_t;


/**
 *	convert id to hash key
 */
#define _SESSPOOL_HKEY(id)		(id % SESSPOOL_HSIZE)

/**
 *	Session debug MACRO
 */
#define _SESSPOOL_DEBUG
#ifdef	_SESSPOOL_DEBUG
#define _SESSPOOL_ERR(fmt, args...)	fprintf(stderr, "session:%s:%d: " fmt, \
						__FILE__, __LINE__, ##args)
#else
#define _SESSPOOL_ERR(fmt, args...)
#endif

/**
 *	convert IP address(u_int32_t) to dot-decimal string
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
 *	Lock session pool @sp if LOCK flag is set.
 *
 *	No return.
 */
void
_sesspool_lock(sesspool_t *sp)
{
	assert(sp);
	
	if (SESSPOOL_IS_LOCK(sp->flags))
		pthread_mutex_lock(&sp->lock);
}

/**
 *	Unlock session pool @sp if LOCK flag is set.
 *
 */
void
_sesspool_unlock(sesspool_t *sp)
{
	assert(sp);

	if (SESSPOOL_IS_LOCK(sp->flags))
		pthread_mutex_unlock(&sp->lock);
}

int
_sesspool_fdmap_find(_sesspool_fdmap_t *map, int size, int fd)
{
	int i;
	int id = -1;
	int count = 0;

	assert(map);
	assert(size > 0);
	assert(fd >= 0);

	for (i = 0; i < size; i++) {
		if (map[i].id >= 0) {
			if (map[i].clifd == fd) {
				count++;
				id = map[i].id;
			}
			if (map[i].svrfd == fd) {
				count++;
				id = map[i].id;
			}
		}
	}

	assert(count <= 1);

	return id;
}

static int 
_sesspool_fdmap_find_free(_sesspool_fdmap_t *map, int size, int start)
{
	int i;

	assert(map);
	assert(size > 0);
	assert(start >= 0 && start < size);

	for (i = start; i < size; i++) {
		if (map[i].id == -1)
			return i;
	}

	for (i = 0; i < start; i++) {
		if (map[i].id == -1)
			return i;
	}

	return -1;
}

static int 
_sesspool_fdmap_add(_sesspool_fdmap_t *map, int size, 
		     int id, int clifd, int svrfd)
{
	assert(map);
	assert(size > 0);
	assert(id >= 0 && id < size);

	assert(map[id].id == -1 || map[id].id == id);

	map[id].id = id;
	map[id].clifd = clifd;
	map[id].svrfd = svrfd;

	return 0;
}

int
_sesspool_fdmap_del(_sesspool_fdmap_t *map, int size, int id)
{
	assert(map);
	assert(size > 0);
	assert(id >= 0 && id < size);

	assert(map[id].id == id);
	
	map[id].id = -1;
	map[id].clifd = -1;
	map[id].svrfd = -1;

	return 0;
}

session_t * 
_sesspool_hash_find(_sesspool_hash_t *hash, int id)
{
	session_t *session;

	assert(hash);
	assert(id >= 0);

	session = hash->head;
	while (session) {
		if (session->id == id)
			return session;
		session = session->next;
	}
	
	return NULL;
}

int
_sesspool_hash_del(_sesspool_hash_t *hash, int id)
{
	session_t *session;

	assert(hash);
	assert(id >= 0);

	session = hash->head;
	while (session) {
		if (session->id == id) {
			if (session->prev) {
				session->prev->next = session->next;
			}
			else {
				hash->head = session->next;
			}

			if (session->next) {
				session->next->prev = session->prev;
			}
			
			session->prev = session->next = NULL;

			return 0;
		}
		session = session->next;
	}

	_SESSPOOL_ERR("can't find id %d in hash\n", id);

	return -1;
}

static int
_sesspool_hash_add(_sesspool_hash_t *hash, session_t *session)
{
	assert(hash);
	assert(session);

	if (hash->head) {
		hash->head->prev = session;
	}

	session->next = hash->head;
	session->prev = NULL;

	hash->head = session;
	hash->size++;

	return 0;
}

static session_t *
_sesspool_hash_get(_sesspool_hash_t *hash)
{
	session_t *session;

	assert(hash);
	
	if (hash->size <= 0)
		return NULL;

	session = hash->head;
	if (session->next)
		session->next->prev = NULL;
	
	hash->head = session->next;
	hash->size--;

	return session;
}

/**
 *	Alloc an new session pool, it can contain @size sessions, and
 *	if the lock is not zero, it using pthread_mutext to protect
 *	this session pool. 
 *
 *	Return new session pool if success, NULL on error.
 */
sesspool_t *
sesspool_alloc(int size, int lock)
{
	int i;
	sesspool_t *sp;
	session_t *session;
	_sesspool_fdmap_t *map;
	_sesspool_hash_t *hash;

	if (size < 1)
		return NULL;

	/* alloc session table */
	sp = malloc(sizeof(sesspool_t));
	if (!sp) {
		_SESSPOOL_ERR("can't alloc memory for session table\n");
		return NULL;
	}
	memset(sp, 0, sizeof(sesspool_t));

	/* alloc memory for sessions */
	sp->pool = malloc(sizeof(session_t) * size);
	if (!sp->pool) {
		free(sp);

		_SESSPOOL_ERR("can't alloc memory for streams point\n");
		return NULL;
	}

	session = sp->pool;
	for (i = 0; i < size; i++) {
		if (i == 0)
			session[i].prev = NULL;
		else
			session[i].prev = &session[i - 1];

		if (i == size - 1)
			session[i].next = NULL;
		else
			session[i].next = &session[i+1];
		session[i].id = -1;
		session[i].flags = 0;
		session[i].clifd = -1;
		session[i].svrfd = -1;
		session[i].sip = 0;
		session[i].dip = 0;
		session[i].sport = 0;
		session[i].dport = 0;
		session[i].owner = 0L;
	}

	/* alloc memory for map */
	map = malloc(sizeof(_sesspool_fdmap_t) * size);
	if (!map) {
		free(sp->pool);
		free(sp);

		_SESSPOOL_ERR("can't alloc memory for session map\n");
		return NULL;
	}
	
	for (i = 0; i < size; i++) {
		map[i].id = -1;
		map[i].clifd = -1;
		map[i].svrfd = -1;
	};
	sp->map = map;
	
	/* alloc memory for hash */
	hash = malloc(sizeof(_sesspool_hash_t) * (SESSPOOL_HSIZE + 1));
	if (!hash) {
		free(sp->pool);
		free(sp->map);
		
		_SESSPOOL_ERR("can't alloc memory for session hash\n");
		return NULL;
	}
	memset(hash, 0, sizeof(_sesspool_hash_t) * (SESSPOOL_HSIZE));
	hash[SESSPOOL_HSIZE].size = size;
	hash[SESSPOOL_HSIZE].head = session;

	sp->hash = hash;

	sp->size = size;
	sp->nused = 0;
	sp->freeid = 0;
	sp->flags = 0;

	if (lock) {
		SESSPOOL_SET_LOCK(sp->flags);
		printf("init lock\n");
		pthread_mutex_init(&sp->lock, NULL);
	}

	return sp;
}

/**
 *	Free a session pool @sp which is alloced using sesspool_alloc().
 *
 *	No return.
 */
void 
sesspool_free(sesspool_t *sp)
{
	_sesspool_hash_t *hash, *entries;
	session_t *session;
	int i;

	if (!sp)
		return;

	if (sp->map)
		free(sp->map);

	if (sp->hash) {
		entries = sp->hash;
		for (i = 0; i < SESSPOOL_HSIZE; i++) {
			hash = &entries[i];
			session = hash->head;
			while (session) {
				if (session->clifd >= 0)
					close(session->clifd);
				if (session->svrfd >= 0)
					close(session->svrfd);
				
				session = session->next;
			}
		}

		free(sp->hash);
	}
	

	if (sp->pool)
		free(sp->pool);

	free(sp);
}

/**
 *	Add an session to session pool @sp, the new session socket pair 
 *	is (@clifd, @svrfd). If LOCK flags is set in @sp->flags, the
 *	thread id is record in new session, so this session only can
 *	used by same thread. I use it to reduce lock time.
 *
 *	Return new session if success, NULL on error.
 */
session_t *
sesspool_add(sesspool_t *sp, int clifd, int svrfd)
{
	_sesspool_hash_t *hash, *entries;
	session_t *session;
	int id;
	int hkey;
	
	if (!sp || (clifd < 0 && svrfd < 0) || clifd == svrfd)
		return NULL;
	
	entries = sp->hash;

	_sesspool_lock(sp);

	if (sp->nused >= sp->size) {
		_sesspool_unlock(sp);
		return NULL;
	}

	if (clifd >= 0) {
		id = _sesspool_fdmap_find(sp->map, sp->size, clifd);
		if (id >= 0) {
			_SESSPOOL_ERR("clifd %d is already in session pool\n", 
				      clifd);

			_sesspool_unlock(sp);
			return NULL;
		}
	}
	if (svrfd >= 0) {
		id = _sesspool_fdmap_find(sp->map, sp->size, svrfd);
		if (id >= 0) {
			_SESSPOOL_ERR("svrfd %d is already in session pool\n",
				      svrfd);

			_sesspool_unlock(sp);
			return NULL;
		}
	}


	id = _sesspool_fdmap_find_free(sp->map, sp->size, sp->freeid);
	assert(id >= 0);

	sp->freeid = id + 1;
	if (sp->freeid >= sp->size)
		sp->freeid = 0;

	hkey = _SESSPOOL_HKEY(id);
	hash = &(entries[hkey]);

	session = _sesspool_hash_get(&entries[SESSPOOL_HSIZE]);
	assert(session);

	session->id = id;
	session->clifd = clifd;
	session->svrfd = svrfd;
	session->sip = 0;
	session->dip = 0;
	session->sport = 0;
	session->dport = 0;
	session->flags = 0;
	
	if (SESSPOOL_IS_LOCK(sp->flags)) {
		session->owner = pthread_self();
	}
	else {
		session->owner = 0L;
	}
	
	_sesspool_hash_add(hash, session);
	_sesspool_fdmap_add(sp->map, sp->size, id, clifd, svrfd);
	
	sp->nused++;

	_sesspool_unlock(sp);

	return session;
}

/**
 *	Delete a session in session pool @sp according session's socket 
 *	fd @fd. If LOCK is set in @sp->flags, only the thread which create
 *	this session can delete it, the other thread should not delete it.
 *
 *	Return 0 if success, -1 on error.
 */
int 
sesspool_del(sesspool_t *sp, int id)
{
	_sesspool_hash_t *entries, *hash;
	session_t *session;
	int hkey;

	if (!sp || id < 0 || id > sp->size)
		return -1;
	
	entries = sp->hash;

	_sesspool_lock(sp);

	hkey = _SESSPOOL_HKEY(id);
	hash = &entries[hkey];

	session = _sesspool_hash_find(hash, id);
	if (!session) {
		_SESSPOOL_ERR("can't find id %d in pool\n", id);
		
		_sesspool_unlock(sp);

		return -1;
	}

	/* verify thread id in thread-safe environment */
	if (SESSPOOL_IS_LOCK(sp->flags))
		assert(session->owner == pthread_self());

	_sesspool_hash_del(hash, id);
	_sesspool_fdmap_del(sp->map, sp->size, id);

	_sesspool_hash_add(&entries[SESSPOOL_HSIZE], session);

	sp->nused--;

	_sesspool_unlock(sp);

	return 0;
}

/**
 *	Make client socket @clifd and server socket @svrfd as a pair
 *	in one session.
 *
 *	Return 0 if success, -1 on error
 */
int
sesspool_map(sesspool_t *sp, int clifd, int svrfd)
{
	int cliid, svrid;
	int hkey;
	session_t *session;
	_sesspool_hash_t *entries, *hash;

	if (!sp || clifd < 0 || svrfd < 0) {
		return -1;
	}

	entries = sp->hash;
	
	_sesspool_lock(sp);

	cliid = _sesspool_fdmap_find(sp->map, sp->size, clifd);
	if (cliid >= 0) {
		svrid = _sesspool_fdmap_find(sp->map, sp->size, svrfd);
		if (svrid >= 0) {			
			if (svrid != cliid) {
				_SESSPOOL_ERR("clifd %d id is %d, svrfd %d id is %d\n",
					   clifd, cliid, svrfd, svrid);

				_sesspool_unlock(sp);
				return -1;
			}
			else {
				_sesspool_unlock(sp);
				return 0;
			}
		}
			
		hkey = _SESSPOOL_HKEY(cliid);
		hash = &(entries[hkey]);
		session = _sesspool_hash_find(hash, cliid);

		assert(session);
		if (session->svrfd >= 0) {
			_SESSPOOL_ERR("svr fd %d conflict with clifd %d origin svrfd %d\n",
				      svrfd, clifd, session->svrfd);
			_sesspool_unlock(sp);

			return -1;
		}

		if (SESSPOOL_IS_LOCK(sp->flags))
			assert(session->owner == pthread_self());
		
		session->svrfd = svrfd;
		_sesspool_fdmap_add(sp->map, sp->size, cliid, clifd, svrfd);
	}
	else {
		svrid = _sesspool_fdmap_find(sp->map, sp->size, svrfd);
		if (svrid < 0) {
			_SESSPOOL_ERR("can't find clifd %d svrfd %d in pool\n",
				      clifd, svrfd);

			_sesspool_unlock(sp);
			return -1;
		}
		
		hkey = _SESSPOOL_HKEY(svrid);
		hash = &(entries[hkey]);
		session = _sesspool_hash_find(hash, svrid);		
		assert(session);
		if (session->clifd >= 0) {
			_SESSPOOL_ERR("clifd %d conflict with svrfd %d origin clifd %d\n",
				      clifd, svrfd, session->clifd);

			_sesspool_unlock(sp);
			return -1;
		}
		
		if (SESSPOOL_IS_LOCK(sp->flags))
			assert(session->owner == pthread_self());

		session->clifd = clifd;
		_sesspool_fdmap_add(sp->map, sp->size, svrid, clifd, svrfd);
	}
	
	_sesspool_unlock(sp);

	return 0;
}

/**
 *	Find a session and return it in session pool@sp acording
 *	session's socket fd @fd. If LOCK is set in @sp->flags, 
 *	only the thread which create this session can find it, 
 *	the other thread should not find it and use it.
 *
 *	Return session if find, NULL if not find or error.
 */
session_t *
sesspool_find(sesspool_t *sp, int fd)
{
	int id;
	session_t *session;
	_sesspool_hash_t *entries, *hash;
	int hkey;

	if (!sp || fd < 0)
		return NULL;

	entries = sp->hash;

	_sesspool_lock(sp);

	id = _sesspool_fdmap_find(sp->map, sp->size, fd);
	if (id < 0) {
		_SESSPOOL_ERR("can't find fd %d in session pool\n", fd);

		_sesspool_unlock(sp);
		return NULL;
	}

	hkey = _SESSPOOL_HKEY(id);
	hash = &(entries[hkey]);

	session = _sesspool_hash_find(hash, id);
	assert(session);

	_sesspool_unlock(sp);

	return session;
}

/**
 *	Print session pool @sp to stdout.
 *
 *	No return.
 */
void
sesspool_print(sesspool_t *sp)
{
	_sesspool_fdmap_t *map = NULL;
	_sesspool_hash_t *hash = NULL;
	session_t *session = NULL;
	int i;

	if (!sp)
		return;
	
	_sesspool_lock(sp);

	printf("sesspool(%p): size %d, nused %d, pool %p, "
	       "hash %p, map %p, freeid %d, flags %x\n",
	       sp, sp->size, sp->nused, sp->pool, sp->hash,
	       sp->map, sp->freeid, sp->flags);

	printf("    map:\n");
	map = sp->map;
	for (i = 0; i < sp->size; i++) {
		printf("\t%d: id %d, clifd %d, svrfd %d\n", 
		       i, map[i].id, map[i].clifd, map[i].svrfd);
	}

	hash = sp->hash;
	for (i = 0; i <= SESSPOOL_HSIZE; i++) {
		printf("    %d: hash(%p): size %d, head %p\n",
		       i, hash, hash[i].size, hash[i].head);
		session = hash[i].head;
		while(session) {
			printf("\tsession(%p), prev %p, next %p, "
			       "id %d, flags 0x%x, clifd %d, svrfd %d, "
			       "sip %u.%u.%u.%u, sport %u, "
			       "dip %u.%u.%u.%u, dport %u, "
			       "owner %lu\n", 
			       session, session->prev, session->next,
			       session->id, session->flags, 
			       session->clifd, session->svrfd, 
			       NIPQUAD(session->sip), session->sport,
			       NIPQUAD(session->dip), session->dport,
			       session->owner);

			session = session->next;
		}
	}
	_sesspool_unlock(sp);
}


/**
 *	Put a packet @p to end of packet queue @q
 *
 *	Return 0 if success, -1 on error.
 */
int 
pktqueue_in(pktqueue_t *q, packet_t *pkt)
{
	if (!q || !pkt)
		return -1;

	pkt->next = NULL;
	if (q->size)
		q->tail->next = pkt;
	else
		q->head = pkt;

	q->tail = pkt;
	q->size++;

	return 0;
}


/**
 *	Get a packet from head of packet queue @q.
 *
 *	Return pointer to packet if success, NULL on error.
 */
packet_t *
pktqueue_out(pktqueue_t *q)
{
	packet_t *pkt;

	if (!q || !q->size)
		return NULL;
	
	pkt = q->head;
	q->head = pkt->next;
	if (q->size == 1)
		q->tail = NULL;
    
	q->size--;
	pkt->next = NULL;
	
	return pkt;
}



