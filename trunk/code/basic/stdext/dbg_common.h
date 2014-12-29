/**
 *	@file	dbg_common.h
 *
 *	@brief	common debug output macro.
 *	
 *	@author	Forrest.zhang
 *
 *	@date	2014-12-04
 */

#ifndef FZ_DBG_COMMON_H
#define FZ_DBG_COMMON_H

#include <stdio.h>
#include <string.h>
#include <errno.h>

/* ERRSTR for get errno string in glibc functions */
#ifndef	ERRSTR
#define	ERRSTR		strerror(errno)
#endif

/* not defined DBG_NOERR */
#ifndef	DBG_NOERR

/* ERR() for output error message */
#ifndef	ERR
#define	ERR(fmt, args...)			\
	fprintf(stderr, "<%s:%d> "fmt,		\
		__FILE__, __LINE__, ##args);
#endif	/* end of ERR */

/* ERR_RET() output error message and return from function */
#ifndef	ERR_RET
#define	ERR_RET(ret, fmt, args...)		\
({						\
	fprintf(stderr, "<%s:%d> "fmt,		\
		__FILE__, __LINE__, ##args);	\
 	return ret;				\
})
#endif	/* end of ERR_RET */

/* defined DBG_NOERR, not output error message */
#else

/* ERR() is empty macro */
#ifndef	ERR
#define	ERR(fmt, args...)
#endif

/* ERR_RET() just return from function */
#ifndef	ERR_RET
#define	ERR_RET(ret, fmt, args...)		\
({						\
	return ret;				\
})
#endif	/* end of ERR_RET */

#endif	/* end of DBG_NOERR */


#endif /* end of FZ_DBG_COMMON_H */


