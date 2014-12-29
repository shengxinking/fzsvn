/**
 *	@file	x509_util.c
 *
 *	@brief	X509 certificate APIs.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2014-08-09
 */

#include "x509_util.h"

X509 * 
x509_load(const char *file)
{

}

int 
x509_verify(const X509 *x509, const EVP_PKEY *rsa)
{

}

int 
x509_is_selfsigned(const X509 *x509)
{

}

int 
x509_is_CA(const X509 *x509)
{

}

int 
x509_get_issuer(const X509 *x509, char *buf, size_t len)
{

}

int 
x509_get_subject(const X509 *x509, char *buf, size_t len)
{

}

int 
x509_get_extension(const X509 *x509, const char *name, char *buf, size_t len)
{

}

