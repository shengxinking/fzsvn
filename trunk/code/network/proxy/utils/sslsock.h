/**
 *	@file	sslsock.h
 *
 *	@brief	SSL socket implement.
 *	
 *	@author	Forrest.zhang
 *
 *	@date	2012-05-22
 */

#ifndef FZ_SSLSOCK_H
#define FZ_SSLSOCK_H

typedef struct sslsock {
	SSL_CTX		**server_ctx;
	SSL_CTX		*client_ctx;
	int		nserver;
	int		nclient;
} sslsock_t;


/**
 *	Alloc a new sslsock object
 *
 *	Return pointer to object if success, NULL on error.
 */
extern sslsock_t * 
sslsock_alloc(int nclient, int nserver);

/**
 * *	
 *
 * */
extern int 
sslsock_add_cert(sslsock_t *sk, const char *cert, const char *key, 
		 const char *pass, int index);


extern int 
sslsock_add_ca(sslsock_t *sk, const char *ca, int index);


extern int 
sslsock_add_crl(sslsock_t *sk, const char *crl, int index);

extern void  
sslsock_free(sslsock_t *sk);

extern SSL * 
sslsock_get(sslsock_t *sk, int index, int issvr);

extern int 
sslsock_put(sslsock_t *sk, SSL *ssl);

#endif /* end of FZ_SSLSOCK_H  */


