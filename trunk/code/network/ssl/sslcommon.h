/*
 * @file	ssl_common.h
 * @brief	the common function for SSL connection using 
 * 		openssl library
 *
 * @author	Forrest.zhang
 */

#ifndef FZ_SSL_COMMON_H
#define FZ_SSL_COMMON_H

#include <openssl/ssl.h>

extern SSL_CTX *ssl_svr_ctx(const char *cafile, 
			const char *keyfile, 
			const char *passwd);

extern SSL_CTX *ssl_cli_ctx(void);

extern int ssl_cert_verify(const char *cafile, 
			const char *keyfile,
			const char *passwd);

extern void ssl_free(SSL_CTX *ctx);


#endif /* end of FZ_SSL_COMMON_H */

