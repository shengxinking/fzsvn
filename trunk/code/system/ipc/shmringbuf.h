/*
 *	@file	shmringbuf.h
 *
 *	@brief	A simple ring buffer using BSD share memory.
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */


#ifndef FZ_SHMRINGBUF_H
#define FZ_SHMRINGBUF_H

#include <sys/ipc.h>
#include <sys/shm.h>

typedef shmrb_stat {
	u_int32_t	full_times;
	u_int32_t	free_times;
} shmrb_stat_t;

typedef shmrb {
	void		*ptr;
	key_t		key;
	int		shmid;
	int		semid;
	int		size;

	shmrb_stat_t	stat;
} shmrb_t;

typedef shmrb_hdr {
	u_int32_t	free_start;
	u_int32_t	free_end;
	u_int32_t	used_start;
	u_int32_t	used_end;
}shmrb_hdr_t;

/**
 *	Alloc a ring buffer in share memory, the key is @key, the 
 *	ring buffer total size is size.
 *
 *	Return the pointer ring buffer if success, NULL on error.
 */
extern shmrb_t *
shmrb_alloc(key_t key, size_t size);


/**
 *	Free a ring buffer which is alloced using shmrb_alloc().
 *
 *	No return.
 */
extern void
shmrb_free(shmrb_t *rb);


/**
 *	Copy buffer @ptr to ring buffer @rb, the buffer @ptr's 
 *	size is @size.
 *
 *	Return 0 if success, -1 on error.
 */
extern int
shmrb_append(shmrb_t *rb, void *ptr, size_t size);


/**
 *	Free a buffer in ring buffer @rb which pointer is @ptr.
 *
 *	Return 0 if success, -1 on error.
 */
extern int
shmrb_remove(shmrb_t *rb, void *ptr);


/**
 *	Print the ring buffer's content.
 *
 *	No return.
 */
extern void
shmrb_print(shmrb_t *rb);


#endif /* end of FZ_SHMRINGBUF_H */

