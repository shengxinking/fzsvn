/**
 *	@file	debug.h
 *
 *	@brief	the debug functions
 *
 *	@author	forrest.zhang
 *
 *	@date	2009-09-09
 */

#ifndef FZ_DEBUG_H
#define FZ_DEBUG_H

#include <stdio.h>
#include <string.h>
#include <errno.h>

/* the debug on/off macro */
//#define PROXY_DEBUG		1

/* enable debug */
#ifdef PROXY_DEBUG

#define FLOW(level, fmt, args...)			\
	if (level <= g_proxy.dbglvl)			\
		printf(fmt, ##args);   

/* print debug message */
#define DBG(fmt, args...)	printf("proxyd: "fmt, ##args)

/* print buffer content */
#define DBGBUF(buf, size, fmt, args...) \
	printf("[buf]: "fmt, ##args);	\
	do {				\
		write(0, buf, size);	\
		fsync(0);		\
		printf("\n\n");		\
	} while (0);

/* disable debug */
#else

#define	FLOW(level, fmt, args...)
#define DBG(fmt, args...)
#define DBGBUF(buf, size, fmt, args...)

#endif

#define ERR(fmt, args...)			\
	fprintf(stderr, "[ERR]:%s:%d: " fmt,	\
		__FILE__, __LINE__, ##args)
#define	ERRSTR		strerror(errno)

/* likely()/unlikely() for performance */
#ifndef unlikely
#define unlikely(x)  __builtin_expect(!!(x), 0)
#endif

#ifndef likely
#define likely(x)    __builtin_expect(!!(x), 1)
#endif

#endif /* end of FZ_DEBUG_H */

