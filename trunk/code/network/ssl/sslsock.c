/**
 *	@file	sslsock.c
 *
 *	@brief	Implement SSL socket APIs: create client/server SSL socket, 
 *		recv/send SSL data.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2009-08-10
 */

#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pkcs12.h>
#include <openssl/rand.h>
#include <sys/poll.h>
#include <errno.h>

#include "sock.h"
#include "sslsock.h"


/**
 *	convert IP address(uint32_t) to dot-decimal string
 *	like xxx.xxx.xxx.xxx 
 */
#ifndef NIPQUAD
#define NIPQUAD(addr) \
        ((unsigned char *)&addr)[0], \
        ((unsigned char *)&addr)[1], \
        ((unsigned char *)&addr)[2], \
        ((unsigned char *)&addr)[3]
#define NIPQUAD_FMT "%u.%u.%u.%u"
#endif

/**
 *	Define debug MACRO to print debug information
 */
#define _SS_DBG	1
#ifdef _SS_DBG
#define _SS_ERR(fmt, args...)	fprintf(stderr, "sslsock:%s:%d: "fmt,\
					__FILE__, __LINE__, ##args)
#else
#define _SS_ERR(fmt, args...)
#endif

/**
 *	Get a temp DH, it's copy from openssl/s_server.c
 */
static unsigned char _ss_dh512_p[] = {
        0xDA,0x58,0x3C,0x16,0xD9,0x85,0x22,0x89,0xD0,0xE4,0xAF,0x75,
        0x6F,0x4C,0xCA,0x92,0xDD,0x4B,0xE5,0x33,0xB8,0x04,0xFB,0x0F,
        0xED,0x94,0xEF,0x9C,0x8A,0x44,0x03,0xED,0x57,0x46,0x50,0xD3,
        0x69,0x99,0xDB,0x29,0xD7,0x76,0x27,0x6B,0xA2,0xD3,0xD4,0x12,
        0xE2,0x18,0xF4,0xDD,0x1E,0x08,0x4C,0xF6,0xD8,0x00,0x3E,0x7C,
        0x47,0x74,0xE8,0x33,
};

static unsigned char _ss_dh512_g[] = {
        0x02,
};

static DH *_ss_get_dh512(void)
{
        DH *dh=NULL;
	
        if ((dh=DH_new()) == NULL) 
		return(NULL);
	
	dh->p=BN_bin2bn(_ss_dh512_p,sizeof(_ss_dh512_p),NULL);
        dh->g=BN_bin2bn(_ss_dh512_g,sizeof(_ss_dh512_g),NULL);
	
	if ((dh->p == NULL) || (dh->g == NULL))
                return(NULL);
	
	return(dh);
}


sslsock_ctx_t * 
ss_ctx_alloc(int socktype, int verifytype)
{
	sslsock_ctx_t *sc = NULL;
	DH *dh = NULL;
//	EC_KEY *ecdh = NULL;

	/* alloc memory for sslsock_ctx_t */
	sc = malloc(sizeof(sslsock_ctx_t));
	if (!sc) {
		_SS_ERR("malloc memory for sslsock_ctx_t failed\n");
		return NULL;
	}
	memset(sc, 0, sizeof(sslsock_ctx_t));

	/* initiate SSL library, load crypto/ssl error strings */
	SSL_load_error_strings();
	ERR_load_crypto_strings();
	SSL_library_init();
	OpenSSL_add_all_algorithms();

	/* create SSL_CTX object */
	sc->ctx = SSL_CTX_new(SSLv23_method());
	if (sc->ctx == NULL) {
		_SS_ERR("create SSL_CTX failed\n");
		ss_ctx_free(sc);
		return NULL;
	}
	
	/* set temp DH, for DSA private key/cert */
	dh = _ss_get_dh512();
	SSL_CTX_set_tmp_dh(sc->ctx, dh);

	/* set temp ECDH, for DSA private key/cert */
//	ecdh = EC_KEY_new_by_curve_name(NID_sect163r2);
//	SSL_CTX_set_tmp_ecdh(sc->ctx,ecdh);

	/* set verify type */
	if (verifytype == SS_VERIFY_NONE)
		SSL_CTX_set_verify(sc->ctx, SSL_VERIFY_NONE, NULL);
	else if (verifytype == SS_VERIFY_CLIENT) {
		SSL_CTX_set_verify(sc->ctx, SSL_VERIFY_PEER | 
				   SSL_VERIFY_CLIENT_ONCE, NULL);
		SSL_CTX_set_verify_depth(sc->ctx, 100);
	}
	else {
		_SS_ERR("unkowned ssl verify type %d\n", verifytype);
		ss_ctx_free(sc);
		return NULL;
	}

	/* set the mode */
	SSL_CTX_set_mode(sc->ctx, SSL_MODE_AUTO_RETRY);

	sc->socktype = socktype;
	sc->verifytype = verifytype;

	return sc;
}


