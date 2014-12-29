/**
 *	@file	sslutil.h
 *
 *	@brief	SSL utilities, like check certificate.
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_SSL_UTIL_H
#define FZ_SSL_UTIL_H

#include <openssl/ssl.h>

#include "cblist.h"

#define	SSL_VRY_CA	0x0001
#define	SSL_VRY_CRL	0x0002
#define	SSL_VRY_ALL	(SSL_VRY_CA | SSL_VRY_CRL)
#define	SSL_VRY_NONE	0x0004

/**
 *	ssl context for client/server.
 */
typedef enum ssl_sd {
	SSL_SD_CLIENT,			/* client side ssl */
	SSL_SD_SERVER,			/* server side ssl */
} ssl_sd_e;

typedef	enum ssl_wt {
	SSL_WT_NONE,
	SSL_WT_READ,			/* the lower-IO blocked read */
	SSL_WT_WRITE,			/* the lower-IO blocked write */
} ssl_wt_e;

typedef enum ssl_key {
	SSL_KEY_512,
	SSL_KEY_1024,
	SSL_KEY_1536,
	SSL_KEY_2048,
	SSL_KEY_3072,
	SSL_KEY_4096,
	SSL_KEY_6144,
	SSK_KEY_8192,
	SSL_KEY_MAX,
} ssl_key_e;

#define	SSL_V2		0x0001
#define	SSL_V3		0x0002
#define	SSL_V10		0x0010
#define	SSL_V11		0x0020
#define	SSL_V12		0x0040
#define	SSL_V1X		(SSL_V10 | SSL_V11 | SSL_V12)
#define	SSL_V23		(SSL_V2 | SSL_V3 | SSL_V1X)

#define	SSL_CIPHER_MAX	256
#define	SSL_DOMAIN_MAX	256

/**
 *	The ssl context.
 */
typedef struct ssl_ctx {
	SSL_CTX		*ctx;
	ssl_sd_e	side;		/* client/server: SSL_SD_XXX */

	int		flags;		/* ssl flags */
	int		renegotiate;	/* enable renegotiate or not */
	cblist_t	list;		/* sni list */
	int		refcnt;		/* reference count */
	char		domain[SSL_DOMAIN_MAX];	/* the domain name, 256 is enough */
	char		ciphers[SSL_CIPHER_MAX];
} ssl_ctx_t;

/**
 *	Alloc SSL context, the @side define it's Client/Server SSL context.
 *	The domain name of this context is @domain. It's only used for 
 *	client, the server context get it from certificate's CN field.
 *
 *	Return SSL context if success, NULL on error.
 */
extern ssl_ctx_t * 
ssl_ctx_alloc(ssl_sd_e side);

/**
 *	Load certificate into SSL context @sc.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
ssl_ctx_load_cert(ssl_ctx_t *sc, const char *certfile, 
		  const char *keyfile, const char *password);

/**
 *	Load Intermedia CA certificate into SSL context @sc.
 *
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
ssl_ctx_load_intmedcert(ssl_ctx_t *sc, const char *certfile);

/**
 *	Load CA certificate into SSL context @sc. 
 *	it'll enable verify peer in server side and client
 *	must send certificate.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
ssl_ctx_load_cacert(ssl_ctx_t *sc, const char *cert);

/**
 *	Load CRL file into SSL context @sc. 
 *	it'll enable verify peer in server side and client
 *	must send certificate.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
ssl_ctx_load_crl(ssl_ctx_t *sc, const char *crlfile);

/**
 *	Set ssl protocol @protocol for SSL context @sc.
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
ssl_ctx_set_protocol(ssl_ctx_t *sc, int protocol);

/**
 *	Set ciphers @ciphers for SSL context @sc
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
ssl_ctx_set_ciphers(ssl_ctx_t *sc, const char *ciphers);

/**
 *	Set renegotiate @renegotiate for SSL context @sc. 
 *	If @renegotiate is not zero, the renegotiate is 
 *	enabled, or else disabled.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
ssl_ctx_set_renegotiate(ssl_ctx_t *sc, int renegotiate);

/**
 *	Set Perfect Forward Security @pfs in SSL context @sc.
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
ssl_ctx_set_pfs(ssl_ctx_t *sc, int pfs);

/**
 *	Set the domain name of give SSL context @sc.
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
ssl_ctx_set_domain(ssl_ctx_t *sc, const char *domain);

/**
 *	Add sni context @sni into SSL context @sc.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
ssl_ctx_add_sni(ssl_ctx_t *sc, ssl_ctx_t *sni);

/**
 *	Free SSL context @sc.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
ssl_ctx_free(ssl_ctx_t *sc);

/**
 *	Alloc a new SSL object.
 *
 *	Return SSL object if success, NULL on error.
 */
