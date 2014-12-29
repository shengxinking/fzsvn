/**
 *	@file	sslsvr.c
 *	@brief	SSL server program, it's a simple SSL server just to 
 *		print the Client's data sending to it.
 *	@author	Forrest.zhang
 */

#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <sys/poll.h>

#include "ip_addr.h"
#include "sock_util.h"
#include "ssl_util.h"

#define	MAX_SSLFD	128
#define	MAX_CA		32
#define	MAX_DCK		32
#define	MAX_INTMEDCA	32

#ifndef	BUFLEN
#define	BUFLEN		1024
#endif

#define	FLOW(level, fmt, args...)			\
	({						\
	if (level < _g_dbglvl)				\
		printf("<%d>: "fmt, level, ##args);	\
	})


struct ssl_fd;
typedef	int		(*event_cb)(struct ssl_fd *sfd);

typedef struct ssl_fd {
	int		fd;
	SSL		*ssl;
	int		events;
	event_cb	cb;
	char		inbuf[BUFLEN];
	int		inlen;
	char		outbuf[BUFLEN];
	int		outlen;
	int		reneg;		/* in renegotiate */
	int		reneg_cnt;	/* renegotiate count */
} ssl_fd_t;

/* the SSL domain name, certificate file and private key file */
typedef struct ssl_dck {
	char		domain[256];
	char		certfile[BUFLEN];
	char		keyfile[BUFLEN];
	char		password[BUFLEN];
} ssl_dck_t;

static ip_port_t	_g_svraddr;
static ssl_ctx_t	*_g_sslctx;
static int		_g_proto = SSL_V23;
static int		_g_pfs;
static ssl_fd_t		_g_sslfds[MAX_SSLFD];
static ssl_dck_t	_g_dcks[MAX_DCK];
static int		_g_dck_cnt;
static char		_g_cafiles[MAX_CA][BUFLEN];
static int		_g_cafile_cnt;
static char		_g_intmedcerts[MAX_INTMEDCA][BUFLEN];
static int		_g_intmedcert_cnt;
static char		_g_crlfile[BUFLEN];
static char		_g_ciphers[BUFLEN];
static int		_g_nonblock;
static int		_g_renegotiate;
static int		_g_renegotiate_cnt;
static int 		_g_dbglvl;
static int 		_g_stop;

static int		_do_ssl_accept(ssl_fd_t *sfd);
static int		_do_ssl_handshake(ssl_fd_t *sfd);
static int		_do_ssl_renegotiate(ssl_fd_t *sfd);
static int		_do_ssl_recv(ssl_fd_t *sfd);
static int		_do_ssl_send(ssl_fd_t *sfd);
static int		_do_ssl_shutdown(ssl_fd_t *sfd);


/**
 *	Show usage of program.
 *
 *	No return value.
 */
static void 
_usage(void)
{
	printf("ssl_server <options>\n\n");
	printf("\t-A <IP:port>\tserver address\n");
	printf("\t-p <2|3|10|11|12>: SSL protocol\n");
	printf("\t\t\t2 is SSLv2\n");
	printf("\t\t\t3 is SSLv3\n");
	printf("\t\t\t10 is TLSv1.0\n");
	printf("\t\t\t11 is TLSv1.1\n");
	printf("\t\t\t12 is TLSv1.2\n");
	printf("\t\t\t0 is all protocols\n");
	printf("\t-c <domain:certfile:keyfile:password>: certificate information\n");
	printf("\t\t\tdomain: domain name\n");
	printf("\t\t\tcertfile: certificate file\n");
	printf("\t\t\tkeyfile: private keyfile\n");
	printf("\t\t\tpassword: private keyfile PEM password\n");
	printf("\t-C <file>\tCA certificate file\n");
	printf("\t-I <file>\tIntermedia certificate file\n");
	printf("\t-L <file>\tCRL certificate file\n");
	printf("\t-f\t\tenable Forward security\n");
	printf("\t-a <ciphers>\tSSL algorithm(cipher-list) string\n");
	printf("\t-r <N>\t\tDo renegotiate times after recv data\n");
	printf("\t-R\t\tenable client renegotiate\n");
	printf("\t-n\t\tusing nonblock mode\n");
	printf("\t-d <0-4>\t\tdebug level <0-4>\n");
	printf("\t-h\tshow help message\n");
}

/**
 *	Parse DCK format string to Domain/Certificate/PrivateKey/Password 
 *	strings and stored in @dck.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_parse_dck(const char *dck)
{
	char *buf;
	char *ptr;
	char *ptr1;
	char *domain = NULL;
	char *keyfile = NULL;
	char *certfile = NULL;
	char *password = NULL;
	
	if (_g_dck_cnt >= MAX_DCK) {
		printf("too many DCK exceed %d\n", MAX_DCK);
		return -1;
	}

	if (!dck) {
		printf("invalid argument\n");
		return -1;
	}

	buf = strdup(dck);
	if (!buf) {
		printf("strdup failed\n");
		return -1;
	}

	/* find domain */
	ptr1 = buf;
	ptr = strchr(ptr1, ':');
	if (ptr) {
		domain = ptr1;
		*ptr = 0;
		ptr1 = ptr + 1;
	}
	else {
		printf("invalid DCK format string\n");
		free(buf);
		return -1;
	}

	/* find certificate */
	ptr = strchr(ptr1, ':');
	if (ptr) {
		certfile = ptr1;
		*ptr = 0;
		ptr1 = ptr + 1;
	}
	else {
		printf("invalid DCK format string\n");
		free(buf);
		return -1;
	}

	/* find private key */
	ptr = strchr(ptr1, ':');
	if (ptr) {
		keyfile = ptr1;
		*ptr = 0;
		ptr1 = ptr + 1;
	}
	else {
		printf("invalid DCK format string\n");
		free(buf);
		return -1;
	}

	/* get password */
	password = ptr1;

	/* check certificate file exist */
	if (certfile[0] && access(certfile, F_OK)) {
		printf("certificate file %s not accessable\n", certfile);
		free(buf);
		return -1;
	}

	/* check private key file exist */
	if (keyfile[0] && access(keyfile, F_OK)) {
		printf("certificate file %s not accessable\n", certfile);
		free(buf);
		return -1;
	}

	if (certfile[0] && !keyfile[0]) {
		printf("certificate %s no private key\n", certfile);
		free(buf);
		return -1;
	}
	
	if (keyfile[0] && !certfile[0]) {
		printf("private key %s no certficate\n", keyfile);
		free(buf);
		return -1;
	}
	
	if (password[0] && !keyfile[0]) {
		printf("password no private key\n");
		free(buf);
		return -1;
	}

	strcpy(_g_dcks[_g_dck_cnt].domain, domain);
	strcpy(_g_dcks[_g_dck_cnt].certfile, certfile);
	strcpy(_g_dcks[_g_dck_cnt].keyfile, keyfile);
	strcpy(_g_dcks[_g_dck_cnt].password, password);
	_g_dck_cnt++;

	free(buf);

	return 0;
}

