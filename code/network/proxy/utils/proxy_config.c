/**
 *	@file	proxy_config.c
 *
 *	@brief	proxy config APIs.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date
 */

#include <ctype.h>

#include "thread.h"
#include "policy.h"
#include "svrpool.h"
#include "proxy_debug.h"
#include "proxy_config.h"

typedef enum cfg_section {
	CFG_PROXY,
	CFG_CERTSET,
	CFG_LISTENER,
	CFG_SVRPOOL,
	CFG_POLICY,
} cfg_section_e;

typedef struct cfg_pctx {
	cfg_section_e	section;
	int		lineno;
	void		*cfg;
} cfg_pctx_t;

static int 
_cfg_atoi(const char *str)
{
	const char *ptr;

	if (!str)
		ERR_RET(-1, "invalid argument\n");

	ptr = str;
	
	/* skipped -N/+N */
	if (*ptr == '-' || *ptr == '+')
		ptr++;

	/* check all char is digit */
	while (*ptr && isdigit(*ptr))
		ptr++;

	if (*ptr)
		return -1;

	return atoi(str);
}

static int 
_cfg_check_proxy(proxy_cfg_t *pycfg)
{
	if (!pycfg)
		ERR_RET(-1, "invalid argument\n");

	return 0;
}

static int 
_cfg_check_certset(certset_t *cert)
{
	if (!cert)
		ERR_RET(-1, "invalid argument\n");

	if (!cert->name[0])
		ERR_RET(-1, "no svrcert name\n");
	
	if (cert->cert.type == CERT_X509) {
		if (!cert->cert.x509.cert[0])
			ERR_RET(-1, "no certificate in svrcert(%s)\n",
				cert->name);
		if (!cert->cert.x509.pkey[0])
			ERR_RET(-1, "no privatekey in svrcert(%s)\n", 
				cert->name);
	}
	else if (cert->cert.type == CERT_PKCS12) {
		if (!cert->cert.pkcs12[0])
			ERR_RET(-1, "no pkcs12 in svrcert(%s)\n",
				cert->name);
	}
	else {
		ERR_RET(-1, "invalid certificate type %d in svrcert(%s)\n", 
			cert->cert.type, cert->name);
	}

	return 0;
}

static int 
_cfg_check_listener(listener_cfg_t *ltncfg)
{	
	if (!ltncfg)
		ERR_RET(-1, "invalid argument\n");

	if (!ltncfg->name[0])
		ERR_RET(-1, "no listener name\n");

	if (ltncfg->address.family == 0)
		ERR_RET(-1, "no listener address in listener(%s)\n", 
			ltncfg->name);

	if (ltncfg->ssl) {
		if (!ltncfg->cert) 
			ERR_RET(-1, "no certificate in listener(%s)\n",
				ltncfg->name);
	}

	return 0;
}

static int 
_cfg_check_svrpool(svrpool_t *sp)
{
	if (!sp)
		ERR_RET(-1, "invalid argument\n");

	if (!sp->cfg.name[0])
		ERR_RET(-1, "no svrpool name provide\n");

	if (sp->nserver < 1)
		ERR_RET(-1, "no server in svrpool (%s)\n", sp->cfg.name);

	return 0;
}

static int 
_cfg_check_policy(policy_cfg_t *plcfg)
{
	if (!plcfg)
		ERR_RET(-1, "invalid argument\n");

	if (!plcfg->name[0]) 
		ERR_RET(-1, "no policy name provide\n");

	if (plcfg->listener == 0)
		ERR_RET(-1, "no listener in policy (%s)\n", plcfg->name);

	if (!plcfg->svrpool)
		ERR_RET(-1, "not svrpool in policy (%s)\n", plcfg->name);

	return 0;
}

static int 
_cfg_check_section(cfg_pctx_t *pctx)
{
	if (!pctx)
		ERR_RET(-1, "invalid argument");

	switch (pctx->section) {
		
	case CFG_PROXY:
		return _cfg_check_proxy(pctx->cfg);

	case CFG_CERTSET:
		return _cfg_check_certset(pctx->cfg);

	case CFG_LISTENER:
		return _cfg_check_listener(pctx->cfg);

	case CFG_SVRPOOL:
		return _cfg_check_svrpool(pctx->cfg);

	case CFG_POLICY:
		return _cfg_check_policy(pctx->cfg);
	}

	ERR_RET(-1, "invalid section %d\n", pctx->section);
}

