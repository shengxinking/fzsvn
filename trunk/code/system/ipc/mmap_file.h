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

#define	MMAP_CTLFILE	"ctlfile"
#define	MMAP_INVALID_ID	-1
#define	MMAP_NAMELEN	16		/* the name length */
#define	MMAP_MAX_OBJ	1024		/* max objects in file */
#define	MMAP_INC_OBJ	10		/* the increase object number */
#define	MMAP_MIN_OBJSIZE 64		/* the mimum object size */	

typedef	int		mmap_oid_t	/* the object id */
typedef	short		mmap_fid_t	/* the file id */

/**
 *	The object control object 	
 *
 */
typedef mmap_obj_ctl {
	size_t		objsize;	/* the object length */
	size_t		nfreed;		/* number of freed objects */
	int		max;		/* the max objects in file */
	u_int8_t	map[MMAP_MAX_OBJ];/* the max object in file */
} mmap_obj_ctl_t;

/**
 *	the file of mmap objects.
 */
typedef mmap_file_ctl {
	mmap_fid_t	fid;		/* the file id */
	void		*mptr;		/* mapped address */
	size_t		len;		/* the file length */
	mmap_obj_ctl_t	objctl;		/* the object control data */
} mmap_file_ctl_t;

/**
 * 	the mmap control object
 */
typedef	mmap_ctl {
	size_t		gap;		/* the object size gap */
	mmap_fid_t	maxid;		/* the max id of mapped files */
	char		dir[MMAP_NAMELEN];/* the directory of mmap files */
	int		nfile;		/* array @files number */
	mmap_file_ctl_t	files[0];	/* mmaped files, sorted by objsize */
} mmap_ctl_t;

extern mmap_ctl_t * 
mmap_create(const char *dir);

extern int 
mmap_destroy(mmap_ctl_t *ctl);

extern int 
mmap_load(const char *dir);

extern mmap_id_t 
mmap_alloc(size_t size);

extern void * 
mmap_id2ptr(mmap_id_t id);

extern int 
mmap_ptr2id(void *ptr);

extern int  
mmap_free(mmap_id_t id);

extern int 
mmap_lock(void);

extern int 
mmap_unlock(void);

#endif /* end of FZ_MMAP_SHM  */