/**
 *	Parse the command line arguments, and stored them into local variables
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_parse_cmd(int argc, char **argv)
{
	char c;
	char optstr[] = ":A:p:c:C:I:L:fa:r:Rnd:h";

	opterr = 0;
	while ( (c = getopt(argc, argv, optstr)) != -1) {
	
		switch (c) {
			
		case 'A':
			if (ip_port_from_str(&_g_svraddr, optarg)) {
				printf("invalid server address %s\n", optarg);
				return -1;
			}
			break;

		case 'p':
			_g_proto = atoi(optarg);

			if (_g_proto == 0)
				_g_proto = SSL_V23;
			else if (_g_proto == 2)
				_g_proto = SSL_V2;
			else if (_g_proto == 3)
				_g_proto = SSL_V3;
			else if (_g_proto == 10)
				_g_proto = SSL_V10;
			else if (_g_proto == 11)
				_g_proto = SSL_V11;
			else if (_g_proto == 12)
				_g_proto = SSL_V12;
			else {
				printf("invalid SSL protocol %s\n", optarg);
				return -1;
			}

			break;

		case 'c':
			if (_parse_dck(optarg)) {
				return -1;
			}
			break;

		case 'C':
			if (access(optarg, F_OK)) {
				printf("invalid CA certificate %s\n", optarg);
				return -1;
			}
			strcpy(_g_cafiles[_g_cafile_cnt], optarg);
			_g_cafile_cnt++;
			break;
			
		case 'I':
			if (access(optarg, F_OK)) {
				printf("invalid Intermedia certificate %s", optarg);
				return -1;
			}
			strcpy(_g_intmedcerts[_g_intmedcert_cnt], optarg);
			_g_intmedcert_cnt++;
			break;

		case 'L':
			if (access(optarg, F_OK)) {
				printf("invalid CRL certificate %s\n", optarg);
				return -1;
			}
			strcpy(_g_crlfile, optarg);
			break;	

		case 'f':
			_g_pfs = 1;
			break;

		case 'a':
			strcpy(_g_ciphers, optarg);
			break;
			
		case 'r':
			_g_renegotiate_cnt = atoi(optarg);
			if (_g_renegotiate_cnt < 0) {
				printf("invalid renegotiate number\n");
				return -1;
			}
			break;

		case 'R':
			_g_renegotiate = 1;
			break;

		case 'n':
			_g_nonblock = 1;
			break;

		case 'd':
			_g_dbglvl = atoi(optarg);
			if (_g_dbglvl < 0 || _g_dbglvl > 4) {
				printf("invalid debug level %s\n", optarg);
				return -1;
			}
			break;

		case 'h':
			_usage();
			exit(0);

		case ':':
			printf("option %c need parameter\n", optopt);
			return -1;
		
		case '?':
			printf("unknowed option %c\n", optopt);
			return -1;
		}
	}

	if (optind < argc)
		return -1;
	
	if (_g_svraddr.family == 0)
		return -1;

	if (_g_dck_cnt < 1)
		return -1;

	return 0;
}

static ssl_fd_t * 
_ssl_fd_alloc(void)
{
	int i;
	ssl_fd_t *sfd;

	for (i = 0; i < MAX_SSLFD; i++) {
		sfd = &_g_sslfds[i];
		if (sfd->fd < 1) {
			memset(sfd, 0, sizeof(*sfd));
			return sfd;
		}
	}

	return NULL;
}

static ssl_fd_t * 
_ssl_fd_find(int fd)
{
	int i;
	ssl_fd_t *sfd;

	for (i = 0; i < MAX_SSLFD; i++) {
		sfd = &_g_sslfds[i];
		if (sfd->fd == fd)
			return sfd;
	}

	return NULL;
}

static int 
_ssl_fd_free(ssl_fd_t *sfd)
{
	if (sfd->ssl) {
		ssl_free(sfd->ssl);
		FLOW(1, "client %d ssl freed\n", sfd->fd);
		sfd->ssl = NULL;
	}

	if (sfd->fd > 0) {
		close(sfd->fd);
		FLOW(1, "client %d closed\n", sfd->fd);
		sfd->fd = -1;
	}

	sfd->cb = NULL;
	sfd->events = 0;
	memset(sfd->inbuf, 0, sizeof(sfd->inbuf));
	sfd->inlen = 0;
	memset(sfd->outbuf, 0, sizeof(sfd->outbuf));
	sfd->outlen = 0;

	return 0;
}

static void _sig_stop(int signo)
{
	printf("\n\nReceive stop signal\n\n");
	_g_stop = 1;
}

/**
 *	Initiate global resource used in program.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_initiate(void)
{
	int i;
	int fd;
	ssl_fd_t *sfd;
	ssl_ctx_t *sc;
	ssl_dck_t *dck;

	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT, _sig_stop);

	for (i = 0; i < _g_dck_cnt; i++) {
		sc = ssl_ctx_alloc(SSL_SD_SERVER);
		if (!sc) {
			printf("alloc ssl_ctx failed\n");
			return -1;
		}
		printf("%d, create ssl_ctx %p\n", i, sc);

		ssl_ctx_set_protocol(sc, _g_proto);
		
		ssl_ctx_set_pfs(sc, _g_pfs);

		dck = &_g_dcks[i];
	
		if (dck->domain[0]) {
			if (ssl_ctx_set_domain(sc, dck->domain)) {
				printf("set domain name %s failed\n", dck->domain);
				return -1;
			}
		}

		if (dck->certfile[0]) {
			if (ssl_ctx_load_cert(sc, dck->certfile, dck->keyfile, 
					      dck->password)) 
			{
				printf("load certfile %s failed\n", dck->certfile);
				return -1;
			}
		}

		if (!_g_sslctx) {
			_g_sslctx = sc;
		}
		else {
			ssl_ctx_add_sni(_g_sslctx, sc);
		}
	}

	/* load intermedia CA */
	for (i = 0; i < _g_intmedcert_cnt; i++) {
		if (ssl_ctx_load_intmedcert(_g_sslctx, _g_intmedcerts[i]))
			return -1;
	}

	/* load CA */
	for (i = 0; i < _g_cafile_cnt; i++) {
		printf("load ca file %s\n", _g_cafiles[i]);
		if (ssl_ctx_load_cacert(_g_sslctx, _g_cafiles[i]))
			return -1;
	}

	/* load CRL */
	if (_g_crlfile[0]) {
		if (ssl_ctx_load_crl(_g_sslctx, _g_crlfile))
			return -1;
	}

	/* set ciphers */
	if (_g_ciphers[0]) {
		if (ssl_ctx_set_ciphers(_g_sslctx, _g_ciphers))
			return -1;
	}

	/* set renegotiate */
	if (_g_renegotiate) {
		if (ssl_ctx_set_renegotiate(_g_sslctx, 1))
			return -1;
	}

	fd = sk_tcp_server(&_g_svraddr, 1, 0);
	if (fd < 0)
		return -1;

	sfd = _ssl_fd_alloc();
	if (!sfd)
		return -1;

	sfd->fd = fd;
	sfd->events = POLLIN;
	sfd->cb = _do_ssl_accept;

	return 0;
}

