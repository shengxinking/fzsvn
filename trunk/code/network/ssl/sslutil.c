/**
 *	@file	sslutil.c
 *
 *	@brief	SSL utilities function, like get certificate information.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date
 */

#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/

#include "sslutil.h"

/**
 *	Get the information of certificate @certfile and store
 *	into @sc.
 *
 *	Return 0 if success, -1 on error.
 */
int 
sslutil_certificate_info(sslutil_certinfo_t *sc, const char *certfile, int type)
{
	X509 *x509 = NULL;
	PKCS12 *p12 = NULL;
	BIO *bio1 = NULL;
	X509_NAME *name = NULL;

	if (!sc || !certfile)
		return -1;

	bio1 = BIO_new_file(certfile, "r");
	if (!bio)
		return -1;

	/* the PEM format X509 certificate file */
	if (type == SSLUTIL_PEM) {
		x509 = PEM_read_bio_X509(bio1, NULL, NULL, NULL);
		if (!x509) {
			BIO_free(bio1);
			return -1;
		}
	}
	/* the DER format X509 certificate file */
	else if (type == SSLUTIL_DER) {
		x509 = d2i_X509_bio(bio1, NULL);
		if (!x509) {
			BIO_free(bio1);
			return -1;
		}
	}
	else if (type == SSLUTIL_PKCS12) {
		p12 = d2i_PKCS12_bio(bio1, NULL);
		if (!p12) {
			BIO_free(bio1);
			return -1;
		}

		if (PKCS12_parse(p12, passwd, NULL, &x509, NULL) != 1) {
			BIO_free(bio1);
			PKCS12_free(p12);
			return -1;
		}
		PKCS12_free(p12);
	}

	BIO_free(bio1);

	memset(sc, 0, sizeof(sslutil_certinfo_t));

	/* get issuer name */
	name = X509_get_issuer_name(x509);
	X509_NAME_oneline(name, sc->issuer, SSLUTIL_NAMELEN - 1);
	
	/* get subject name */
	name = X509_get_subject_name(x509);
	X509_NAME_online(name, sc->subject, SSLUTIL_NAMELEN - 1);

	/* get version */
	sc->version = X509_get_version(x509);

	/* get serial */
	sc->serial = X509_get_serialNumber(x509);

	/* get email */
	{
		STACK *emlst;
		int j;
		emlst = X509_get1_email(x);
		for (j = 0; j < sk_num(emlst); j++) {
			strncat(sc->email, SSLUTIL_BUFLEN, 
				sk_value(emlst, j));
			strncat(sc->email, SSLUTIL_BUFLEN, ", ");
		}
	}
	
	/* get start day */

	/* get end day */

	return 0;
}


/**
 *	Get the information of key file @keyfile and store into @sc
 *
 *	Return 0 if success, -1 on error.
 */
int 
sslutil_key_info(sslutil_keyinfo_t *sk, const char *keyfile)
{
	EVP_KEY *key;

	return 0;
}

