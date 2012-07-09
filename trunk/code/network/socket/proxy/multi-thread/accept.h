/*
 *	@file	accept.h
 *
 *	@brief	Declare some functions and data types for accept thread 
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_ACCEPT_H
#define FZ_ACCEPT_H

/**
 *	The private data of accept thread.
 */
typedef struct _accept {
	int		http_fd;	/* HTTP listen fd */
	int		https_fd;	/* HTTPS listen fd */
	int		index;		/* the next work index */
} accept_t;


/**
 *	The main function of accept thread.
 *
 *	Always return NULL.
 */
void * 
accept_run(void *arg);


#endif /* end of FZ_ACCEPT_H */


