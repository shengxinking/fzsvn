/**
 *	@file	netproc.c
 *
 *	@brief	proc/sys/net file operator.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date
 */

/**
 *	Read integer from proc file @file and return it.
 *
 *	Return >= 0 if success, -1 on error.
 */
static int 
_netp_read_int(const char *file)
{
	
}

/**
 *	Write integer into proc file @file.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_netp_write_int(const char *file, int val)
{

}

/**
 *	Read 2 integer from proc file @file and stored in @val1, @val2.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_netp_read_2_int(const char *file, int *val1, int *val2)
{

}

/**
 *	Write 2 integer @val1 @val2 into proc file @file.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_netp_write_2_int(const char *file, int val1, int val2)
{

}

/**
 *	Read string from proc file @file and saved in @buf, 
 *	@buf length is @len.
 *
 *	Return 0 if success, -1 on error.
 */
static int  
_netp_read_str(const char *file, char *buf, size_t len)
{

}

/**
 *	Write string @buf into proc file @file, the @buf length 
 *	is @len
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_netp_write_str(const char *file, const char *buf, size_t len)
{

}