static int 
_cfg_parse_proxy(cfg_pctx_t *pctx, proxy_t *py, 
		 const char *kw, const char **args, int narg)
{
	int val;
	proxy_cfg_t *pycfg;

	pycfg = pctx->cfg;
	if (!pycfg)
		ERR_RET(-1, "not proxy config in parse context\n");

	if (strcmp(kw, "worker") == 0) {
		if (narg != 1) 
			ERR_RET(-1, "line %d: too many arguments for <worker>\n", 
				pctx->lineno);
		
		val = _cfg_atoi(args[0]);
		if (val < 1 || val > 40) 
			ERR_RET(-1, "line %d: argument exceed range(1-40)\n", 
				pctx->lineno);
		pycfg->nworker = val;
	}
	else if (strcmp(kw, "nice") == 0) {
		if (narg != 1)
			ERR_RET(-1, "line %d: too many argument for <nice>\n", 
				pctx->lineno);
		
		val = _cfg_atoi(args[0]);
		if (val < -20 || val > 19)
			ERR_RET(-1, "line %d: argument exceed range(-20 - 19)\n",
				pctx->lineno);
		pycfg->nice = val;
	}
	else if (strcmp(kw, "naccept") == 0) {
		if (narg != 1)
			ERR_RET(-1, "line %d: too many argument for <naccept>\n", 
				pctx->lineno);
		
		val = _cfg_atoi(args[0]);
		if (val < 1 || val > 16)
			ERR_RET(-1, "line %d: argument exceed range(1-16)\n",
				pctx->lineno);
		pycfg->naccept = val;
	}
	else if (strcmp(kw, "use_splice") == 0) {
		if (narg != 1)
			ERR_RET(-1, "line %d: too many arguments for <use_splice>\n",
				pctx->lineno);

		if (strcmp(args[0], "yes") == 0)
			pycfg->use_splice = 1;
		else if (strcmp(args[0], "no") == 0)
			pycfg->use_splice = 0;
		else 
			ERR_RET(-1, "line %d: argument must be yes|no\n", 
				pctx->lineno);				
	}
	else if (strcmp(kw, "use_nbsplice") == 0) {
		if (narg != 1)
			ERR_RET(-1, "line %d: too many arguments for <use_nbsplice>\n",
				pctx->lineno);

		if (strcmp(args[0], "yes") == 0)
			pycfg->use_nbsplice = 1;
		else if (strcmp(args[0], "no") == 0)
			pycfg->use_nbsplice = 0;
		else 
			ERR_RET(-1, "line %d: argument must be yes|no\n", 
				pctx->lineno);				
	}
	else if (strcmp(kw, "maxconn") == 0) {
		if (narg != 1) 
			ERR_RET(-1, "line %d: too many arguments for <maxconn>\n", 
				pctx->lineno);
		
		val = _cfg_atoi(args[0]);
		if (val < 1 || val > 5000000) 
			ERR_RET(-1, "line %d: argument exceed range(1-5000000)\n", 
				pctx->lineno);
		pycfg->maxconn = val;
	}
	else if (strcmp(kw, "bind_cpu") == 0) {
		if (narg != 1)
			ERR_RET(-1, "line %d: too many arguments for <bind_cpu>\n",
				pctx->lineno);

		if (strcmp(args[0], "yes") == 0)
			pycfg->bind_cpu = 1;
		else if (strcmp(args[0], "no") == 0)
			pycfg->bind_cpu = 0;
		else 
			ERR_RET(-1, "line %d: argument must be yes|no\n", 
				pctx->lineno);				
	}
	else if (strcmp(kw, "bind_cpu_algo") == 0) {
		if (narg != 1)
			ERR_RET(-1, "line %d: too many arguments for <bind_cpu_algo>\n",
				pctx->lineno);

		if (strcmp(args[0], "rr") == 0)
			pycfg->bind_cpu_algo = THREAD_BIND_RR;
		else if (strcmp(args[0], "odd") == 0)
			pycfg->bind_cpu_algo = THREAD_BIND_ODD;
		else if (strcmp(args[0], "even") == 0)
			pycfg->bind_cpu_algo = THREAD_BIND_EVEN;
		else 
			ERR_RET(-1, "line %d: argument must be rr|odd|even\n", 
				pctx->lineno);				
	}
	else if (strcmp(kw, "bind_cpu_ht") == 0) {
		if (narg != 1)
			ERR_RET(-1, "line %d: too many arguments for <bind_cpu_ht>\n",
				pctx->lineno);

		if (strcmp(args[0], "low") == 0)
			pycfg->bind_cpu_ht = THREAD_HT_LOW;
		else if (strcmp(args[0], "high") == 0)
			pycfg->bind_cpu_ht = THREAD_HT_HIGH;
		else if (strcmp(args[0], "full") == 0)
			pycfg->bind_cpu_ht = THREAD_HT_FULL;
		else 
			ERR_RET(-1, "line %d: argument must be yes|no\n", 
				pctx->lineno);		
	}
	else if (strcmp(kw, "debug") == 0) {
		if (narg != 1)
			ERR_RET(-1, "line %d: too many arguments for <debug>\n",
				pctx->lineno);

		val = _cfg_atoi(args[0]);
		if (val < 0 || val > 7)
			ERR_RET(-1, "line %d: argument exceed range(0-7)\n", 
				pctx->lineno);

		pycfg->debug = val;
	}
	else if (strcmp(kw, "flow") == 0) {
		if (narg != 1)
			ERR_RET(-1, "line %d: too many arguments for <flow>\n",
				pctx->lineno);

		val = _cfg_atoi(args[0]);
		if (val < 0 || val > 7)
			ERR_RET(-1, "line %d: argument exceed range(0-7)\n", 
				pctx->lineno);

		pycfg->flow = val;
	}
	else if (strcmp(kw, "http") == 0) {
		if (narg != 1)
			ERR_RET(-1, "line %d: too many arguments for <http>\n",
				pctx->lineno);

		val = _cfg_atoi(args[0]);
		if (val < 0 || val > 7)
			ERR_RET(-1, "line %d: argument exceed range(0-7)\n", 
				pctx->lineno);

		pycfg->http = val;
	}
	else if (strcmp(kw, "timestamp") == 0) {
		if (narg != 1)
			ERR_RET(-1, "line %d: too many arguments for <timestamp>\n",
				pctx->lineno);

		if (strcmp(args[0], "yes") == 0)
			pycfg->timestamp = 1;
		else if (strcmp(args[0], "no") == 0)
			pycfg->timestamp = 0;
		else 
			ERR_RET(-1, "line %d: argument must be yes|no\n", 
				pctx->lineno);				
	}
	else {
		ERR_RET(-1, "line %d: invalid keyword (%s)\n",
			pctx->lineno, kw);
	}

	return 0;
}

