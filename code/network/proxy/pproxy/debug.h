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

#include "gcc_common.h"

/* the debug level: 
 * 0	didn't show debug information.
 * 1	show flow information.
 * 2	show init information and flow information.
 * 3	show all debug information 
 */
extern int	g_dbglvl;

#define	WFLOW(level, fmt, args...)				\
	if (unlikely(g_dbglvl > level))					\
		printf("work[%d]<%d>: ssn(%u) "fmt, 		\
			py->index, pl->index, s->id, ##args)

/* print debug message */
#define DBG(fmt, args...)			\
	if (unlikely(g_dbglvl > 0))				\
		printf("pproxy: "fmt, ##args)

/* print buffer content */
#define DBGBUF(buf, size, fmt, args...) 	\
	if (g_dbglvl > 2)	{		\
		printf("[buf]: "fmt, ##args);	\
		do {				\
			write(0, buf, size);	\
			fsync(0);		\
			printf("\n\n");		\
		} while (0);			\
	}

/* print error message */
#define ERR(fmt, args...)			\
	fprintf(stderr, "[ERR]:%s:%d: " fmt,	\
		__FILE__, __LINE__, ##args)
#define	ERRSTR		strerror(errno)

#endif /* end of FZ_DEBUG_H */

