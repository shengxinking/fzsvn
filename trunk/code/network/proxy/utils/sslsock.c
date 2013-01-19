/**
 *	@file	sslsock.c
 *
 *	@brief	SSL socket implement.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2012-05-22
 */

#include "sslsock.h"

/**
 *	
 *
 *
 *
 */
sslsock_t *
sslsock_alloc(void)
{
	return NULL;
}

void 
sslsock_free(sslsock_t *sk)
{

}

/**
 * 	Return a SSL object.
 *
 *	issvr:	=0 is a client SSL, !=0 is a server SSL.
 *	index: the client/server index.
 *
 */
SSL *
sslsock_get(sslsock_t *sk, int index, int issvr)
{

	return NULL;
}


extern int
sslsock_put(sslsock_t *sk, SSL *ssl)
{
	return 0;
}


