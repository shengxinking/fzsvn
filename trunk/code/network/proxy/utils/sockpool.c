/*
 * @file        sockpool.c
 * @brief       socket pool implement, it's use for server socket
 *
 * @author      FZ
 */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <assert.h>

#include "sock.h"
#include "sockpool.h"

#define _SOCKPOOL_DBG

#define _SOCKPOOL_BIT_USED	0
#define _SOCKPOOL_USED		(1 << _SOCKPOOL_BIT_USED)

#define _SOCKPOOL_IS_USED(flags)	(flags & _SOCKPOOL_USED)
#define _SOCKPOOL_SET_USED(flags)	(flags |= _SOCKPOOL_USED)
#define _SOCKPOOL_CLR_USED(flags)	(flags &= ~_SOCKPOOL_USED)

typedef struct _sockpool_item {
	struct _sockpool_item	*prev;
	struct _sockpool_item	*next;
	int             	fd;
	u_int32_t       	nused;
	u_int16_t               flags;

	u_int16_t       	port;
	u_int32_t       	ip;
} _sockpool_item_t;


typedef struct _sockpool_hash {
	u_int32_t               size;
	_sockpool_item_t         *head;
} _sockpool_hash_t;


/* debug function using in sockpool */
#ifdef _SOCKPOOL_DBG
#define _SOCKPOOL_ERR(fmt, args...)	fprintf(stderr, "sockpool:%s:%d: "fmt, \
						__FILE__, __LINE__, ##args)
#else
#define _SOCKPOOL_ERR(fmt, args...)
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
 *	Return the hash key acording IP and port.
 *	We only add all 8-bit in IP and port. In most local network, all
 *	machine only different in last 8-bit, so we using 8-bit hash key
 *	and can give each IP a unique hash key.
 *
 *	Return 8-bit hash key.
 */
static u_int8_t 
_sockpool_keygen(u_int32_t ip, u_int16_t port)
{
	u_int32_t key;

	key = (u_int8_t)((ip >> 24) & 0xff);
	key += (u_int8_t)((ip >> 16) & 0xff);
	key += (u_int8_t)((ip >> 8) & 0xff);
	key += (u_int8_t)((ip) & 0xff);
	key += (u_int8_t)((port >> 8) & 0xff);
	key += (u_int8_t)((port) & 0xff);

	return (u_int8_t)(key % SOCKPOOL_HSIZE);
}


/**
 *	Test the socket @fd is clear, it means no data in this 
 *	socket need read, or it's not closed by peer.
 *
 *	Return 0 if fd can reuse, -1 means need close.
 */
static int 
_sockpool_testfd(int fd)
{
	char buf[1];
	int ret = 0;
	
	assert(fd >= 0);

	ret = recv(fd, buf, 1, MSG_DONTWAIT);
	if (ret < 0 && errno == EAGAIN)
		return 0;
	else
		return -1;
}

/**
 *	Print the _sockpool_item @item to stdout. It's used for debug
 *
 *	No return.
 */
static void 
_sockpool_item_print(_sockpool_item_t *item)
{
	assert(item);
	
	printf("\titem(%p): prev %p, next %p, fd %d, nused %u, "
	       "flags %x, ip %d.%d.%d.%d, port %d\n",
	       item, item->prev, item->next, item->fd, item->nused,
	       item->flags, NIPQUAD(item->ip), item->port);
}


/**
 *	Get a _sockpool_item from hash entry @entry. It remove first item
 *	in double list @entry->head and return it. The last hash entry is
 *	store freed sockpool_item object, so other functions need a freed
 *	_sockpool_item, it'll call this function, the @entry is a pointer 
 *	to last entry.
 *
 *	Return an _sockpool_item if success, NULL on error.
 */
static _sockpool_item_t *
_sockpool_hash_get(_sockpool_hash_t *entry)
{
	_sockpool_item_t *item = NULL;

	assert(entry);

	if (entry->size) {
		item = entry->head;
		if (item) {
			entry->head = item->next;
		}
		if (entry->head) {
			entry->head->prev = NULL;
		}
		entry->size--;
	}
	else {
		_SOCKPOOL_ERR("can't get a freed _sockpool_item");
	}

	return item;
}

