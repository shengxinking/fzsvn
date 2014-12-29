/**
 *	@file	ssl_decode.h
 *
 *	@brief	SSL record decode structure and APIs.
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_SSL_DECODE_H
#define FZ_SSL_DECODE_H

#include <sys/types.h>
#include <openssl/evp.h>


#define	DSSL_REC_MAX		20480

/* SSL record type */
#define	DSSL_RT_CCS		0x14
#define	DSSL_RT_ALERT		0x15
#define	DSSL_RT_HSK		0x16
#define	DSSL_RT_APP		0x17

/* handshake type */
#define	DSSL_HT_HELLO_REQ	0x0
#define	DSSL_HT_CLI_HELLO	0x1
#define	DSSL_HT_SVR_HELLO	0x2
#define	DSSL_HT_CERTIFICATE	0xb
#define	DSSL_HT_SVR_KEYEXG	0xc
#define	DSSL_HT_CERT_REQ	0xd
#define	DSSL_HT_SVR_DONE	0xe
#define	DSSL_HT_CLI_CERTVRY	0xf
#define	DSSL_HT_CLI_KEYEXG	0x10
#define	DSSL_HT_FINISHED		0x14

/* change cipher spec type */
#define	DSSL_CT_CCS		0x1

/* alert type */
#define	DSSL_AT_WARN		0x1
#define	DSSL_AT_FATAL		0x2

/* SSL record header */
typedef struct dssl_rec {
	u_int8_t	type;		/* record type */
	u_int8_t	major;		/* major version */
	u_int8_t	minor;		/* minor version */
	u_int16_t	len;		/* payload length, excluding header */
} dssl_rec_t;

/* SSL handshake header */
typedef struct dssl_hsk {
	u_int8_t	type;		/* handshake type */
	u_int16_t	len;		/* payload length excluding header */
} dssl_hsk_t;

/* SSL decode context */
typedef struct dssl_ctx {
	EVP_PKEY	*key;		/* RSA private key */
	u_int32_t	refcnt;		/* reference count */
} dssl_ctx_t;

/* SSL session */
typedef struct dssl {
	ssl_dctx_t	*ctx;		/* the context of this session */
	dbuf_t		domain;		/* client domain name */
	dbuf_t		sid;		/* client session id */
	dbuf_t		random[2];	/* client random */
	dbuf_t		pms;		/* pre master security */
	dbuf_t		ms;		/* master security */
	EVP_CIPHER_CTX	*key[2];	/* client decrypt key */
	EVP_MD_CTX	*md[2];		/* client message digest */
	dbuf_t		rec[2];		/* SSL record buffer */
	dbuf_t		pdata[2];	/* SSL plain data */
} dssl_t;

/* SSL session for session reuse */
typedef struct dssl_ssn {
	dbuf_t		sid;
	dbuf_t		pms;
} dssl_ssn_t;

extern dssl_ctx_t * 
dssl_ctx_new(void);

extern int 
dssl_ctx_set_pkey(dssl_ctx_t *ctx, const char *pkey, const char *pw);

extern int
dssl_ctx_free(dssl_ctx_t *ctx);

extern dssl_t *
dssl_new(dssl_ctx_t *ctx);

extern int
dssl_free(dssl_t *ssl);

/**
 *	Decode SSL packet @pkt.
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
dssl_decode(dssl_t *s, const u_int8_t *buf, size_t len);


#endif /* end of FZ_SSL_DECODE_H */


