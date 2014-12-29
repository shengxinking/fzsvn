/**
 *	@file	
 *
 *	@brief
 *	
 *	@author	Forrest.zhang	
 *
 *	@date
 */

#include "svrpool.h"
#include "proxy_debug.h"

/**
 *	Get GCD of integer @a and @b.
 *
 *	Return GCD value.
 */
static int
_gcd(int a, int b)
{
        int c;

        while ((c = a % b)) {
                a = b;
                b = c;
        }

        return b;
}

/**
 *	Calc gcd for svrpool_data @spdata.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_spdata_calc_gcd(svrpool_data_t *spdata)
{
	int i;
	int gcd = 0;
	server_cfg_t *svrcfg;

	if (unlikely(!spdata))
		ERR_RET(-1, "invalid argument\n");

        for (i = 0; i < spdata->nserver; i++) {
		svrcfg = &spdata->servers[i]->server->cfg;
                if (gcd > 0)
                        gcd = _gcd(svrcfg->weight, gcd);
                else
                        gcd = svrcfg->weight;
	}

	spdata->gcd = gcd ? gcd : 1;
     
	return 0;
}

server_t * 
server_alloc(void)
{
	server_t *svr;

	svr = calloc(1, sizeof(*svr));
	if (unlikely(!svr))
		ERR_RET(NULL, "calloc memory for server_t failed\n");

	CBLIST_INIT(&svr->list);

	return svr;
}

server_t * 
server_clone(server_t *svr)
{
	if (unlikely(!svr)) 
		ERR_RET(NULL, "invalid argument\n");
	
	__sync_fetch_and_add(&svr->refcnt, 1);

	return svr;
}

int 
server_free(server_t *svr)
{
	int refcnt;

	if (unlikely(!svr))
		ERR_RET(-1, "invalid argument\n");

	refcnt = __sync_fetch_and_sub(&svr->refcnt, 1);
	if (refcnt == 0) {
		free(svr);
	}

	return 0;
}

server_data_t * 
server_alloc_data(server_t *svr)
{
	server_data_t *svrdata;

	if (unlikely(!svr))
		ERR_RET(NULL, "invalid argument\n");

	svrdata = calloc(1, sizeof(*svrdata));
	if (unlikely(!svrdata))
		ERR_RET(NULL, "calloc memory for server data failed\n");

	CBLIST_INIT(&svrdata->ssnlist);
	svrdata->server = server_clone(svr);
	
	if (svr->cfg.ssl) {
		if (svr->cfg.cert)
			svrdata->sslctx = certset_alloc_ctx(svr->cfg.cert, SSL_SD_CLIENT);
		else
			svrdata->sslctx = ssl_ctx_alloc(SSL_SD_CLIENT);

		if (!svrdata->sslctx) {
			server_free(svr);
			free(svrdata);
			ERR_RET(NULL, "alloc ssl context failed\n");
		}
	}

	return svrdata;
}

server_data_t * 
server_clone_data(server_data_t *svrdata)
{
	if (unlikely(!svrdata))
		ERR_RET(NULL, "invalid argument\n");

	__sync_fetch_and_add(&svrdata->refcnt, 1);

	return svrdata;
}

int 
server_free_data(server_data_t *svrdata)
{
	int refcnt;

	if (unlikely(!svrdata))
		ERR_RET(-1, "invalid argument");

	refcnt = __sync_fetch_and_sub(&svrdata->refcnt, 1);

	if (refcnt == 0) {		
		server_free(svrdata->server);
		free(svrdata);	
	}

	return 0;
}

int 
server_inc_conn(server_data_t *svrdata)
{
	if (unlikely(!svrdata))
		ERR_RET(-1, "invalid argument\n");

	__sync_fetch_and_add(&svrdata->stat.nconn, 1);
	__sync_fetch_and_add(&svrdata->server->stat.nconn, 1);

	return 0;
}

int 
server_dec_conn(server_data_t *svrdata)
{
	if (unlikely(!svrdata))
		ERR_RET(-1, "invalid argument\n");

	__sync_fetch_and_sub(&svrdata->stat.nconn, 1);
	__sync_fetch_and_sub(&svrdata->server->stat.nconn, 1);

	return 0;
}

svrpool_t *
svrpool_alloc(void)
{
	svrpool_t *sp;

	sp = calloc(1, sizeof(*sp));
	if (unlikely(!sp))
		ERR_RET(NULL, "calloc memory for svrpool_t failed\n");

	CBLIST_INIT(&sp->list);
	CBLIST_INIT(&sp->svrlist);

	return sp;
}

svrpool_t * 
svrpool_clone(svrpool_t *sp)
{
	if (unlikely(!sp))
		ERR_RET(NULL, "invalid argument\n");

	__sync_fetch_and_add(&sp->refcnt, 1);
	return sp;
}

int 
svrpool_free(svrpool_t *sp)
{
	int refcnt;
	server_t *svr, *bk;

	if (unlikely(!sp))
		ERR_RET(-1, "invalid argument\n");

	refcnt = __sync_fetch_and_sub(&sp->refcnt, 1);

	if (refcnt == 0) {
		/* free server */
		CBLIST_FOR_EACH_SAFE(&sp->svrlist, svr, bk, list) {
			CBLIST_DEL(&svr->list);
			server_free(svr);
		}

		/* free svrpool */
		free(sp);
	}

	return 0;
}

