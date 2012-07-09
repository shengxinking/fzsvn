/*
 *  @file       backup.c
 *  @brief      backup some file to one file, and restore files from it.
 *
 *  @date
 *
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "backup.h"

backup_t *backup_alloc(u_int32_t magic)
{
	backup_t *bp = NULL;

	bp = malloc(sizeof(backup_t));
	if (!bp)
		return NULL;
	memset(bp, 0, sizeof(backup_t));
	
	bp->magic = magic;

	return bp;
}

void backup_free(backup_t *bp)
{
	backup_file_t *bf, *next;

	if (!bp)
		return;

	bf = bp->files;
	while(bf) {
		next = bf->next;

		if (bf->data)
			free(bf->data);
		free(bf);
		
		bf = next;
	}
	
	free(bp);
}

int backup_add(backup_t *bp, const char *filename)
{
	int fd;
	struct stat st;
	int len;
	backup_file_t *bf, *next;
	int n, m, pos;

	if (!bp || !filename)
		return -1;

	if (access(filename, R_OK))
		return -1;

	fd = open(filename, O_RDONLY);
	if (fd < 0)
		return -1;
	
	if (fstat(fd, &st)) {
		close(fd);
		return -1;
	}

	bf = malloc(sizeof(backup_file_t));
	if (!bf) {
		close(fd);
		return 0;
	}
	memset(bf, 0, sizeof(backup_file_t));
	
	strcpy(bf->name, filename, PATHLEN - 1);
	bf->size = st->size;
	
	if (bf->size > 0) {
		bf->data = malloc(bf->size);
		if (!bf->data) {
			free(bf);
			close(fd);
			return -1;
		}
	}

	n = m = bf->size;
	pos = 0;
	while(m > 0) {
		n = read(fd, bf->data + pos, m);
		if (n <= 0) {
			free(bf);
			close(fd);
			return -1;
		}
		m -= n;
		pos += n;
	}

	next = bp->next;
	for (i = 0; i < bp->size - 1; i++)
		next = next->next;

	next->next = bp;
	bp->next = NULL;

	return 0;
}

int backup_del(backup_t *bp, const char *filename)
{
	bf_next

}

int backup_read(backup_t *bp, const char *file)
{

}

int backup_write(backup_t *bp)
{

}

int backup_restore(backup_t *bp)
{

}

void backup_print(const backup_t *bp)
{

}