static int 
_cfg_parse_certset(cfg_pctx_t *pctx, proxy_t *py,
		   const char *kw, const char **args, int narg)
{
	cacert_t *ca;
	certset_t *cert;

	cert = pctx->cfg;
	
	if (strcmp(kw, "name") == 0) {
		if (narg != 1) 
			ERR_RET(-1, "line %d: wrong arguments for <name>\n", 
				pctx->lineno);
		
		if (strlen(args[0]) >= MAX_NAME)
			ERR_RET(-1, "line %d: argument exceed range(1-%d)\n",
				pctx->lineno, MAX_NAME);

		if (cert->name[0]) {
			ERR_RET(-1, "line %d: already have name(%s)\n", 
				pctx->lineno, cert->name);
		}

		if (proxy_find_certset(py, args[0]))
			ERR_RET(-1, "line %d: certificate(%s) already exist\n",
				pctx->lineno, args[0]);

		strcpy(cert->name, args[0]);
	}
	else if (strcmp(kw, "domain") == 0) {
		if (narg != 1)
			ERR_RET(-1, "line %d: wrong arguments for <certificate>\n", 
				pctx->lineno);

		if (strlen(args[0]) >= MAX_NAME)
			ERR_RET(-1, "line %d: argument exceed range(1-%d)\n",
				pctx->lineno, MAX_NAME);

		if (cert->domain[0]) 
			ERR_RET(-1, "lind %d: already have domain(%s)\n",
				pctx->lineno, cert->domain);

		strcpy(cert->domain, args[0]);
	}
	else if (strcmp(kw, "certificate") == 0) {
		if (narg != 1)
			ERR_RET(-1, "line %d: wrong arguments for <certificate>\n", 
				pctx->lineno);

		if (cert->cert.type == CERT_PKCS12)
			ERR_RET(-1, "line %d: already set pkcs12\n", 
				pctx->lineno);

		if (cert->cert.x509.cert[0])
			ERR_RET(-1, "line %d: already set certificate\n", 
				pctx->lineno);

		strcpy(cert->cert.x509.cert, args[0]);
	}
	else if (strcmp(kw, "privatekey") == 0) {
		if (narg != 1)
			ERR_RET(-1, "line %d: wrong arguments for <privatekey>\n", 
				pctx->lineno);

		if (cert->cert.type == CERT_PKCS12)
			ERR_RET(-1, "line %d: already set pkcs12\n", 
				pctx->lineno);

		if (cert->cert.x509.pkey[0])
			ERR_RET(-1, "line %d: already set privatekey\n", 
				pctx->lineno);

		strcpy(cert->cert.x509.pkey, args[0]);
	}
	else if (strcmp(kw, "pkcs12") == 0) {
		if (narg != 1)
			ERR_RET(-1, "line %d: wrong arguments for <pkcs12>\n", 
				pctx->lineno);

		if (cert->cert.type == CERT_PKCS12)
			ERR_RET(-1, "line %d: already set pkcs12\n", 
				pctx->lineno);

		if (cert->cert.x509.cert[0])
			ERR_RET(-1, "line %d: already set certificate\n", 
				pctx->lineno);

		if (cert->cert.x509.pkey[0])
			ERR_RET(-1, "line %d: already set privatekey\n", 
				pctx->lineno);

		strcpy(cert->cert.pkcs12, args[0]);
	}
	else if (strcmp(kw, "intca") == 0) {
		if (narg != 1)
			ERR_RET(-1, "line %d: wrong arguments for <intca>\n", 
				pctx->lineno);

		ca = cacert_alloc(args[0]);
		if (!ca)
			ERR_RET(-1, "alloc cacert failed\n");

		CBLIST_ADD_TAIL(&cert->intcalist, &ca->list);
		cert->nintca++;
	}
	else if (strcmp(kw, "ca") == 0) {
		if (narg != 1)
			ERR_RET(-1, "line %d: wrong arguments for <ca>\n", 
				pctx->lineno);

		ca = cacert_alloc(args[0]);
		if (!ca)
			ERR_RET(-1, "alloc cacert failed\n");

		CBLIST_ADD_TAIL(&cert->calist, &ca->list);
		cert->nca++;
	}
	else if (strcmp(kw, "crl") == 0) {
		if (narg != 1)
			ERR_RET(-1, "line %d: wrong arguments for <crl>\n", 
				pctx->lineno);

		if (cert->crl[0])
			ERR_RET(-1, "line %d: already set crl\n", 
				pctx->lineno);

		strcpy(cert->crl, args[0]);
	}
	else {
		ERR_RET(-1, "line %d: invalid keyword (%s)\n",
			pctx->lineno, kw);
	}

	return 0;
}

