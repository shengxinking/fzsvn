/*
 * @file	ssl_common.c
 * @brief	common SSL function implement using
 * 		openssl library
 *
 * @author	Forrest.zhang
 */

#include <string.h>
#include <stdio.h>
#include <openssl/err.h>

#include "sslcommon.h"

#define PASSLEN		20
static char _ssl_passwd[PASSLEN] = {0};

static int _ssl_set_passwd(char *buf, int size, int rwflag, void *userdata)
{
	if (size < strlen(_ssl_passwd) + 1)
		return 0;
	strncpy(buf, _ssl_passwd, size);
	return strlen(_ssl_passwd);
}

SSL_CTX *ssl_svr_ctx(const char *cafile, 
			const char *keyfile, 
			const char *passwd)
{
	SSL_CTX *ctx = NULL;

	SSL_load_error_strings();
	SSL_library_init();

	ctx = SSL_CTX_new(SSLv23_server_method());
	if (!ctx) {
		printf("SSL_CTX_new error\n");
		ERR_print_errors_fp(stderr);
		return NULL;
	}
	
	if (SSL_CTX_use_certificate_file(
		ctx, cafile, SSL_FILETYPE_PEM) != 1) {
		printf("SSL_CTX_use_certificate_file error\n");
		ERR_print_errors_fp(stderr);
		return NULL;
	}

	if (passwd) {
		strncpy(_ssl_passwd, passwd, 19);
		SSL_CTX_set_default_passwd_cb(ctx, _ssl_set_passwd);
	}

	if (SSL_CTX_use_PrivateKey_file(
		ctx, keyfile, SSL_FILETYPE_PEM) != 1) {
		printf("SSL_CTX_use_PrivateKey_file error\n");
		ERR_print_errors_fp(stderr);
		return NULL;
	}

	return ctx;
}

SSL_CTX *ssl_cli_ctx(void)
{
	SSL_CTX *ctx = NULL;

	SSL_load_error_strings();
	SSL_library_init();

	ctx = SSL_CTX_new(SSLv23_client_method());
	if (!ctx) {
		printf("SSL_CTX_new error\n");
		ERR_print_errors_fp(stderr);
		return NULL;
	}
	
	return ctx;
}

int ssl_cert_verify(const char *cafile,
		const char *keyfile,
		const char *passwd)
{
	SSL_CTX *ctx = NULL;

	SSL_load_error_strings();
	SSL_library_init();

	ctx = SSL_CTX_new(SSLv23_server_method());
	if (!ctx) {
		printf("SSL_CTX_new error\n");
		ERR_print_errors_fp(stderr);
		return -1;
	}
	
	if (SSL_CTX_use_certificate_file(
		ctx, cafile, SSL_FILETYPE_PEM) != 1) {
		printf("SSL_CTX_use_certificate_file error\n");
		ERR_print_errors_fp(stderr);
		SSL_CTX_free(ctx);
		return -1;
	}

	if (passwd) {
		strncpy(_ssl_passwd, passwd, 19);
		SSL_CTX_set_default_passwd_cb(ctx, _ssl_set_passwd);
	}

	if (SSL_CTX_use_PrivateKey_file(
		ctx, keyfile, SSL_FILETYPE_PEM) != 1) {
		printf("SSL_CTX_use_PrivateKey_file error\n");
		ERR_print_errors_fp(stderr);
		SSL_CTX_free(ctx);
		return -1;
	}

	SSL_CTX_free(ctx);
	return 0;
}


void ssl_free(SSL_CTX *ctx)
{
	if (ctx)
		SSL_CTX_free(ctx);
}