int 
svrpool_print(const svrpool_t *sp, const char *prefix)
{
	const server_t *svr;
	const server_cfg_t *svrcfg;
	const svrpool_cfg_t *spcfg;
	char ipstr[IP_STR_LEN];

	if (unlikely(!sp || !prefix))
		ERR_RET(-1, "invalid argument\n");

	spcfg = &sp->cfg;
	
	printf("%ssvrpool(%p) <%s>:\n", prefix, sp, spcfg->name);
	printf("%s\talgo:           %d\n", prefix, spcfg->algo);
	printf("%s\tserver number:  %d\n", prefix, sp->nserver);
	CBLIST_FOR_EACH(&sp->svrlist, svr, list) {
		svrcfg = &svr->cfg;
		printf("%s\tserver:         %d %s %s\n", prefix, 
		       svrcfg->weight, svrcfg->ssl ? "https" : "http", 
		       ip_port_to_str(&svrcfg->address, ipstr, IP_STR_LEN));
	}

	return 0;
}

svrpool_data_t * 
svrpool_alloc_data(svrpool_t *sp)
{
	int i;
	server_t *svr;
	svrpool_data_t *spdata;

	if (unlikely(!sp))
		ERR_RET(NULL, "invalid agrument\n");

	spdata = calloc(1, sizeof(*spdata));
	if (unlikely(!spdata))
		ERR_RET(NULL, "malloc memory for svrpool data failed\n");

	i = 0;
	CBLIST_FOR_EACH(&sp->svrlist, svr, list) {
		spdata->servers[i] = server_alloc_data(svr);
		i++;
	}
	spdata->nserver = i;

	_spdata_calc_gcd(spdata);

	return spdata;
}

svrpool_data_t *
svrpool_clone_data(svrpool_data_t *spdata)
{
	if (unlikely(!spdata))
		ERR_RET(NULL, "invalid argument\n");
	
	__sync_fetch_and_add(&spdata->refcnt, 1);

	return spdata;
}

int 
svrpool_free_data(svrpool_data_t *spdata)
{
	int i;
	int refcnt;

	if (unlikely(!spdata))
		ERR_RET(-1, "invalid argument\n");

	refcnt = __sync_fetch_and_sub(&spdata->refcnt, 1);

	if (refcnt == 0) {
		/* free server_data */
		for (i = 0; i < spdata->nserver; i++) {
			printf("call free svrdata, refcnt %d\n", spdata->servers[i]->refcnt);
			server_free_data(spdata->servers[i]);
		}
		
		/* free svrpool_data */
		free(spdata);
	}

	return 0;
}

server_data_t *
svrpool_get_rp_server(svrpool_data_t *spdata)
{
	int pos;
	server_data_t *svrdata = NULL;

	if (unlikely(!spdata))
		ERR_RET(NULL, "invalid argument\n");

	/* rr algorithm */
	pos = __sync_fetch_and_add(&spdata->rrpos, 1);
	pos = pos % spdata->nserver;

	svrdata = spdata->servers[pos];

	return server_clone_data(svrdata);
} 

server_data_t * 
svrpool_get_tp_server(svrpool_data_t *spdata, ip_port_t *dest)
{
	int i;
	server_data_t *svrdata = NULL;

	if (unlikely(!spdata || !dest))
		ERR_RET(NULL, "invalid argument\n");

	for (i = 0; i < spdata->nserver; i++) {
		svrdata = spdata->servers[i];
		if (ip_port_compare(&svrdata->server->cfg.address, dest) == 0)
			break;
	}

	if (i == spdata->nserver)
		return NULL;

	return server_clone_data(svrdata);
}