static int 
_cfg_parse_listener(cfg_pctx_t *pctx, proxy_t *py,
		    const char *kw, const char **args, int narg)
{
	ip_port_t address;
	certset_t *cert;
	listener_cfg_t *ltncfg;

	ltncfg = pctx->cfg;

	if (strcmp(kw, "name") == 0) {
		if (narg != 1) 
			ERR_RET(-1, "line %d: wrong arguments for <name>\n", 
				pctx->lineno);
		
		if (strlen(args[0]) >= MAX_NAME)
			ERR_RET(-1, "line %d: argument exceed range(1-%d)\n",
				pctx->lineno, MAX_NAME);

		if (ltncfg->name[0]) {
			ERR_RET(-1, "line %d: already have name(%s)\n", 
				pctx->lineno, ltncfg->name);
		}

		if (proxy_find_listener(py, args[0]))
			ERR_RET(-1, "line %d: listener(%s) already exist\n",
				pctx->lineno, args[0]);

		strcpy(ltncfg->name, args[0]);
	}
	else if (strcmp(kw, "address") == 0) {
		if (narg != 1)
			ERR_RET(-1, "line %d: wrong arguments for <name>\n", 
				pctx->lineno);

		if (ip_port_from_str(&address, args[0]))
			ERR_RET(-1, "line %d: invalid listen address(%s)\n",
				pctx->lineno, args[0]);

		ltncfg->address = address;
	}
	else if (strcmp(kw, "ssl") == 0) {
		if (narg != 1)
			ERR_RET(-1, "line %d: wrong arguments for <ssl>\n", 
				pctx->lineno);
		
		if (strcmp(args[0], "yes") == 0)
			ltncfg->ssl = 1;
		else if (strcmp(args[0], "no") == 0)
			ltncfg->ssl = 0;
		else 
			ERR_RET(-1, "line %d: argument must be yes|no\n", 
				pctx->lineno);		
	}
	else if (strcmp(kw, "certset") == 0) {
		if (narg != 1)
			ERR_RET(-1, "line %d: wrong arguments for <certificate>\n", 
				pctx->lineno);

		if (strlen(args[0]) >= PATH_MAX)
			ERR_RET(-1, "line %d: argument exceed range(1-%d)\n",
				pctx->lineno, PATH_MAX);
		
		cert = proxy_find_certset(py, args[0]);
		if (!cert)
			ERR_RET(-1, "line %d: not found certset(%s)\n",
				pctx->lineno, args[0]);

		ltncfg->cert = certset_clone(cert);
	}
	else {
		ERR_RET(-1, "line %d: invalid keyword (%s)\n",
			pctx->lineno, kw);
	}

	return 0;
}