/**
 *	Free global resources used in program.
 *
 *	No return.
 */
static void 
_release(void)
{
	int i;
	ssl_fd_t *sfd;

	for (i = 0; i < MAX_SSLFD; i++) {
		sfd = &_g_sslfds[i];
		_ssl_fd_free(sfd);
	}

	if (_g_sslctx)
		ssl_ctx_free(_g_sslctx);

}

/**
 *	Accept client ssl connection, socket fd into @_g_conns.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_do_ssl_accept(ssl_fd_t *sfd)
{
	int fd;
	SSL *ssl;
	ssl_wt_e wait;
	ssl_fd_t *clisfd;
	ip_port_t cliaddr;
	ip_port_t svraddr;
	char ipstr1[IP_STR_LEN];
	char ipstr2[IP_STR_LEN];

	fd = sk_tcp_accept(sfd->fd, &cliaddr, &svraddr);
	if (fd < 0) {
		printf("_accept error: %s\n", strerror(errno));
		return -1;
	}

	FLOW(1, "client %d accepted(%s->%s)\n", fd, 
	     ip_port_to_str(&cliaddr, ipstr1, IP_STR_LEN), 
	     ip_port_to_str(&svraddr, ipstr2, IP_STR_LEN));

	if (_g_nonblock)
		sk_set_nonblock(fd, 1);

	ssl = ssl_alloc(_g_sslctx);
	if (!ssl) {
		FLOW(1, "client %d alloc ssl failed\n", fd);
		close(fd);
		return -1;
	}

	if (ssl_accept(ssl, fd, &wait)) {
		FLOW(1, "client %d ssl-accept failed: %s\n", 
		     fd, ssl_get_errstr(ssl_get_error(ssl)));
		close(fd);
		return -1;
	}
	
	clisfd = _ssl_fd_alloc();
	if (!clisfd) {
		FLOW(1, "client %d can't get free slot\n", fd);
		close(fd);
		ssl_free(ssl);
		return -1;
	}

	clisfd->fd = fd;
	clisfd->ssl = ssl;
	clisfd->reneg_cnt = _g_renegotiate_cnt;
	if (wait) {
		clisfd->events = (wait == SSL_WT_READ) ? POLLIN : POLLOUT;
		clisfd->cb = _do_ssl_handshake;
		FLOW(1, "client %d ssl-accept need wait\n", fd);
	}
	else {
		clisfd->events = POLLOUT;
		clisfd->cb = _do_ssl_recv;
		FLOW(1, "client %d ssl-accept success\n", fd);
	}
	
	return 0;
}

static int 
_do_ssl_handshake(ssl_fd_t *sfd)
{
	ssl_wt_e wait;

	if (ssl_handshake(sfd->ssl, &wait)) {
		FLOW(1, "client %d handshake failed: %s\n", sfd->fd, 
		     ssl_get_errstr(ssl_get_error(sfd->ssl)));
		_ssl_fd_free(sfd);
		return -1;
	}

	if (wait == SSL_WT_NONE) {
		if (sfd->reneg) {
			sfd->reneg_cnt--;
			sfd->events = POLLOUT;
			sfd->cb = _do_ssl_renegotiate;
		}
		else {
			sfd->events = POLLIN;
			sfd->cb = _do_ssl_recv;
		}
		FLOW(1, "client %d handshake success\n", sfd->fd);
	}
	else {
		sfd->events = (wait == SSL_WT_READ) ? POLLIN : POLLOUT;
		//FLOW(1, "client %d handshake need wait\n", sfd->fd);
	}

	return 0;
}

static int 
_do_ssl_renegotiate(ssl_fd_t *sfd)
{
	ssl_wt_e wait;

	if (sfd->reneg_cnt < 1) {
		sfd->reneg = 0;
		sfd->events = POLLOUT;
		sfd->cb = _do_ssl_send;
		return 0;
	}

	/* call reneg */
	if (ssl_renegotiate(sfd->ssl, &wait)) {
		FLOW(1, "client %d reneg(%d) failed\n", 
		     sfd->reneg_cnt, sfd->fd);
		_ssl_fd_free(sfd);
		return -1;
	}

	/* check finished */
	if (wait == SSL_WT_NONE) {
		FLOW(1, "client %d renegotate(%d) success\n", 
		     sfd->fd, sfd->reneg_cnt);
		sfd->reneg_cnt--;
		sfd->events = POLLOUT;
		if (sfd->reneg_cnt < 1) {
			sfd->cb = _do_ssl_send;
		}
	}
	else {
		sfd->events = (wait == SSL_WT_READ) ? POLLIN : POLLOUT;
		sfd->cb = _do_ssl_handshake;
		FLOW(1, "client %d reneg(%d) need wait\n", 
		     sfd->fd, sfd->reneg_cnt);
	}

	return 0;
}

