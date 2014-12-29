/**
 *	@file	childproc.h
 *
 *	@brief	Child process header file.
 *
 */

#ifndef	FZ_CHILDPROC_H
#define FZ_CHILDPROC_H

#define	MAX_CHILD	20

#include <sys/types.h>
#include "proxy.h"

enum {
	CPROC_BIND_RR,
	CPROC_BIND_ODD,
	CPROC_BIND_EVEN,
};

enum {
	CPROC_HT_FULL,
	CPROC_HT_LOW,
	CPROC_HT_HIGH,
};

typedef struct cproc {
	pid_t		pid;		/* child pid */
	int		index;		/* child index */
} cproc_t;

/**
 *	Create a child process and save information in @cp.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
cproc_create(cproc_t *cp, proxy_arg_t *arg);

/**
 *	Destroy a child process.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
cproc_destroy(cproc_t *cp);

/**
 *	Bind child process to CPU.
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
cproc_bind_cpu(pid_t pid, int index, int algo, int high);

#endif	/* end of FZ_CHILDPROC_H */
