/*
 *  @file       backup.h
 *  @brief      backup some files to one file, and restore files from backuped file
 *
 *  @date       2007-10-16
 */

#ifndef FS_BACKUP_H
#define FS_BACKUP_H

#include <sys/types.h>

#ifndef PATHLEN
#define PATHLEN                 256
#endif

typedef struct backup_file {
	struct backup_file      *next;
	char                    path[PATHLEN];
	int                     size;
	char                    *data;
} backup_file_t;


typedef struct backup {
	u_int32_t               magic;
	u_int32_t               size;
	backup_file_t           *files;
} backup_t;


extern backup_t *backup_alloc(u_int32_t magic);

extern void backup_free(backup_t *bp);

extern int backup_add(backup_t *bp, const char *file);

extern int backup_del(backup_t *bp, const char *file);

extern int backup_read(backup_t *bp, const char *file);

extern int backup_write(backup_t *bp, const char *file);

extern int backup_restore(backup_t *bp);

extern void backup_print(const backup_t *bp)

#endif /* end of FS_BACKUP_H */