static int 
_do_ssl_recv(ssl_fd_t *sfd)
{
	int n;
	int len;
	char *ptr;
	int closed;
	int handshake;

	do {
		ptr = sfd->inbuf + sfd->inlen;
		len = sizeof(sfd->inbuf) - sfd->inlen - 1;
		n = ssl_recv(sfd->ssl, ptr, len, &closed, &handshake);		
		if (n < 0) {
			FLOW(1, "client %d recv data failed\n", sfd->fd);
			_ssl_fd_free(sfd);
			return -1;
		}
		
		if (n == 0) {
			if (handshake) {
				FLOW(1, "client %d recv handshake\n", sfd->fd);
				sfd->events = POLLOUT;
				sfd->cb = _do_ssl_handshake;
			}
			if (closed) {
				FLOW(1, "client %d recv shutdown\n", sfd->fd);
				sfd->events = POLLOUT;
				sfd->cb = _do_ssl_shutdown;
			}
			return 0;
		}
		
		FLOW(1, "client %d recv %d bytes: %s\n", sfd->fd, n, ptr);

		sfd->inlen += n;
	} while (SSL_pending(sfd->ssl));

	/* recv finished, need send recved data to peer */	
	memcpy(sfd->outbuf, sfd->inbuf, sfd->inlen);
	sfd->outlen = sfd->inlen;
	sfd->inlen = 0;

	sfd->events = POLLOUT;
	if (sfd->reneg_cnt > 0) {
		sfd->reneg = 1;
		sfd->cb = _do_ssl_renegotiate;
	}
	else {
		sfd->cb = _do_ssl_send;
	}

	return 0;
}

