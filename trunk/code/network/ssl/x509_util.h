/**
 *	@file	x509_util.h
 *
 *	@brief	X509 certificate utils.
 *	
 *	@author	Forrest.zhang
 *
 *	@date	2014-08-09
 */

#ifndef FZ_X509_UTIL_H
#define FZ_X509_UTIL_H

#include <openssl/x509.h>

extern X509 * 
x509_load(const char *file);

extern int 
x509_verify(const X509 *x509, const EVP_PKEY *pkey);

extern int 
x509_is_selfsigned(const X509 *x509);

extern int 
x509_is_CA(const X509 *x509);

extern int 
x509_get_subject(const X509 *x509, char *buf, size_t len);

extern int 
x509_get_issuer(const X509 *x509, char *buf, size_t len);

extern int 
x509_get_extension(const X509 *x509, const char *name, char *buf, size_t len);



#endif /* end of FZ_X509_UTIL_H */

