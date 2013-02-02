/**
 *	@file	dir_func.c
 *
 *	@brief	The directory handler function.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2013-02-02
 */

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <stddef.h>

static size_t 
_dir_struct_len(const char *dir)
{
	size_t len;
	long namelen;

	namelen = pathconf(dir, _PC_NAME_MAX);
	if (namelen < 0)
		namelen = 255;

	len = offsetof(struct dirent, d_name) + namelen;

	return len;
}

/**
 *	Check the file is skipped or not.
 *
 *	Return 1 if need skipped, 0 mean not skipped.
 */
static int 
_dir_filter(struct dirent *d, const char **exceptions)
{
	size_t dlen;
	char *str;
	size_t slen;

	dlen = sizeof(d->d_name);

	/* skipped "." */
	if (str_cmp(d->d_name, dlen, ".", 1) == 0)
		return 1;
	
	/* skipped ".." */
	if (str_cmp(d->d_name, dlen, "..", 2) == 0)
		return 1;

	if (!exceptions)
		return 0;

	while ((str = exceptions[i++]) != NULL) {
		slen = strlen(str);
		if (str_cmp(d->d_name, dlen, str, slen) == 0)
			return 1;
	}
	
	return 0;
}

int 
_dir_empty(const char *dir, const char **esceptions, int remove_self)
{
	struct dirent *d, *dres = NULL;
	DIR *dfp;
	size_t len;
	size_t dlen;
	struct stat st;
	int ret = 0;
	char fname[1024];

	if (unlikely(!dir))
		return -1;

	if (access(dir, R_OK|W_OK))
		return -1;

	dfp = opendir(dir);
	if (!dfp)
		return -1;

	len = _dir_struct_len(dir);
	d = malloc(len);
	if (!d) {
		closedir(dfp);
		return -1;
	}

	while (readdir_r(dfp, d, &dres) == 0) {

		/* the directory is end */
		if (dres == NULL)
			break;
		
		if (_dir_filter(d, exceptions))
			continue;
		
		snprintf(fname, sizeof(fname), "%s/%s", dir, d->d_name);

		/* get file type */
		if (stat(fname, &st)) {
			ret = -1;
			break;
		}

		/* regular file */
		if (S_IS_DIR(st.st_mode)) {
			_dir_empty(fname, NULL, 1);
		}
		/* dirent */
		else if (S_IS_DIR(st.st_mode)) {
			unlink(fname);
		}
	} 

	closedir(dfp);

	if (remove_self)
		unlink(dir);

	if (d)
		free(d);

	return ret;
}

int 
dir_empty(const char *dir, const char **exceptions)
{
	return _dir_empty(dir, 0);
}

int 
dir_remove(const char *dir)
{
	return _dir_empty(dir, NULL, 1);
}

int 
dir_create(const char *dir, int mode)
{
	char *ptr;
	char *str;
	size_t dlen;
	struct stat st;
	int ret = 0;

	if (unlikely(!dir))
		return -1;

	/* save directory name */
	dlen = strlen(dir);
	str = malloc(dlen + 1);
	memcpy(str, dir, dlen);
	str[dlen] = 0;

	ptr = str;
	while ((ptr = str_chr(ptr, '/')) != NULL) {
		*ptr = 0;

		ret = stat(str, &st);
		if (ret == 0) {
			if (!S_IS_DIR(st.st_mode)) { 
				ret = -1;
				break;
			}
		}
		else {
			ret = mkdir(str, mode);
			if (ret)
				break;
		}
		
		*ptr = '/';
		ptr++;
	}

	if (str)
		free(str);

	return ret;
}
