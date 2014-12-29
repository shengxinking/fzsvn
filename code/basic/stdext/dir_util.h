/**
 *	@file	dir_util.h
 *
 *	@brief	directory APIs.
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_DIR_UTIL_H
#define FZ_DIR_UTIL_H

/**
 *	Remove all files/directories in directory @dir except 
 *	files/directory in string array @execptions.
 *
 *	Return if success, -1 on error.
 */
extern int 
dir_empty(const char *dir, const char *exceptions[]);

/**
 *	Create a new directory @dir, if the ancient path of @dir
 *	is not exist, also created. All new create directory mode
 *	is @mode.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
dir_create(const char *dir, int mode);

/**
 *	Remove directory @dir and files/directories in it.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
dir_remove(const char *dir);

/**
 *	Copy directory @srcdir and all files/directories in 
 *	it to directory @dstdir, if @dstdir not exist, 
 *	create it.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
dir_copy(const char *dstdir, const char *srcdir, int overwrite);

/**
 *	Get all files in directory @dir.
 *
 *	Need call str_free_array() returned string array.
 *
 *	Return string array of files if success, -1 on error.
 */
extern char ** 
dir_files(const char *dir);

/**
 *	Get all directories in directory @dir
 *
 *	Need call str_array_free() after use.
 * 
 *	Return string array of directories if success, -1 on error.
 */
extern char ** 
dir_dirs(const char *dir);



#endif /* end of FZ_DIR_UTIL_H */


