/**
 *	@file	certset.h
 *
 *	@brief	certificate set structure and API.
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_CERTSET_H
#define FZ_CERTSET_H

#include <sys/param.h>

#include "cblist.h"
#include "ssl_util.h"
#include "proxy_common.h"

/**
 *	certificate type.
 */
typedef enum {
	CERT_X509,
	CERT_PKCS12,
} cert_type_e;

/**
 *	certificate structure.
 */
typedef struct cert {
	cert_type_e	type;		/* local certificate type */
	union {
		struct {
			char	cert[PATH_MAX]; /* X509 certificate */
			char	pkey[PATH_MAX]; /* private key */
		}x509;
		char	pkcs12[PATH_MAX];/* PKCS12 file */
	};
} cert_t;

/**
 *	CA certificate.
 */
typedef struct cacert {
	char		cert[PATH_MAX];
	cblist_t	list;
} cacert_t;

/**
 *	certificate set for SSL client/server
 */
typedef struct certset {
	char		name[MAX_NAME];
	char		domain[MAX_NAME];/* domain name */
	cert_t		cert;		/* local cert */
	cblist_t	intcalist;
	int		nintca;
	cblist_t	calist;
	int		nca;
	char		crl[PATH_MAX];
	cblist_t	list;		/* list into proxy */
	int		refcnt;
} certset_t;

extern cacert_t * 
cacert_alloc(const char *cafile);

extern int 
cacert_free(cacert_t *cacert);

extern certset_t * 
certset_alloc(void);

extern certset_t *  
certset_clone(certset_t *cert);

extern int 
certset_free(certset_t *cert);

extern int 
certset_print(const certset_t *cert, const char *prefix);

extern ssl_ctx_t * 
certset_alloc_ctx(const certset_t *cert, ssl_sd_e side);


#endif /* end of FZ_CERTSET_H */