static int 
_do_ssl_send(ssl_fd_t *sfd)
{
	int n;
	int len;
	char *ptr;
	
	if (sfd->outlen < 1) {
		sfd->events = POLLIN;
		sfd->cb = _do_ssl_recv;
	}

	ptr = sfd->outbuf;
	len = sfd->outlen;
	n = ssl_send(sfd->ssl, ptr, len);
	if (n < 0) {
		FLOW(1, "client %d send %d bytes failed\n", sfd->fd, len);
		_ssl_fd_free(sfd);
		return -1;
	}
	
	if (n != len) {
		FLOW(1, "client %d send %d bytes blocked(%d)\n", sfd->fd, len, n);
		ptr = sfd->outbuf + n;
		memmove(sfd->outbuf, ptr, len - n);
		sfd->outlen = len - n;
		return 0;
	}

	FLOW(1, "client %d send %d bytes: %s\n", sfd->fd, len, ptr);
	sfd->events = POLLIN;
	sfd->cb = _do_ssl_shutdown;

	if (sfd->reneg)
		sfd->reneg = 0;

	return 0;
}

static int 
_do_ssl_shutdown(ssl_fd_t *sfd)
{
	int n;
	int closed;
	int buflen;
	ssl_wt_e wait;
	char buf[BUFLEN];

	if (sfd->ssl) {
		if (ssl_shutdown(sfd->ssl, &wait)) {
			FLOW(1, "client %d shutdown failed\n", sfd->fd);
			_ssl_fd_free(sfd);
			return -1;
		}

		if (wait) {
			FLOW(1, "client %d shutdown need wait\n", sfd->fd);
			sfd->events = POLLIN;
		}
		else {
			ssl_free(sfd->ssl);
			FLOW(1, "client %d shutdown success\n", sfd->fd);
			FLOW(1, "client %d ssl freed\n", sfd->fd);
			sfd->ssl = NULL;
			sfd->events = POLLIN;
		}
	}
	else {
		buflen = sizeof(buf);
		memset(buf, 0, buflen);
		n  = sk_recv(sfd->fd, buf, buflen, &closed);
		if (n == 0 && closed) {
			FLOW(1, "client %d recv closed\n", sfd->fd);
			_ssl_fd_free(sfd);
			return 0;
		}

		if (n < 0)
			FLOW(1, "client %d recv error\n", sfd->fd);
		else
			FLOW(1, "client %d recv data after shutdown\n", sfd->fd);

		_ssl_fd_free(sfd);
		return -1;
	}

	return 0;
}