extern SSL * 
ssl_alloc(ssl_ctx_t *sc);

/**
 *	Set @fd into SSL object @ssl. It'll call @ssl_handshake()
 *	after set fd.
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
ssl_set_fd(SSL *ssl, int fd);

/**
 *	Accept a SSL connection on SSL object @ssl. the @ssl is 
 *	alloced by @ssl_alloc() 
 *
 *	the @wait value is below:
 *
 *	SSL_WT_NONE	operation finished.
 *	SSL_WT_READ	operation blocked by read, call ssl_handshake(),
 *	SSL_WT_WRITE	operation blocked by write, call ssl_handshake().
 *
 *	Return 0 success, -1 on error.
 */
extern int 
ssl_accept(SSL *ssl, int fd, ssl_wt_e *wait);

/**
 *	Connect to SSL server on SSL object @ssl. the @ssl is 
 *	alloced by @ssl_alloc() 
 *
 *	the @wait value is below:
 *
 *	SSL_WT_NONE	operation finished.
 *	SSL_WT_READ	operation blocked by read, need call ssl_handshake(),
 *	SSL_WT_WRITE	operation blocked by write, need call ssl_handshake().
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
ssl_connect(SSL *ssl, int fd, ssl_wt_e *wait);

/**
 *	Do ssl handshake on SSL object @ssl, the @wait value
 *	is below:
 *
 *	SSL_WT_NONE	operation finished, not call again.
 *	SSL_WT_READ	operation blocked by read, call again,
 *	SSL_WT_WRITE	operation blocked by write, call again.
 *
 *	Return 0 if success -1 on error.
 */
extern int 
ssl_handshake(SSL *ssl, ssl_wt_e *wait);

/**
 *	Do SSL renegotiate on a exist SSL connect. the @wait value
 *	is below:
 *
 *	SSL_WT_NONE	operation finished.
 *	SSL_WT_READ	operation blocked by read, call ssl_handshake(),
 *	SSL_WT_WRITE	operation blocked by write, call ssl_handshake().
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
ssl_renegotiate(SSL *ssl, ssl_wt_e *wait);

/**
 *	Recv a data from SSL object @ssl, the recved data stored 
 *	in @buf, the @buf length is @len.
 *
 *	If @closed set 1, then the peer side shutdown SSL.
 *
 *	If @handshake set 1, then the peer side send handshake message, 
 *	need call ssl_handshake() to finished handshake.
 *
 *	Return received data length if success, -1 on error.
 */
extern int 
ssl_recv(SSL *ssl, void *buf, int len, int *closed, int *handshake);

/**
 *	Send a data via SSL object @ssl, the data stored in @buf, 
 *	length is @len
 *
 *	return >= 0 is success, -1 on error.
 */
extern int 
ssl_send(SSL *ssl, const void *buf, int len);

/**
 *	Shutdown the SSL connection on @ssl.
 * 
 *	If @wait is set 1, then need call it again.
 *
 *	Return 0 if shutdown success, -1 on error.
 */
extern int 
ssl_shutdown(SSL *ssl, ssl_wt_e *wait);

/**
 *	Return error code of @ssl in handshake. 
 *
 *	Return the error number if success, 0 if no error.
 */
extern unsigned long 
ssl_get_error(const SSL *ssl);

/**
 *	Return error string according error code @error.
 *
 *	Return the error string if success, NULL if no error.
 */
extern const char * 
ssl_get_errstr(unsigned long error);

/**
 *	Free the SSL connect @sn.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
ssl_free(SSL *ssl);

#endif /* end of FZ_SSL_UTIL_H */

