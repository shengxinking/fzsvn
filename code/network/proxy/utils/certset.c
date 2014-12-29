/**
 *	@file	certset.c
 *
 *	@brief	certificate set APIs.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2014-08-01
 */


#include "certset.h"
#include "proxy_debug.h"

cacert_t *
cacert_alloc(const char *cafile)
{
	cacert_t *cacert;
	
	if (!cafile)
		ERR_RET(NULL, "invalid argument\n");

	cacert = calloc(1, sizeof(*cacert));

	strncpy(cacert->cert, cafile, PATH_MAX - 1);
	CBLIST_INIT(&cacert->list);

	return cacert;
}

int 
cacert_free(cacert_t *cacert)
{
	if (!cacert)
		ERR_RET(-1, "invalid argument\n");
	
	free(cacert);

	return 0;
}

certset_t * 
certset_alloc(void)
{
	certset_t *cert;

	cert = calloc(1, sizeof(*cert));
	if (!cert)
		ERR_RET(NULL, "calloc memory for certset failed\n");
	
	CBLIST_INIT(&cert->intcalist);
	CBLIST_INIT(&cert->calist);

	return cert;
}

certset_t * 
certset_clone(certset_t *cert)
{
	if (!cert)
		ERR_RET(NULL, "invalid argument\n");

	__sync_fetch_and_add(&cert->refcnt, 1);
	
	return cert;
}

int 
certset_free(certset_t *cert)
{
	int refcnt;
	cacert_t *ca, *cabak;

	if (!cert)
		ERR_RET(-1, "invalid argument\n");

	refcnt = __sync_fetch_and_sub(&cert->refcnt, 1);
	
	if (refcnt == 0) {
		CBLIST_FOR_EACH_SAFE(&cert->intcalist, ca, cabak, list) {
			CBLIST_DEL(&ca->list);
			cacert_free(ca);
		}
		CBLIST_FOR_EACH_SAFE(&cert->calist, ca, cabak, list) {
			CBLIST_DEL(&ca->list);
			cacert_free(ca);
		}
		free(cert);
	}

	return 0;
}

int 
certset_print(const certset_t *cert, const char *prefix)
{
	cacert_t *ca;

	if (!cert)
		ERR_RET(-1, "invalid argument\n");

	printf("%scertset(%p) <%s>:\n", prefix, cert, cert->name);
	if (cert->cert.type == CERT_X509) {
		printf("%s\tcertificate:    %s\n", prefix, cert->cert.x509.cert);
		printf("%s\tprivatekey:     %s\n", prefix, cert->cert.x509.pkey);
	}
	else 
		printf("%s\tpkcs12:         %s\n", prefix, cert->cert.pkcs12);

	printf("%s\tintca number:   %d\n", prefix, cert->nintca);
	printf("%s\tintca number:   %d\n", prefix, cert->nintca);
	printf("%s\tcrl             %s\n", prefix, cert->crl);
	CBLIST_FOR_EACH(&cert->intcalist, ca, list)
		printf("%s\tintca:          %s\n", prefix, ca->cert);
	CBLIST_FOR_EACH(&cert->calist, ca, list)
		printf("%s\tca:             %s\n", prefix, ca->cert);
	
	return 0;
}

ssl_ctx_t *  
certset_alloc_ctx(const certset_t *cert, ssl_sd_e side)
{
	ssl_ctx_t *sc;
	cacert_t *ca;

	if (!cert)
		ERR_RET(NULL, "invalid argument\n");

	sc = ssl_ctx_alloc(side);
	if (!sc)
		ERR_RET(NULL, "alloc SSL ctx failed\n");

	/* load X509 certficate */
	if (cert->cert.type == CERT_X509) {
		if (ssl_ctx_load_cert(sc, cert->cert.x509.cert, 
				      cert->cert.x509.pkey, NULL))
		{
			ssl_ctx_free(sc);
			ERR_RET(NULL, "load certificate %s/%s failed\n", 
				cert->cert.x509.cert, cert->cert.x509.pkey);
		}				
	}
	else {
		ERR("not support PKCS12 now\n");
	}

	CBLIST_FOR_EACH(&cert->intcalist, ca, list) {
		if (ssl_ctx_load_intmedcert(sc, ca->cert)) {
			ssl_ctx_free(sc);
			ERR_RET(NULL, "load intca %s failed\n", ca->cert);
		}
	}

	CBLIST_FOR_EACH(&cert->calist, ca, list) {
		if (ssl_ctx_load_cacert(sc, ca->cert)) {
			ssl_ctx_free(sc);
			ERR_RET(NULL, "load intmedcert %s failed\n", ca->cert);
		}
	}

	if (cert->domain[0]) {
		if (ssl_ctx_set_domain(sc, cert->domain)) {
			ssl_ctx_free(sc);
			ERR_RET(NULL, "set domain %s failed\n", cert->domain);
		}
	}

	DBG(2, "certset(%s) alloc ssl context(%p)\n", cert->name, sc);

	return sc;
}


