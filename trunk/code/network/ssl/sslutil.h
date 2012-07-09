/**
 *	@file	sslutil.h
 *
 *	@brief	SSL utilities, like check certificate.
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_SSLUTIL_H
#define FZ_SSLUTIL_H

#define SSLUTIL_BUFLEN		1024
#define SSLUTIL_NAMLEN		64

/**
 *	X509/PKCS12 certificate description
 */
typedef struct sslutil_certinfo {
	int	version;			/* x509 version */
	char	serial[SSLUTIL_NAMLEN];		/* serial number */
	char	sigalgo[SSLUTIL_NAMLEN];	/* signature algorithm */

	char	issuer[SSLUTIL_BUFLEN];		/* CA issuer */
	char	subject[SSLUTIL_BUFLEN];	/* subject */
	char	email[SSLUTIL_BUFLEN];		/* the email address */

	char	startday[SSLUTIL_NAMLEN];	/* start day */
	char	endday[SSLUTIL_NAMLEN];		/* end day */

	char	subpkey[SSLUTIL_NAMLEN];	/* public key of certificate */
	char	extension[SSLUTIL_BUFLEN];	/* extension infomation */
} sslutil_certinfo_t;


/**
 *	public/private RSA/DSA key type 
 */
typedef enum {
	SSLUTIL_PRIVATE_RSA = 0,
	SSLUTIL_PUBLIC_RSA,
	SSLUTIL_PRIVATE_DSA,
	SSLUTIL_PUBLIC_DSA,
} sslutil_ktype_t;

/**
 *	certificate type: PEM, DER, PKCS12
 */
typedef enum {
	SSLUTIL_CERT_PEM = 0 ,
	SSLUTIL_CERT_DER,
	SSLUTIL_CERT_PKCS12,
} sslutil_ctype_t;


/**
 *	RSA/DSA private/publice key description
 */
typedef struct sslutil_keyinfo {
	int	type;			/* the key type: see above */
	char	name[SSL_NAMELEN];	/* the key name: see above */
	int	bits;			/* the key bits */
	int	exponent;		/* the key exponent */
	int	module;			/* the key module */
} sslutil_keyinfo_t;


/**
 *	Get the information of certificate @certfile and store
 *	into @sc.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
sslutil_certificate_info(sslutil_certinfo_t *sc, 
			 const char *certfile, int type);


/**
 *	Get the information of key file @keyfile and store into @sc
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
sslutil_key_info(sslutil_keyinfo_t *sk, const char *keyfile, int type);


/**
 *	Convert ASN1_STRING @astr to ACSII string and stored in @buf,
 *	the buffer @buf length is @len
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
sslutil_a2c_string(ASN1_STRING *astr, char *buf, int len);


/**
 *	Convert ASN1_INTEGER @ai to ACSII string and stored in @buf,
 *	the buffer @buf length is @len
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
sslutil_a2c_integer(ASN1_INTEGER *ai, char *buf, int len);


/**
 *	Convert X509_NAME @nm to ACSII string and stored in @buf,
 *	the buffer @buf length is @len
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
sslutil_a2c_name(X509_NAME *nm, char *buf, int len);

/**
 *	Convert ASN1_TIME @atm to ACSII string and stored in @buf,
 *	the buffer @buf length is @len
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
sslutil_a2c_time(ASN1_TIME *atm, char *buf, int len);



#endif /* end of FZ_  */