/**
 *	The private key password callback function.
 *
 *	Return the password length.
 */
static int 
_ss_passwd_cb(char *buf, int size, int rwflag, void *userdata)
{
	char *passwd;

	passwd = (char *)userdata;
	if (!passwd || strlen(passwd) < 1)
		return 0;

	memcpy(buf, passwd, size);

	return strlen(passwd);
}


/**
 *	Load X509 certificate @certfile and Private key file @keyfile 
 *	into sslsock_ctx @sc, the file type is @filetype.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_ss_ctx_load_x509(sslsock_ctx_t *sc,
		  const char *cert,
	          const char *key, 
	          const char *passwd, 
	          int filetype)
{
	int ret = 0;

	if (!sc || !cert || !key)
		return -1;

	if (access(cert, R_OK)) {
		_SS_ERR("can't access certificate %s\n", cert);
		return -1;
	}

	if (access(key, R_OK)) {
		_SS_ERR("can't access private keyfile %s\n", key);
		return -1;
	}

	/* set private-key/pkcs12 password callback */
	if (passwd && strlen(passwd) > 0) {
		SSL_CTX_set_default_passwd_cb_userdata(sc->ctx, (char *)passwd);
	}
	else {
		SSL_CTX_set_default_passwd_cb_userdata(sc->ctx, "");
	}
	SSL_CTX_set_default_passwd_cb(sc->ctx, _ss_passwd_cb);

	/* load X509 certificate file */
	ret = SSL_CTX_use_certificate_file(sc->ctx, cert, filetype);
	if (ret < 1) {
		return -1;
	}

	/* load private key file */
	ret = SSL_CTX_use_PrivateKey_file(sc->ctx, key, filetype); 
	if (ret < 1) {
		_SS_ERR("load private key file failed\n");
		return -1;
	}
	
	return 0;
}


/**
 *	Load PKCS12 certificate @certfile into sslsock_ctx @sc
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_ss_ctx_load_pkcs12(sslsock_ctx_t *sc, const char *cert, const char *passwd)
{
	BIO *bio1 = NULL;
	X509 *x509 = NULL;
	EVP_PKEY *key = NULL;
	PKCS12 *p12 = NULL;

	if (!sc || !sc->ctx || !cert)
		return -1;

	if (access(cert, R_OK)) {
		_SS_ERR("can't access PKCS12 file %s\n", cert);
		return -1;
	}

	/* read pkcs12 file into BIO */
	bio1 = BIO_new_file(cert, "rb");
	if (!bio1) {
		_SS_ERR("BIO read pkcs12 file %s failed\n", cert);
		return -1;
	}

	/* decode PKCS12 file */
	p12 = d2i_PKCS12_bio(bio1, NULL);
	if (!p12) {
		BIO_free(bio1);
		return -1;
	}

	BIO_free(bio1);
	bio1 = NULL;

	/* decode PKCS12 object to X509 object and EVP_PKEY object */
	if (PKCS12_parse(p12, passwd, &key, &x509, NULL) < 1) {
		_SS_ERR("parse pkcs12 failed, error passwd\n");
		PKCS12_free(p12);
		return -1;
	}
	PKCS12_free(p12);
	p12 = NULL;

	/* check X509 object and EVP_PKEY object */
	if (!x509 || !key) {
		if (x509)
			X509_free(x509);
		if (key)
			EVP_PKEY_free(key);
		return -1;
	}

	/* load X509 object */
	if (SSL_CTX_use_certificate(sc->ctx, x509) < 1) {
		_SS_ERR("use X509 cert failed\n");
		if (x509)
			X509_free(x509);
		if (key)
			EVP_PKEY_free(key);
		return -1;
	}

	/* load private key */
	if (SSL_CTX_use_PrivateKey(sc->ctx, key) < 1) {
		_SS_ERR("use EVP_PKEY failed\n");
		if (x509)
			X509_free(x509);
		if (key)
			EVP_PKEY_free(key);
		return -1;
	}

	X509_free(x509);
	EVP_PKEY_free(key);

	return 0;
}


