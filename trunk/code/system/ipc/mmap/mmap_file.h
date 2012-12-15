/**
 *	@file
 *
 *	@brief
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_MMAP_SHM_H
#define FZ_MMAP_SHM_H

#define	MMAP_DIR		"/var/mmap"
#define	MMAP_CTLFILE		MMAPDIR"/ctlfile"
#define	MMAP_INVALID_ID		-1

typedef	int	mmap_id_t

typedef	mmap_ctl {
	mmap_id_t	id;		/* mmaped id */
	void		*mptr;		/* mmaped ptr */
} mmap_ctl_t

extern int 
mmap_create(void);

extern int 
mmap_destroy(void);

extern int 
mmap_reload(void);

extern mmap_id_t 
mmap_alloc(size_t size);

extern int 
mmap_free(mmap_id_t id);

extern int 
mmap_lock(void);

extern int 
mmap_unlock(void);


#endif /* end of FZ_MMAP_SHM  */