static int 
_cfg_parse_svrpool(cfg_pctx_t *pctx, proxy_t *py, 
		   const char *kw, const char **args, int narg)
{
	int ssl;
	int val;
	server_t *svr;
	svrpool_t *sp;
	ip_port_t address;

	sp = pctx->cfg;
	if (!sp)
		ERR_RET(-1, "not svrpool config in parse context\n");

	if (strcmp(kw, "name") == 0) {
		if (narg != 1) 
			ERR_RET(-1, "line %d: wrong arguments for <name>\n", 
				pctx->lineno);
		
		if (sp->cfg.name[0])
			ERR_RET(-1, "line %d: already set name(%s)\n", 
				pctx->lineno, sp->cfg.name);

		if (proxy_find_svrpool(py, args[0])) 
			ERR_RET(-1, "line %d: svrpool %s already exist\n", 
				pctx->lineno, args[0]);

		if (strlen(args[0]) >= MAX_NAME)
			ERR_RET(-1, "line %d: argument exceed range(1-%d)\n",
				pctx->lineno, MAX_NAME);

		strcpy(sp->cfg.name, args[0]);
	}
	else if (strcmp(kw, "algo") == 0) {
		if (narg != 1)
			ERR_RET(-1, "line %d: wrong arguments for <algo>\n", 
				pctx->lineno);

		if (strcmp(args[0], "rr") == 0)
			sp->cfg.algo = SP_ALGO_RR;
		else if (strcmp(args[0], "wrr") == 0)
			sp->cfg.algo = SP_ALGO_WRR;
		else if (strcmp(args[0], "lc") == 0)
			sp->cfg.algo = SP_ALGO_LC;
		else if (strcmp(args[0], "hash") == 0)
			sp->cfg.algo = SP_ALGO_HASH;
		else 
			ERR_RET(-1, "line %d: invalid argument(%s)\n",
				pctx->lineno, args[0]);
	}
	else if (strcmp(kw, "server") == 0) {
		if (narg != 3)
			ERR_RET(-1, "line %d: wrong argument for <server>\n",
				pctx->lineno);

		val = _cfg_atoi(args[0]);
		if (val < 1 || val >= 100)
			ERR_RET(-1, "line %d: argument exceed range(1-99)\n",
				pctx->lineno);

		if (strcmp(args[1], "http") == 0)
			ssl = 0;
		else if (strcmp(args[1], "https") == 0)
			ssl = 1;
		else 
			ERR_RET(-1, "line %d: wrong argument for protocol(%s)\n", 
				pctx->lineno, args[1]);
		
		if (ip_port_from_str(&address, args[2]))
			ERR_RET(-1, "line %d: invalid server address(%s)\n",
				pctx->lineno, args[2]);

		svr = server_alloc();
		if (!svr)
			ERR_RET(-1, "line %d: alloc server failed\n", 
				pctx->lineno);
		
		svr->cfg.ssl = ssl;
		svr->cfg.weight = val;
		svr->cfg.address = address;

		CBLIST_ADD_TAIL(&sp->svrlist, &svr->list);
		sp->nserver++;
	}
	else {
		ERR_RET(-1, "line %d: invalid keyword (%s)\n",
			pctx->lineno, kw);
	}

	return 0;
}