int 
ss_ctx_load_cert(sslsock_ctx_t *sc, 
		 const char *cert, 
		 const char *key, 
		 const char *passwd)
		 
{
	int ret = 0;
	
	if (!sc) {
		return -1;
	}
	
	if (sc->socktype == SS_SERVER) {
		if (!cert || strlen(cert) < 1)
			return -1;
	}
	else {
		/* client SSL socket can work without certificate */
		if ( sc->verifytype == SS_VERIFY_CLIENT &&
		     (!cert || strlen(cert) < 1) )
			return -1;
		if (!cert || strlen(cert) < 1)
			return 0;
	}
	
	/* try load PKCS12 */
	ret = _ss_ctx_load_pkcs12(sc, cert, passwd);
	if (ret == 0) {
		goto _check_key;
	}		
	
	/* try load X509 PEM cert */
	ret = _ss_ctx_load_x509(sc, cert, key, passwd, 
				SSL_FILETYPE_PEM);
	if (ret == 0) {
		goto _check_key;
	}
	
	/* try load X509 DER cert */
	ret = _ss_ctx_load_x509(sc, cert, key, passwd,
				SSL_FILETYPE_ASN1);
	if (ret == 0) {
		goto _check_key;
	}
	
	return -1;
	
_check_key:
	
	/* check private key */
	ret = SSL_CTX_check_private_key(sc->ctx);
	if (ret < 1) {
		_SS_ERR("the private key is not fit certificate\n");
		return -1;
	}
	
	return 0;
}
		 

int 
ss_ctx_load_ca(sslsock_ctx_t *sc, const char *cert)
{
	if (!sc || !cert)
		return -1;

	return 0;
}


int 
ss_ctx_load_crl(sslsock_ctx_t *sc, const char *crl)
{
	if (!sc || !crl)
		return -1;

	return 0;
}


int 
ss_ctx_set_cipher(sslsock_ctx_t *sc, const char *cipher)
{
	int ret = 0;

	if (!cipher)
		return -1;

	ret = SSL_CTX_set_cipher_list(sc->ctx, cipher);
	if (ret < 1) {
		_SS_ERR("set SSL_CTX cipher failed\n");
		return -1;
	}

	return 0;
}


int 
ss_handshake(sslsock_t *ss)
{
	int ret = 0;

	if (!ss || ss->fd < 0 || !ss->ssl)
		return -1;

	ret = SSL_do_handshake(ss->ssl);

	if (ret != 1) {
		if (SSL_get_error(ss->ssl, ret) == SSL_ERROR_WANT_READ) {
			return 0;
		}

		_SS_ERR("SSL_connect failed: ret %d\n", ret);

		ERR_print_errors_fp(stdout);
		return -1;
	} 

	return 1;
}


int 
ss_set_nbio(sslsock_t *ss, int nbio)
{
	if (!ss)
		return -1;

	return sock_set_nbio(ss->fd, nbio);
}




sslsock_t * 
ss_server(sslsock_ctx_t *sc, u_int32_t ip, u_int16_t port)
{
	sslsock_t *ss = NULL;
	
	if (!sc)
		return NULL;

	/* check @sc */
	if (sc->socktype != SS_SERVER) {
		sc->errnum = SS_ERR_SOCKTYPE;

		_SS_ERR("the sslsock_ctx_t is not SS_SERVER\n");
		return NULL;
	}

	/* alloc object */
	ss = malloc(sizeof(sslsock_t));
	if (!ss) {
		sc->errnum = SS_ERR_MALLOC;

		_SS_ERR("malloc memory for sslsock_t failed\n");
		return NULL;
	}
	memset(ss, 0, sizeof(sslsock_t));

	ss->ssl = SSL_new(sc->ctx);
	if (!ss->ssl) {
		sc->errnum = SS_ERR_NEWSSL;

		_SS_ERR("SSL_new failed\n");
		free(ss);
		return NULL;
	}

	ss->fd = sock_tcpsvr(ip, port);
	if (ss->fd < 0) {
		sc->errnum = SS_ERR_TCPSVR;
		ss_free(ss);

		_SS_ERR("sock_tcpsvr(%u.%u.%u.%u:%u) failed\n", 
			     NIPQUAD(ip), ntohs(port));
		return NULL;
	}

	ss->sip = ip;
	ss->sport = port;
	ss->type = SS_SERVER;
	ss->state = SS_STE_ACCEPT;
	ss->ctx = sc;

	return ss;
}



