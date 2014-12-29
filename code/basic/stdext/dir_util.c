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
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stddef.h>
#include <errno.h>

#include "gcc_common.h"
#include "str_util.h"
#include "dir_util.h"


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
	const char *str;
	size_t slen;
	int i;

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
		slen = strlen(str) + 1;
		if (str_cmp(d->d_name, dlen, str, slen) == 0)
			return 1;
	}
	
	return 0;
}

int 
_dir_empty(const char *dir, const char **exceptions, int remove_self)
{
	struct dirent *d, *dres = NULL;
	DIR *dfp;
	size_t len;
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
		if (S_ISDIR(st.st_mode)) {
			_dir_empty(fname, NULL, 1);
		}
		/* dirent */
		else if (S_ISDIR(st.st_mode)) {
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
dir_empty(const char *dir, const char *exceptions[])
{
	return _dir_empty(dir, exceptions, 0);
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
	char *end = NULL;
	char *str = NULL;
	char *dname;
	char cwd[512];
	int slen;
	struct stat st;
	int ret = 0;

	if (unlikely(!dir))
		return -1;

	/* save current work directory */
	if (getcwd(cwd, sizeof(cwd)) == NULL)
		return -1;

	/* get dir length */
	slen = strlen(dir) + 1;
	if (slen < 2)
		return -1;

	/* malloc memory to store dir */
	str = malloc(slen);
	if (!str)
		return -1;
	if (strncpy(str, dir, slen) < 0)
		return -1;

	end = str + slen;
	ptr = str;

	/* check is "/xxx/xxx" dir, chang working dir to "/" */
	while (*ptr == '/') {
		chdir("/");
		ptr++;
		slen--;
	}

	dname = ptr;
	slen = end - ptr;
	while ((ptr = str_chr(dname, slen, '/')) != NULL) {
		
		*ptr = 0;

		/* dir name is empty, like "//" skipped it */
		if (strlen(dname) == 0) {
			*ptr = '/';
			ptr++;
			dname = ptr;
			slen = end - ptr;
			continue;
		}

		ret = stat(dname, &st);
		if (ret == 0) {
			if (!S_ISDIR(st.st_mode)) {
				ret = -1;
				break;
			}
		}
		else {
			ret = mkdir(dname, mode);
			if (ret) {
				ret = -1;
				break;
			}
		}

		if (chdir(dname)) {
			ret = -1;
			break;;
		}
		
		*ptr = '/';
		ptr++;
		dname = ptr;
		slen = end - ptr;
	}

	if (str)
		free(str);

	/* restore current pwd */
	chdir(cwd);

	return ret;
}

char ** 
dir_files(const char *dir)
{
	struct dirent *d, *dres = NULL;
	DIR *dfp;
	size_t len;
	char fname[1024];
	struct stat st;
	char **files = NULL, **orig_files = NULL;
	int nfile = 0;

	if (unlikely(!dir))
		return NULL;

	if (access(dir, R_OK|W_OK))
		return NULL;

	dfp = opendir(dir);
	if (!dfp)
		return NULL;

	len = _dir_struct_len(dir);
	d = malloc(len);
	if (!d) {
		closedir(dfp);
		return NULL;
	}

	while (readdir_r(dfp, d, &dres) == 0) {

		/* the directory is end */
		if (dres == NULL)
			break;
		
		len = sizeof(fname) - 1;
		snprintf(fname, len, "%s/%s", dir, dres->d_name);

		/* get file type */
		if (stat(fname, &st)) {
			str_array_free(files);
			files = NULL;
			break;;
		}

		/* regular file */
		if (S_ISREG(st.st_mode)) {
			nfile++;
			/* alloc string array */
			len = sizeof(char *) * (nfile + 1);
			files = realloc(orig_files, len);
			if (!files) {
				str_array_free(orig_files);
				break;;
			}
			files[nfile - 1] = strdup(dres->d_name);
			files[nfile] = NULL;
			orig_files = files;
		}
	} 

	closedir(dfp);
	if (d)
		free(d);

	return files;
}


char ** 
dir_dirs(const char *dir)
{
	struct dirent *d, *dres = NULL;
	DIR *dfp;
	size_t len;
	char fname[1024];
	struct stat st;
	char **dirs = NULL, **orig_dirs = NULL;
	int ndir = 0;

	if (unlikely(!dir))
		return NULL;

	if (access(dir, R_OK|W_OK))
		return NULL;

	dfp = opendir(dir);
	if (!dfp)
		return NULL;

	len = _dir_struct_len(dir);
	d = malloc(len);
	if (!d) {
		closedir(dfp);
		return NULL;
	}

	while (readdir_r(dfp, d, &dres) == 0) {

		/* the directory is end */
		if (dres == NULL)
			break;
		
		len = sizeof(fname) - 1;
		snprintf(fname, len, "%s/%s", dir, dres->d_name);

		/* get file type */
		if (stat(fname, &st)) {
			str_array_free(dirs);
			dirs = NULL;
			break;;
		}

		/* regular file */
		if (S_ISDIR(st.st_mode)) {
			ndir++;
			/* alloc string array */
			len = sizeof(char *) * (ndir + 1);
			dirs = realloc(orig_dirs, len);
			if (!dirs) {
				str_array_free(orig_dirs);
				break;;
			}
			dirs[ndir - 1] = strdup(dres->d_name);
			dirs[ndir] = NULL;
			orig_dirs = dirs;
		}
	} 

	closedir(dfp);
	if (d)
		free(d);

	return dirs;
}


int 
dir_copy(const char *src, const char *dst, int overwrite)
{
	if (unlikely(!src || !dst))
		return -1;
	
	return 0;
}


int 
dir_rename(const char *src, const char *dst, int overwrite)
{
	if (unlikely(!src || !dst))
		return -1;


	return 0;
}