/**
 *	Add a _sockpool_item @item to hash entry @entry's first position.
 *
 *	No return.
 */
static void 
_sockpool_hash_add(_sockpool_hash_t *entry,
		   _sockpool_item_t *item)
{
	assert(entry);
	assert(item);
	
	if (entry->head) {
		item->next = entry->head;
		entry->head->prev = item;
	}
	else {
		item->next = NULL;
	}

	entry->head = item;
	item->prev = NULL;

	entry->size++;
}

/**
 *	Remove a _sockpool_item @item from hash entry @entry.
 *
 *	No return.
 */
static void 
_sockpool_hash_del(_sockpool_hash_t *entry, 
		   _sockpool_item_t *item)
{
	assert(entry);
	assert(item);
	
	if (item->prev) {
		item->prev->next = item->next;
	}
	else {
		entry->head = item->next;
	}

	if (item->next) {
		item->next->prev = item->prev;
	}
			
	entry->size--;
}

/**
 *	Find a freed socket from hash entry @entry.
 *	if find it, then test the fd, if test result is
 *	success, return this item, else remove this item
 *	from hash entry @entry and continue.
 *
 *	Return _sockpool_item if find success, NULL if 
 *	not found.
 */
static _sockpool_item_t *
_sockpool_hash_find_free(sockpool_t *pool,
		    _sockpool_hash_t *entries, 
		    _sockpool_hash_t *entry)
{
	_sockpool_item_t *item;
	_sockpool_item_t *next;
	int fd;

	assert(pool);
	assert(entries);
	assert(entry);

	/* try get a fd from pool */
	item = entry->head;
	while (item) {
		assert(item->fd >= 0);

		if (_SOCKPOOL_IS_USED(item->flags)) {
			item = item->next;
			continue;
		}
		
		fd = item->fd;
		
		/* test socket failed */
		if (_sockpool_testfd(fd)) {
			next = item->next;

			/* delete item from this hash entry */
			_sockpool_hash_del(entry, item);

			/* add this item to free hash entry */
			_sockpool_hash_add(&(entries[SOCKPOOL_HSIZE]), item);

			pool->nused--;
			_SOCKPOOL_ERR("socket %d test failed\n", fd);
			close(fd);

			item = next;
			continue;
		} 
		else {
			return item;
		}
	}

	return NULL;
}


/**
 *	Find an _sockpool_item in _sockpool_hash @entry which fd, ip and port all
 *	matched.
 *
 *	Return the matched _sockpool_item if success, NULL means not exist.
 */
static _sockpool_item_t *
_sockpool_hash_find(_sockpool_hash_t *entry,
		    int fd,
		    u_int32_t ip,
		    u_int16_t port)
{
	_sockpool_item_t *item;

	assert(entry);
	assert(fd >= 0);

	item = entry->head;
	while (item) {
		assert(item->fd >= 0);
		
		if (item->fd == fd) {
			assert(item->ip == ip);
			assert(item->port == port);

			return item;
		}
		item = item->next;
	}

	return NULL;
}


/**
 *	Check the fd @fd is unique(include not exist) in hash entry @entry
 *
 *	Return 0 if fd is unique, -1 means not unique.
 */
static int 
_sockpool_hash_unique(_sockpool_hash_t *entry, int fd)
{
	_sockpool_item_t *item;
	int count = 0;

	assert(entry);
	assert(fd >= 0);

	item = entry->head;
	while (item) {
		assert(item->fd >= 0);

		if (item->fd == fd)
			count++;

		item = item->next;
	}

	if (count > 1)
		return -1;
	else
		return 0;
}

/**
 *	Lock the socket pool if the LOCK bit is set.
 *
 *	No return.
 */
static void _sockpool_lock(sockpool_t *pool)
{
	assert(pool);

	if (SOCKPOOL_IS_LOCK(pool->flags))
		pthread_mutex_lock(&(pool->lock));
}

/**
 *	Unlock the socket pool if the LOCK bit is set.
 *
 *	No return.
 */
static void _sockpool_unlock(sockpool_t *pool)
{
	assert(pool);

	if (SOCKPOOL_IS_LOCK(pool->flags))
		pthread_mutex_unlock(&(pool->lock));
}