sslsock_t * 
ss_client(sslsock_ctx_t *sc, u_int32_t ip, u_int16_t port)
{
	sslsock_t *ss = NULL;
	struct sockaddr_in cliaddr;
	socklen_t len;
	
	if (!sc)
		return NULL;

	if (sc->socktype != SS_CLIENT) {
		sc->errnum = SS_ERR_SOCKTYPE; 

		_SS_ERR("the sslsock_ctx_t socktype error\n");
		return NULL;
	}

	/* alloc sslsock object */
	ss = malloc(sizeof(sslsock_t));
	if (!ss) {
		sc->errnum = SS_ERR_MALLOC;
		
		_SS_ERR("malloc memory for sslsock_t failed\n");
		return NULL;
	}
	memset(ss, 0, sizeof(sslsock_t));
	ss->type = SS_CLIENT;

	/* create SSL object */
	ss->ssl = SSL_new(sc->ctx);
	if (!ss->ssl) {
		ss_free(ss);

		_SS_ERR("SSL_new failed\n");
		return NULL;
	}

	/* TCP connect to server */
	ss->fd = sock_tcpcli(ip, port);
	if (ss->fd < 0) {
		sc->errnum = SS_ERR_TCPCLI;
		ss_free(ss);

		_SS_ERR("sock_tcpcli(%u.%u.%u.%u:%u) failed\n", 
			     NIPQUAD(ip), ntohs(port));
		return NULL;
	}

	/* bind socket fd to SSL */
	if (SSL_set_fd(ss->ssl, ss->fd) < 1) {
		sc->errnum = SS_ERR_SSL;
		ss_free(ss);

		_SS_ERR("SSL_set_fd failed\n");
		return NULL;
	}

	/* get local IP/Port */
	len = sizeof(cliaddr);
	if (getsockname(ss->fd, (struct sockaddr *)&cliaddr, &len)) {
		sc->errnum = SS_ERR_GETSOCKNAME;
		ss_free(ss);

		_SS_ERR("getsockname failed\n");
		return NULL;
	}

	/* do SSL handshake */
	ss->ctx = sc;

	/* set IP/Port */
	ss->sip = cliaddr.sin_addr.s_addr;
	ss->dip = ip;
	ss->sport = cliaddr.sin_port;
	ss->dport = port;
	ss->state = SS_STE_CONNECTED;
	SSL_set_connect_state(ss->ssl);

	return ss;
}


sslsock_t *
ss_accept(sslsock_t *ss, int algo)
{
	sslsock_t *cliss = NULL;
	struct sockaddr_in cliaddr;
	u_int32_t dip;
	u_int16_t dport;

	/* check socktype */
	if (ss->type != SS_SERVER) {
		ss->errnum = SS_ERR_SOCKTYPE;

		_SS_ERR("the socktype error\n");
		return NULL;
	}

	/* check socket fd */
	if (ss->fd < 0) {
		ss->errnum = SS_ERR_NOFD;
		return NULL;
	}

	/* malloc memory for sslsock_t object */
	cliss = malloc(sizeof(sslsock_t));
	if (!cliss) {
		ss->errnum = SS_ERR_MALLOC;
		return NULL;
	}
	memset(cliss, 0, sizeof(sslsock_t));
	cliss->type = SS_SERVER;

	/* alloc SSL */
	cliss->ssl = SSL_new(ss->ctx->ctx);
	if (!cliss->ssl) {
		ss->errnum = SS_ERR_NOSSL;
		ss_free(cliss);
		return NULL;
	}
	if (SSL_get_verify_result(cliss->ssl) != X509_V_OK) {
		ss_free(cliss);
		_SS_ERR("SSL_get_verify_result failed\n");
		return NULL;
	}
	
	/* tcp accept */
	cliss->fd = sock_accept(ss->fd, &dip, &dport);
	if (cliss->fd < 0) {
		if (errno == EAGAIN || errno == EINTR)
			return NULL;

		ss->errnum = SS_ERR_ACCEPT;

		ss_free(cliss);
		return NULL;
	}

	/* bind socket fd to SSL */
	if (SSL_set_fd(cliss->ssl, cliss->fd) < 1) {
		ss->errnum = SS_ERR_SSL;
		ss_free(cliss);

		_SS_ERR("SSL_set_fd failed\n");
		return NULL;
	}

	/* set IP/Port */
	cliss->sip = cliaddr.sin_addr.s_addr;
	cliss->dip = ss->sip;
	cliss->sport = cliaddr.sin_port;
	cliss->dport = ss->sport;
	cliss->state = SS_STE_CONNECTED;
	SSL_set_accept_state(cliss->ssl);
	
	return cliss;
}