static int 
_cfg_parse_policy(cfg_pctx_t *pctx, proxy_t *py, 
		  const char *kw, const char **args, int narg)
{
	policy_t *pl;
	svrpool_t *sp;
	listener_t *ltn;
	policy_cfg_t *plcfg;

	plcfg = pctx->cfg;

	if (strcmp(kw, "name") == 0) {
		if (narg != 1) 
			ERR_RET(-1, "line %d: wrong arguments for <name>\n", 
				pctx->lineno);
		
		if (strlen(args[0]) >= MAX_NAME)
			ERR_RET(-1, "line %d: argument exceed range(1-%d)\n",
				pctx->lineno, MAX_NAME);

		if (plcfg->name[0]) {
			ERR_RET(-1, "line %d: already have name(%s)\n", 
				pctx->lineno, plcfg->name);
		}

		if (proxy_find_policy(py, args[0]))
			ERR_RET(-1, "line %d: policy(%s) already exist\n",
				pctx->lineno, args[0]);

		strcpy(plcfg->name, args[0]);
	}
	else if (strcmp(kw, "mode") == 0) {
		if (narg != 1)
			ERR_RET(-1, "line %d: wrong arguments for <mode>\n", 
				pctx->lineno);

		if (strcmp(args[0], "reverse") == 0)
			plcfg->mode = PL_MODE_REVERSE;
		else if (strcmp(args[0], "trapt") == 0)
			plcfg->mode = PL_MODE_TRAPT;
		else if (strcmp(args[0], "tproxy") == 0)
			plcfg->mode = PL_MODE_TPROXY;
		else 
			ERR_RET(-1, "line %d: invalid argument(%s)\n",
				pctx->lineno, args[0]);
	
	}
	else if (strcmp(kw, "listener") == 0) {
		if (narg != 1)
			ERR_RET(-1, "line %d: wrong arguments for <listener>\n", 
				pctx->lineno);
		
		ltn = proxy_find_listener(py, args[0]);
		if (!ltn)
			ERR_RET(-1, "line %d: not found listener(%s)\n",
				pctx->lineno, args[0]);

		/* find listener is used by other policy */
		CBLIST_FOR_EACH(&py->pllist, pl, list) {
			if (pl->cfg.listener == ltn)
				ERR_RET(-1, "line %d: listener(%s) used by policy(%s)\n", 
					pctx->lineno, args[0], pl->cfg.name);
		}

		plcfg->listener = listener_clone(ltn);
	}
	else if (strcmp(kw, "svrpool") == 0) {
		if (narg != 1)
			ERR_RET(-1, "line %d: wrong arguments for <server_pool>\n", 
				pctx->lineno);
		
		sp = proxy_find_svrpool(py, args[0]);
		if (!sp)
			ERR_RET(-1, "line %d: not found server_pool(%s)\n",
				pctx->lineno, args[0]);

		plcfg->svrpool = svrpool_clone(sp);
	} 
	else {
		ERR_RET(-1, "line %d: invalid keyword (%s)\n",
			pctx->lineno, kw);
	}

	return 0;
}

