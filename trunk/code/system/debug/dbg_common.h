/**
 *	@file	dbg_common.h
 *
 *	@brief	common debug APIs
 *	
 *	@author	Forrest.zhang
 *
 *	@date	2014-06-14
 */

#ifndef FZ_DBG_COMMON_H
#define FZ_DBG_COMMON_H

#define	ERR(fmt, args...)				\
	fprintf(stderr, "<%s:%d> "fmt, 			\
		__FILE__, __LINE__, ##args)


#define	ERR_RET(ret, fmt, args...)			\
({							\
	fprintf(stderr, "<%s:%d> "fmt, 			\
		__FILE__, __LINE__, ##args);		\
 	return ret;					\
 })


#endif /* end of FZ_DBG_COMMON_H */