const char *
ss_ctx_errstr(sslsock_ctx_t *sc)
{
	return "";
}


const char *
ss_errstr(sslsock_t *ss)
{
	return "";
}


int 
ss_recv(sslsock_t *ss, char *buf, size_t size, int *closed)
{
	int n; 

	if (!ss || !buf || size < 0 || !closed) {
		ss->errnum = SS_ERR_PARAM;
		return -1;
	}

	n = SSL_read(ss->ssl, buf, size);
	if (n < 0) {
		if (SSL_get_error(ss->ssl, n) == SSL_ERROR_WANT_READ) {
			n = 0;
		}
		else {
			ss->errnum = SS_ERR_RECV;
			return -1;
		}
	}
	else if (n == 0) {
		*closed = 1;
		return 0;
	}

	return n;
}


int 
ss_send(sslsock_t *ss, const char *buf, size_t size)
{
	struct pollfd pfd;
	int n, m; 
	int ret = 0;
	int remain;

	if (!ss || !buf || size < 0) {
		ss->errnum = SS_ERR_PARAM;
		return -1;
	}

	pfd.fd = ss->fd;
	pfd.events = POLLOUT;
	pfd.revents = 0;
	remain = size;
	m = 0;
	while (remain > 0) {
		ret = poll(&pfd, 1, -1);
		if (ret < 0) {
			if (errno == EINTR) {
				return m;
			}
			return -1;
		}
		else if (ret == 0) {
			return m;
		}		

		if (pfd.revents & POLLERR)
			return -1;

		n = SSL_write(ss->ssl, buf + m, remain);

		if (n < 0) {
			if (SSL_get_error(ss->ssl, n) == SSL_ERROR_WANT_WRITE) {
				continue;
			}
			
			ss->errnum = SS_ERR_SEND;
			return -1;
		}
		m += n;
		remain -= n;
	}

	return m;
}


void 
ss_print_ciphers(const sslsock_t *ss)
{
	STACK_OF(SSL_CIPHER) *sk;
	SSL_CIPHER *cipher;
	int n = 0;
	int i = 0;

	if (!ss || !ss->ssl)
		return;

	sk = SSL_get_ciphers(ss->ssl);
	if (!sk)
		return;

	n = sk_SSL_CIPHER_num(sk);
	printf("--------------- ciphers --------------\n");
	for (i = 0; i < n; i++) {
		cipher = sk_SSL_CIPHER_value(sk, i);
		printf("%d: %s\n", i, SSL_CIPHER_get_name(cipher));		
	}
	printf("---------------   end   --------------\n");

}


void 
ss_ctx_free(sslsock_ctx_t *sc)
{
	if (!sc)
		return;

	if (sc->ctx) {
		SSL_CTX_free(sc->ctx);
		sc->ctx = NULL;
	}

	ERR_free_strings();
	EVP_cleanup();

	free(sc);
}


void 
ss_free(sslsock_t *ss)
{
	int ret = 0;
	int retry = 8;

	if (!ss)
		return;
		
	if (ss->ssl) {		
		if (ss->state == SS_STE_CONNECTED) {
			ret = SSL_shutdown(ss->ssl);
			while (retry > 0 && ret != 1) {				
				usleep(100);
				ret = SSL_shutdown(ss->ssl);
				if (ret < 0 && SSL_get_error(ss->ssl, ret) != SSL_ERROR_WANT_READ) {
					_SS_ERR("SSL_shutdown failed\n");
					break;
				}
				retry--;
			}
		}
		SSL_clear(ss->ssl);
		SSL_free(ss->ssl);
		ss->ssl = NULL;
	}

	if (ss->fd >= 0) {
		close(ss->fd);
		ss->fd = -1;
	}

	free(ss);
}



