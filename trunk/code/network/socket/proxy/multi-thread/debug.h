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


//#define PROXY_DEBUG


/* the debug on/off macro */

#ifdef PROXY_DEBUG

/* print debug message */
#define DBG(fmt, args...)	printf("proxy:%s:%d: "fmt, __FILE__,	\
				       __LINE__, ##args)

/* print buffer content */
#define DBGBUF(buf, size, fmt, args...) \
	printf("[buf]: "fmt, ##args); \
	do { \
	write(0, buf, size); \
	fsync(0); \
	printf("\n\n"); \
	} while (0);
#else
#define DBG(fmt, args...)
#define DBGBUF(buf, size, fmt, args...)
#endif

#define ERR(fmt, args...) fprintf(stderr, "ERR:%s:%d: " fmt, \
				  __FILE__, __LINE__, ##args)

/* print IP infomation */
#define NIPQUAD(addr) \
        ((unsigned char *)&addr)[0], \
        ((unsigned char *)&addr)[1], \
        ((unsigned char *)&addr)[2], \
        ((unsigned char *)&addr)[3]
#define NIPQUAD_FMT "%u.%u.%u.%u"


#endif /* end of FZ_DEBUG_H */