/**
 *	Alloc a new socket pool, which can contain @siz socket
 *	in pool. If locked is not zero, set LOCK bit in pool's 
 *	flags and all API is thread-safe.
 *
 *	Return socket pool if success, NULL on error
 */
sockpool_t *sockpool_alloc(size_t size, int locked)
{
	_sockpool_hash_t *entries = NULL;
	_sockpool_item_t *items = NULL;
	sockpool_t *pool = NULL;
	int i;

	if (size < 1)
		return NULL;
	
	pool = malloc(sizeof(sockpool_t));
	if (!pool) {
		_SOCKPOOL_ERR("malloc sockpool(%u) error\n", size);
		return NULL;
	}
	pool->memsize = sizeof(sockpool_t);

	/* alloc memory for sockpool_item, it's used to 
	 * store socket fd. 
	 */
	pool->pool = malloc(sizeof(_sockpool_item_t) * size);
	if (!pool->pool) {
		_SOCKPOOL_ERR("alloc memory for sockpool_item error\n");
		free(pool);
		return NULL;
	}
	items = pool->pool;
	for (i = 0; i < size; i++) {
		items[i].fd = -1;
		items[i].ip = 0;
		items[i].port = 0;
		items[i].flags = 0;
		items[i].nused = 0;

		if (i == 0) {
			items[i].prev = NULL;
		}
		else {
			items[i].prev = &(items[i - 1]);
		}
		if (i == size - 1)
			items[i].next = NULL;
		else
			items[i].next = &(items[i + 1]);
	}
	pool->memsize += sizeof(_sockpool_item_t) * size;

	/* alloc memory for sockpool_hash entry.
	 * NOTE: the last entry pool->hash[SOCKPOOL_HSIZE] is used to store 
	 * freed sockpool_item.
	 */
	pool->hash = malloc(sizeof(_sockpool_hash_t) * (SOCKPOOL_HSIZE + 1));
	if (!pool->hash) {
		_SOCKPOOL_ERR("alloc memory for sockpool_hash error\n");
		free(pool->pool);
		free(pool);
		return NULL;
	}
	entries = pool->hash;
	for (i = 0; i < SOCKPOOL_HSIZE; i++) {
		entries[i].size = 0;
		entries[i].head = NULL;
	}
	entries[SOCKPOOL_HSIZE].size = size;
	entries[SOCKPOOL_HSIZE].head = &(items[0]);

	pool->memsize += sizeof(_sockpool_hash_t) * (SOCKPOOL_HSIZE + 1);

	pool->size = size;
	pool->nused = 0;
	pool->flags = 0;
	pool->success = 0;
	pool->failed = 0;

	/* init lock if argument @locked is non zero */
	if (locked) {
		SOCKPOOL_SET_LOCK(pool->flags);
		pthread_mutex_init(&pool->lock, NULL);
	}
	
	return pool;
}

/**
 *	Free socket pool @pool which is alloced using sockpool_alloc.
 *
 *	No return.
 */
void sockpool_free(sockpool_t *pool)
{
	int i;
	_sockpool_item_t *item;
	_sockpool_hash_t *hash;
	_sockpool_hash_t *entries;

	if (!pool)
		return;

	if (pool->hash) {
		entries = pool->hash;
		
		for (i = 0; i < SOCKPOOL_HSIZE; i++) { 
			hash = &entries[i];
			item = hash->head;
			while (item) {
				assert(item->fd >= 0);
				close(item->fd);

				item = item->next;
			}
		}

		free(pool->hash);
	}

	if (pool->pool)
		free(pool->pool);

	free(pool);
}


/**
 *	Get a socket fd from socket pool @pool, the returned socket
 *	is connected to address @ip:port.
 *
 *	Return socket fd if >=0, -1 on error.
 */