static int 
_cfg_parse_line(cfg_pctx_t *pctx, proxy_t *py, 
		const char *kw, const char **args, int narg)
{
	policy_t *pl;
	svrpool_t *sp;
	certset_t *cert;

	listener_t *ltn;

	if (!pctx || !py || !kw || !args)
		return -1;

	if (!pctx->cfg)
		ERR_RET(-1, "parse_ctx have no cfg\n");

	if (strcmp(kw, "[proxy]") == 0) {
		if (_cfg_check_section(pctx))
			ERR_RET(-1, "previous section(%d) not finished\n", 
				pctx->section);

		pctx->section = CFG_PROXY;
		pctx->cfg = &py->cfg;
		return 0;
	}
	else if (strcmp(kw, "[certset]") == 0) {
		if (_cfg_check_section(pctx))
			ERR_RET(-1, "previouse section(%d) not finished\n", 
				pctx->section);
		cert = certset_alloc();
		if (!cert)
			ERR_RET(-1, "alloc a new certset failed\n");

		if (proxy_add_certset(py, cert)) {
			certset_free(cert);
			ERR_RET(-1, "add certset into proxy failed\n");
		}
		
		pctx->section = CFG_CERTSET;
		pctx->cfg = cert;

		return 0;
	}
	else if (strcmp(kw, "[listener]") == 0) {
		if (_cfg_check_section(pctx))
			ERR_RET(-1, "previouse section(%d) not finished\n", 
				pctx->section);
		
		ltn = listener_alloc();
		if (!ltn)
			ERR_RET(-1, "alloc a new listener failed\n");

		if (proxy_add_listener(py, ltn)) {
			listener_free(ltn);
			ERR_RET(-1, "add listener into proxy failed\n");
		}

		pctx->section = CFG_LISTENER;
		pctx->cfg = &ltn->cfg;		

		return 0;
	}
	else if (strcmp(kw, "[svrpool]") == 0) {
		if (_cfg_check_section(pctx))
			ERR_RET(-1, "previouse section(%d) not finished\n", pctx->section);
		
		sp = svrpool_alloc();
		if (!sp)
			ERR_RET(-1, "alloc a new server pool failed\n");

		if (proxy_add_svrpool(py, sp)) {
			svrpool_free(sp);
			ERR_RET(-1, "add svrpool into proxy failed\n");
		}

		pctx->section = CFG_SVRPOOL;
		pctx->cfg = sp;

		return 0;
	}
	else if (strcmp(kw, "[policy]") == 0) {
		if (_cfg_check_section(pctx))
			ERR_RET(-1, "previouse section(%d) not finished\n", pctx->section);

		pl = policy_alloc();
		if (!pl)
			ERR_RET(-1, "alloc a new policy failed\n");

		if (proxy_add_policy(py, pl)) {
			policy_free(pl);
			ERR_RET(-1, "add policy into proxy failed\n");
		}

		pctx->section = CFG_POLICY;
		pctx->cfg = &pl->cfg;

		return 0;
	}
	
	switch (pctx->section) {

	case CFG_PROXY:
		return _cfg_parse_proxy(pctx, py, kw, args, narg);

	case CFG_CERTSET:
		return _cfg_parse_certset(pctx, py, kw, args, narg);
		
	case CFG_LISTENER:
		return _cfg_parse_listener(pctx, py, kw, args, narg);

	case CFG_SVRPOOL:
		return _cfg_parse_svrpool(pctx, py, kw, args, narg);

	case CFG_POLICY:
		return _cfg_parse_policy(pctx, py, kw, args, narg);
	}
	
	ERR_RET(-1, "invalid section %d\n", pctx->section);
}

int 
cfg_load_file(proxy_t *py, const char *file)
{
	int i;
	FILE *fp;
	int lineno = 1;	
	char line[1024];
	char *delim = " \t";
	char *keyword;
	char *saveptr = NULL;
	int narg;
	const char *args[4];
	cfg_pctx_t pctx;

	fp = fopen(file, "r");
	if (!fp)
		ERR_RET(-1, "read config file %s failed\n", file);

	/* init parse context */
	pctx.section = CFG_PROXY;
	pctx.lineno = 1;
	pctx.cfg = &py->cfg;

	/* parse each line in config file */
	memset(line, 0, sizeof(line));
	while (fgets(line, sizeof(line), fp)) {
		if (line[sizeof(line) - 1] != '\n' && 
		    line[sizeof(line) - 1] != 0)
		{
			fclose(fp);
			ERR_RET(-1, "line %d too long\n", lineno);
		}
		
		if (line[strlen(line) - 1] == '\n')
			line[strlen(line) - 1] = 0;

		/* get keyword */
		keyword = strtok_r(line, delim, &saveptr);

		/* empty line */
		if (!keyword || keyword[0] == '#') {
			lineno++;
			memset(line, 0, sizeof(line));
			continue;
		}

		/* get argument */
		narg = 0;
		for (i = 0; i < 4; i++) {
			args[i] = strtok_r(NULL, delim, &saveptr);

			/* NULL or comment */
			if (args[i] == NULL || args[i][0] == '#') {
				args[i] = NULL;
				break;
			}

			narg++;
		}

		if (narg >= 4) {
			fclose(fp);
			ERR_RET(-1, "line %d too many args(>3)\n", lineno);
		}

		/* parse keyword */
		if (_cfg_parse_line(&pctx, py, keyword, args, narg)) {
			fclose(fp);
			ERR_RET(-1, "line %d parse failed\n", lineno);
		}

		lineno++;
		pctx.lineno++;
		memset(line, 0, sizeof(line));
	}

	if (_cfg_check_section(&pctx))
		ERR_RET(-1, "previous section %d not finished\n", pctx.section);

	fclose(fp);
	return 0;
}

int
cfg_write_file(proxy_t *py, const char *file)
{
	
	return 0;
}
