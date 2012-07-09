/**
 *	@file	sslciphers.c
 *
 *	@brief	Print all SSL ciphers name.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2009-06-24
 */

#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/bio.h>

#include <unistd.h>
#include <stdio.h>
#include <getopt.h>

static SSL_CTX	*_g_sslctx = NULL;	/* ssl context */
static int 	_g_sslproto = 0;	/* ssl version, see following: */

enum {
	SSL_VER_2 = 0,
	SSL_VER_3,
	SSL_VER_23,
	TLS_VER_1,
	VER_MAX,
};

static void 
_usage(void)
{
	printf("sslciphers <options>\n");
	printf("\t-v\tSSL version: <ssl2 | ssl3 | ssl23 |tls1 |dtls1>\n");
	printf("\t-h\tshow help message\n");
}


static int 
_parse_cmd(int argc, char **argv)
{
	char c;
	const char optstr[] = ":v:h";

	opterr = 0;
	while ( (c = getopt(argc, argv, optstr)) != -1) {
		
		switch (c) {
			
		case 'v':
			if (strcmp(optarg, "ssl2") == 0) {
				_g_sslproto = SSL_VER_2;
			}
			else if (strcmp(optarg, "ssl3") == 0) {
				_g_sslproto = SSL_VER_3;
			}
			else if (strcmp(optarg, "ssl23") == 0) {
				_g_sslproto = SSL_VER_23;
			}
			else if (strcmp(optarg, "tls1") == 0) {
				_g_sslproto = TLS_VER_1;
			}
			else {
				printf("unkowned SSL protocol: %s\n", 
				       optarg);
				return -1;
			}
			break;

		case 'h':
			_usage();
			exit(0);
		
		case ':':
			printf("option %c miss argument\n", optopt);
			return -1;
		
		case '?':
			printf("unknow option %c\n", optopt);
			return -1;
		}

	}

	if (optind != argc)
		return -1;

	return 0;
}

static int 
_initiate(void)
{
	SSL_load_error_strings();
	SSL_library_init();

	switch (_g_sslproto) {

	case SSL_VER_2:
		_g_sslctx = SSL_CTX_new(SSLv2_method());
		break;
	case SSL_VER_3:
		_g_sslctx = SSL_CTX_new(SSLv3_method());
		break;
	case SSL_VER_23:
		_g_sslctx = SSL_CTX_new(SSLv23_method());
		break;
	case TLS_VER_1:
		_g_sslctx = SSL_CTX_new(TLSv1_method());
		break;
	default:
		return -1;
	}

	if (!_g_sslctx)
		return -1;

	return 0;
}


static void
_release(void)
{
	if (_g_sslctx) {
		SSL_CTX_free(_g_sslctx);
	}
	_g_sslctx = NULL;
}

int 
main(int argc, char **argv)
{
	SSL *ssl;
	char des[1024] = {0};
	STACK_OF(SSL_CIPHER) *sk;
	SSL_CIPHER *cipher;
	int n = 0;
	int i = 0;

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}	

	if (_initiate()) {
		_release();
		return -1;
	}

	ssl = SSL_new(_g_sslctx);

	sk = SSL_get_ciphers(ssl);
	n = sk_SSL_CIPHER_num(sk);
	printf("have %d ciphers\n", n);
	for (i = 0; i < n; i++) {
		cipher = sk_SSL_CIPHER_value(sk, i);
		SSL_CIPHER_description(cipher, des, 1023);
		printf("[%d] %s\n", i, des);
	}

	SSL_free(ssl);

	_release();
	
	return 0;
}