int sockpool_get(sockpool_t *pool, u_int32_t ip, u_int16_t port)
{
	_sockpool_item_t *item = NULL;
	_sockpool_hash_t *entry = NULL, *entries;
	u_int8_t key;
	int fd;

	if (!pool || ip < 1 || port < 1)
		return -1;

	/* find a sockpool_hash entry, it's only get the entry address, 
	 * and not need locked 
	 */
	key = _sockpool_keygen(ip, port);
	entries = pool->hash;
	entry = &entries[key];

	_sockpool_lock(pool);
	
	/* find a free sockpool_item from hash entry */
	item = _sockpool_hash_find_free(pool, entries, entry);
	if (item) {
		item->nused++;
		fd = item->fd;
		
		if (_sockpool_hash_unique(entry, fd)) {
			_SOCKPOOL_ERR("fd %d is not unique in hash\n", fd);
		}

		pool->success++;

		_SOCKPOOL_SET_USED(item->flags);		
		_sockpool_unlock(pool);

		return fd;
	}
	
	/* test pool's capacity */
	if (pool->nused >= pool->size) {
		pool->failed++;
		_SOCKPOOL_ERR("can't alloc more fd\n");
		_sockpool_unlock(pool);
		return -1;
	}

	fd =sock_tcpcli(ip, port, 0, 1);
	if (fd < 0) {
		pool->failed++;
		_sockpool_unlock(pool);
		_SOCKPOOL_ERR("can't connect to %d.%d.%d.%d:%d\n",
			      NIPQUAD(ip), port);
		return -1;
	}
	
	item = _sockpool_hash_get(&(entries[SOCKPOOL_HSIZE]));
	if (!item) {
		pool->failed++;
		_SOCKPOOL_ERR("can't get item from pool\n");
		_sockpool_unlock(pool);
		close(fd);
		return -1;
	}

	/* get a free node */
	item->flags = 0;
	item->fd = fd;
	item->nused = 1;
	item->ip = ip;
	item->port = port;
	_SOCKPOOL_SET_USED(item->flags);

	_sockpool_hash_add(entry, item);

	pool->nused++;
	pool->success++;

	_sockpool_unlock(pool);

	return fd;
}


/**
 *	Put a socket into socket pool, the returned socket is one which 
 *	get by @sockpool_get. If isclose is not zero, this socket will
 *	close and not reuse.
 *
 *	Return 0 if success, -1 on error.
 */
int 
sockpool_put(sockpool_t *pool, int fd, int isclose)
{
	u_int8_t key;
	_sockpool_hash_t *entry, *entries;
	_sockpool_item_t *item;
	struct sockaddr_in addr;
	socklen_t addrlen;
	u_int32_t ip;
	u_int16_t port;

	if (!pool || fd < 0)
		return -1;

	/* get IP and port peer */
	addrlen = sizeof(addr);

	if (getpeername(fd, (struct sockaddr *)&addr, &addrlen)) {
		_SOCKPOOL_ERR("getsockname(%d) error: %s\n", fd, strerror(errno));
		return -1;
	}
	ip = addr.sin_addr.s_addr;
	port = ntohs(addr.sin_port);

	/* find the hash entry */
	key = _sockpool_keygen(ip, port);
	entries = pool->hash;
	entry = &(entries[key]);

	_sockpool_lock(pool);

	item = _sockpool_hash_find(entry, fd, ip, port);
	if (!item) {
		_SOCKPOOL_ERR("can't find fd %d in pool\n", fd);
		_sockpool_unlock(pool);

		return -1;
	}

	if (!isclose) {
		_SOCKPOOL_CLR_USED(item->flags);
	}
	else {			
		_sockpool_hash_del(entry, item);
		_sockpool_hash_add(&(entries[SOCKPOOL_HSIZE]), item);
		
		pool->nused--;
		close(fd);
	}

	_sockpool_unlock(pool);
	return 0;
}

/**
 *	Print socket pool information, it's used for debug.
 *
 *	No return.
 */
void sockpool_print(const sockpool_t *pool)
{
	_sockpool_item_t *item;
	_sockpool_hash_t *entry, *entries;
	int i;

	assert(pool);
	
	printf("sockpool(%p): size %u, flags %x, nused %u, success %u, failed %u"
	       "pool %p, hash %p\n",
	       pool, pool->size, pool->flags, pool->nused, pool->success, pool->failed,
	       pool->pool, pool->hash);

	entries = pool->hash;
	for (i = 0; i < SOCKPOOL_HSIZE + 1; i++) {
		entry = &entries[i];
		printf("    %d hash(%p): size %u, head %p\n", 
		       i, entry, entry->size, entry->head);
		item = entry->head;
		while (item) {
			_sockpool_item_print(item);
			item = item->next;
		}
	}
}



