/**
 *	@file	packet_ssl.c
 *
 *	@brief	SSL packet decode functions.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2014-12-25
 */

#include <openssl/ssl.h>

#include "gcc_common.h"
#include "dbg_common.h"

#include "dssl_util.h"

/**
 *	PEM format cert/privatekey file password callback function
 *
 *	Return password length if success, 0 if no password.
 */
static int 
_dssl_pem_password_cb(char *buf, int size, int rwflag, void *u)
{
	char *passwd;
	size_t plen;

	passwd = (char *)u;
	if (!passwd)
		return 0;

	plen = strlen(passwd);
	if (plen < 1 || plen > size)
                return 0;

        memcpy(buf, passwd, plen);

        return plen;
}



dssl_ctx_t *
dssl_ctx_new(void)
{
	dssl_ctx_t *ctx;

	ctx = malloc(sizeof(dssl_ctx_t));
	if (unlikely(!ctx))
		ERR_RET(NULL, "malloc failed: %s\n", ERRSTR);

	memset(ctx, 0, sizeof(*ctx));

	return ctx;
}

void 
dssl_ctx_free(dssl_ctx_t *ctx)
{
	int refcnt;

	if (unlikely(!ctx)) {
		ERR("invalid argument\n");
		return;
	}

	refcnt = __sync_fetch_and_sub(&ctx->refcnt, 1);
	if (refcnt > 0)
		return;
	
	if (ctx->pkey)
		EVP_PKEY_free(ctx->pkey);

	free(ctx);
}

int 
dssl_ctx_load_pkey(dssl_ctx_t *ctx, const char *keyfile, const char *pw)
{
	FILE *fp;
	EVP_PKEY *pkey;
	pem_password_cb *cb = NULL;
	
	if (unlikely(!ctx || !keyfile))
		ERR_RET(-1, "invalid argument @keyfile\n");

	if (pw)
		cb = _dssl_pem_password_cb;

	fp = fopen(keyfile, "r");
	if (unlikely(!fp))
		ERR_RET(-1, "open keyfile %s failed: %s\n", 
			keyfile, ERRSTR);

	/* try to read PEM format private key file */
	pkey = PEM_read_PrivateKey(fp, NULL, cb, (char *)pw);
	if (pkey) {
		fclose(fp);
		ctx->pkey = pkey;
		return 0;
	}

	/* try to read DER format private key file */
	fseek(fp, 0L, SEEK_SET);
	pkey = d2i_PrivateKey_fp(fp, NULL);

	fclose(fp);

	if (!pkey) 
		ERR_RET(-1, "load private key %s failed\n", keyfile);

	ctx->pkey = pkey;

	return 0;
}

int 
dssl_decode(dssl_t *s, const u_int8_t *buf, size_t len, int dir)
{
	if (unlikely(!s || !buf || len < 1))
		ERR_RET(-1, "invalid argument\n");

	
	return 0;
}

u_int8_t *
dssl_data(dssl_t *s, int dir)
{
	dbuf_t *buf;
	
	if (unlikely(!s))
		ERR_RET(NULL, "invalid argument\n");

	buf = &s->bufs[dir % 2];
	return DBUF_DATA(buf);
}

int 
dssl_drop_data(dssl_t *s, int n, int dir)
{
	return 0;
}