static int 
_fill_poll_fd(struct pollfd *pfds, int npfd)
{
	int i;
	int n;
	ssl_fd_t *sfd;
	
	n = 0;
	for (i = 0; i < MAX_SSLFD; i++) {
		sfd = &_g_sslfds[i];
		if (sfd->fd < 1)
			continue;
		pfds[n].fd = sfd->fd;
		pfds[n].events = sfd->events;
		pfds[n].revents = 0;		
		n++;
	}

	return n;
}

/**
 *	Main loop function, it accept SSL connections, recv SSl data.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_do_loop(void)
{
	int i;
	int n;
	int npfd;
	ssl_fd_t *sfd;
	struct pollfd pfds[MAX_SSLFD];

	while (!_g_stop) {
		
		npfd = _fill_poll_fd(pfds, MAX_SSLFD);
		
		n = poll(pfds, npfd, 1);
		if (n < 0) {
			if (errno != EINTR) {
				printf("poll failed: %s\n", strerror(errno));
				break;
			}
			continue;
		}
		if (n == 0) 
			continue;

		//FLOW(1, "poll get %d events\n", n);
		
		for (i = 0; i < npfd; i++) {
			sfd = _ssl_fd_find(pfds[i].fd);
			if (!sfd) {
				printf("not found fd %d\n", pfds[i].fd);
				continue;
			}

			if (pfds[i].revents & POLLERR) {
				FLOW(1, "client %d get error event\n", pfds[i].fd);
				_ssl_fd_free(sfd);
				continue;
			}

			if (pfds[i].revents & sfd->events) {
				sfd->cb(sfd);
			}
		}
	}
	
	return 0;
}

/**
 *	The main function of SSL server.
 *
 *	Return 0 if success, -1 on error.
 */
int 
main(int argc, char **argv)
{
	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_initiate()) {
		return -1;
	}

	_do_loop();

	_release();

	return 0;
}



