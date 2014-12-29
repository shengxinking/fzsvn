/**
 *	@file	proxy_debug.h
 *
 *	@brief	proxy debug Macro and APIs.
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_PROXY_DEBUG_H
#define FZ_PROXY_DEBUG_H

#include <unistd.h>
#include <string.h>

#include "gcc_common.h"
#include "dbg_common.h"

#define TSFMT		"<%02d:%02d:%02d>"
#define FLOWFMT		"[worker %d][flow]: ssn %u "
#define HTTPFMT		"[worker %d][http]: ssn %u "

extern int		g_timestamp;	/* timestamp in debug, 0/1 */
extern int		g_dbglvl;	/* global debug level(0-7) */
extern int		g_flowlvl;	/* global flow level(0-7) */
extern int		g_httplvl;	/* global http level(0-7) */


#define	DBG(level, fmt, args...)				\
({							\
	if (unlikely(level <= g_dbglvl)) {		\
		if (g_timestamp) {			\
			time_t _dbg_clock;		\
			struct tm _dbg_tm;		\
			_dbg_clock = time(NULL);		\
			gmtime_r(&_dbg_clock, &_dbg_tm);	\
			printf(TSFMT"debug: "fmt,	\
			       _dbg_tm.tm_hour,		\
			       _dbg_tm.tm_min,		\
			       _dbg_tm.tm_sec,		\
			       ##args);			\
		}					\
		else {					\
			printf("[debug]: "fmt ,		\
			       ##args);			\
		}					\
	}						\
})

#define	FLOW(level, fmt, args...)			\
({							\
	if (unlikely(level <= g_flowlvl)) {		\
		if (g_timestamp) {			\
			time_t _dbg_clock;		\
			struct tm _dbg_tm;		\
			_dbg_clock = time(NULL);		\
			gmtime_r(&_dbg_clock, &_dbg_tm);	\
			printf(TSFMT""FLOWFMT""fmt,	\
			       _dbg_tm.tm_hour,		\
			       _dbg_tm.tm_min,		\
			       _dbg_tm.tm_sec,		\
			       ti->index,		\
			       s->sid,			\
			       ##args);			\
		}					\
		else {					\
			printf(FLOWFMT""fmt,		\
			       ti->index,		\
			       s->sid, 			\
			       ##args);			\
		}					\
	}						\
})

#define	HTTP(level, fmt, args...)			\
({							\
	if (unlikely(level <= g_httplvl)) {		\
		if (g_timestamp) {			\
			time_t _dbg_clock;		\
			struct tm _dbg_tm;		\
			_dbg_clock = time(NULL);		\
			gmtime_r(&_dbg_clock, &_dbg_tm);	\
			printf(TSFMT""HTTPFMT""fmt,	\
			       _dbg_tm.tm_hour,		\
			       _dbg_tm.tm_min,		\
			       _dbg_tm.tm_sec,		\
			       ti->index,		\
			       s->sid,			\
			       ##args);			\
		}					\
		else {					\
			printf(HTTPFMT""fmt,		\
			       ti->index,		\
			       s->sid, 			\
			       ##args);			\
		}					\
	}						\
})


#endif /* end of FZ_PROXY_DEBUG_H */


