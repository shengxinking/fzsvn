/**
 *	@file	ssl_client.c:	
 *
 *	@brief	a simple SSL client, just do connect to SSL server and closed.
 *
 *	@author	Forrest.zhang
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/poll.h>

#include "ip_addr.h"
#include "sock_util.h"
#include "ssl_util.h"

#define	MAX_CA		32
#define	MAX_DCK		32

#ifndef	BUFLEN
#define	BUFLEN		4096
#endif

#define	FLOW(level, fmt, args...)				\
	({							\
		if (level < _g_dbglvl)				\
			printf("<%d>: "fmt, level, ##args);	\
	})

typedef struct ssl_fd {
	int		fd;
	SSL		*ssl;
	int		events;
	char		inbuf[BUFLEN];
	int		inlen;
	char		outbuf[BUFLEN];
	int		outlen;
} ssl_fd_t;

/* the SSL domain name, certificate file and private key file */
typedef struct ssl_dck {
	char		domain[256];
	char		certfile[BUFLEN];
	char		keyfile[BUFLEN];
	char		password[BUFLEN];
} ssl_dck_t;

static ip_port_t	_g_svraddr; 
static ssl_ctx_t	*_g_sslctx = NULL;
static int		_g_proto = SSL_V23;
static ssl_dck_t	_g_dck;
static char		_g_cafiles[BUFLEN][MAX_CA];
static int		_g_cafile_cnt;
static char		_g_crlfile[BUFLEN];
static char		_g_ciphers[BUFLEN];
static char		_g_msg[BUFLEN];
static int		_g_msglen;
static int		_g_nonblock;
static int		_g_renegotiate;
static int		_g_renegotiate_cnt;
static int		_g_dbglvl;


static void 
_usage(void)
{
	printf("ssl_client <options>\n");
	printf("\t-A <IP:port>\tserver address\n");
	printf("\t-p <2|3|10|11|12>\tSSL protocol:\n");
	printf("\t\t\t2 is SSLv2\n");
	printf("\t\t\t3 is SSLv3\n");
	printf("\t\t\t10 is TLSv1.0\n");
	printf("\t\t\t11 is TLSv1.1\n");
	printf("\t\t\t12 is TLSv1.2\n");
	printf("\t\t\t0 is all protocols\n");
	printf("\t-c <domain:certfile:keyfile:password>:\n");
	printf("\t\tdomain: domain name\n");
	printf("\t\tcertfile: certificate file\n");
	printf("\t\tkeyfile: private keyfile\n");
	printf("\t\tpassword: private keyfile PEM password\n");
	printf("\t-C <CAfile>\tCA certificate file\n");
	printf("\t-L <CRLfile>\tCRL certificate file\n");
	printf("\t-a <cipher>\tSSL cipher-list string\n");
	printf("\t-m <string>\tmessage send to server\n");
	printf("\t-r <N>\t\trenegotiate number\n");
	printf("\t-R\tenable server renegotiate\n");
	printf("\t-n\t\tuse non-block mode\n");
	printf("\t-d <0-4>\tdebug level\n");
	printf("\t-h\tshow help info\n");
}

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

	strcpy(_g_dck.domain, domain);
	strcpy(_g_dck.certfile, certfile);
	strcpy(_g_dck.keyfile, keyfile);
	strcpy(_g_dck.password, password);

	free(buf);

	return 0;
}

static int 
_parse_cmd(int argc, char **argv)
{
	char c;
	const char optstr[] = ":A:p:c:C:L:a:m:r:Rnd:h";

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

		case 'L':
			if (access(optarg, F_OK)) {
				printf("invalid CRL certificate %s\n", optarg);
				return -1;
			}
			strcpy(_g_crlfile, optarg);
			break;

		case 'a':
			strcpy(_g_ciphers, optarg);
			break;
			
		case 'm':
			strcpy(_g_msg, optarg);
			_g_msglen = strlen(_g_msg);
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
			printf("option %c miss argument\n", optopt);
			return -1;
		
		case '?':
			printf("unknow option %c\n", optopt);
			return -1;
		}

	}

	if (optind != argc)
		return -1;

	if (_g_svraddr.family == 0)
		return -1;

	return 0;
}

static int 
_initiate(void)
{
	int i;

	/* alloc SSL context object */
	_g_sslctx = ssl_ctx_alloc(SSL_SD_CLIENT);
	if (!_g_sslctx)
		return -1;
	
	ssl_ctx_set_protocol(_g_sslctx, _g_proto);

	/* set domain name */
	if (_g_dck.domain[0]) {
		if (ssl_ctx_set_domain(_g_sslctx, _g_dck.domain)) {
			return -1;
		}
	}

	/* load client certificate if exist */
	if (_g_dck.certfile[0]) {
		if (ssl_ctx_load_cert(_g_sslctx, _g_dck.certfile, 
				      _g_dck.keyfile, _g_dck.password))
		{
			return -1;
		}
	}

	/* load CA certificate if exist */
	for (i = 0; i < _g_cafile_cnt; i++) {
		if (ssl_ctx_load_cacert(_g_sslctx, _g_cafiles[i]))
			return -1;
	}

	/* load CRL certificate if exist */
	if (strlen(_g_crlfile) > 0) {
		if (ssl_ctx_load_crl(_g_sslctx, _g_crlfile))
			return -1;
	}

	/* set renegotiate number */
	if (_g_renegotiate) {
		if (ssl_ctx_set_renegotiate(_g_sslctx, 1))
			return -1;
	}
	
	/* set ciphers */
	if (_g_ciphers[0]) {
		if (ssl_ctx_set_ciphers(_g_sslctx, _g_ciphers))
			return -1;
	}

	return 0;
}

static void
_release(void)
{
	if (_g_sslctx)
		ssl_ctx_free(_g_sslctx);
}

static int 
_ssl_fd_free(ssl_fd_t *sfd)
{
	if (sfd->ssl) {
		ssl_free(sfd->ssl);
		FLOW(1, "client %d ssl freed\n", sfd->fd);
	}

	if (sfd->fd > 0) {
		close(sfd->fd);
		FLOW(1, "client %d closed\n", sfd->fd);
	}

	return 0;
}

static int 
_do_tcp_connect(ssl_fd_t *sfd)
{
	int n;
	int fd;
	int wait;
	struct pollfd pfd;

	/* connect to server */
	fd = sk_tcp_client_nb(&_g_svraddr, NULL, 0, &wait);
	if (wait == 0) {
		sfd->fd = fd;
		FLOW(1, "client %d connect success\n", fd);
		return 0;
	}

	FLOW(1, "client %d connect need wait\n", fd);

	while (wait) {
		pfd.fd = fd;
		pfd.events = POLLOUT;
		pfd.revents = 0;
		
		n = poll(&pfd, 1, 1);
		if (n < 0) {
			printf("connect poll error: %s\n", strerror(errno));
			close(fd);
			return -1;
		}

		if (n == 0)
			continue;

		if (pfd.revents & POLLERR) {
			FLOW(1, "client %d connect poll error\n", fd);
			close(fd);
			return -1;
		}

		if (sk_is_connected(fd)) {
			FLOW(1, "client %d connect failed\n", fd);
			close(fd);
			return -1;
		}

		break;
	}

	FLOW(1, "client %d connect success\n", fd);

	sfd->fd = fd;

	return 0;
}

static int 
_do_ssl_handshake(ssl_fd_t *sfd, ssl_wt_e wait)
{
	int n;
	int ret;
	struct pollfd pfd;

	while (wait) {
		pfd.fd = sfd->fd;	
		pfd.events = (wait == SSL_WT_READ) ? POLLIN : POLLOUT;
		pfd.revents = 0;

		n = poll(&pfd, 1, 1);
		if (n < 0) {
			printf("handshake poll error\n");
			return -1;
		}
		else if (n == 0)
			continue;

		if (pfd.revents & POLLERR) {
			FLOW(1, "client %d handshake poll failed\n", sfd->fd);
			return -1;
		}

		ret = ssl_handshake(sfd->ssl, &wait);
		if (ret < 0) {
			FLOW(1, "client %d handshake failed: %s\n", sfd->fd, 
			     ssl_get_errstr(ssl_get_error(sfd->ssl)));
			return -1;
		}		
	}

	FLOW(1, "client %d handshake success\n", sfd->fd);

	return 0;
}

static int 
_do_ssl_connect(ssl_fd_t *sfd)
{
	SSL *ssl;
	ssl_wt_e wait;
	
	if (_do_tcp_connect(sfd))
		return -1;

	ssl = ssl_alloc(_g_sslctx);
	if (!ssl) {
		FLOW(1, "client %d alloc ssl failed\n", sfd->fd);
		return -1;
	}
	sfd->ssl = ssl;

	if (ssl_connect(ssl, sfd->fd, &wait)) {
		FLOW(1, "client %d ssl-connect failed\n", sfd->fd);
		return -1;
	}

	if (wait == SSL_WT_NONE) {
		FLOW(1, "client %d ssl-connect success\n", sfd->fd);
		return 0;
	}

	FLOW(1, "client %d ssl-connect need wait\n", sfd->fd);

	if (_do_ssl_handshake(sfd, wait) < 0)
		return -1;

	FLOW(1, "client %d ssl-connect success\n", sfd->fd);

	return 0;
}

static int 
_do_ssl_renegotiate(ssl_fd_t *sfd)
{
	int i;
	int ret;
	ssl_wt_e wait;

	if (_g_renegotiate_cnt < 1)
		return 0;

	for (i = 0; i < _g_renegotiate_cnt; i++) {
		ret = ssl_renegotiate(sfd->ssl, &wait);
		if (ret < 0) {
			FLOW(1, "client %d active-renegotiate(%d) failed %s\n", 
			     sfd->fd, i, 
			     ssl_get_errstr(ssl_get_error(sfd->ssl)));
			return -1;
		}

		if (wait == SSL_WT_NONE) {
			FLOW(1, "client %d renegotiate(%d) success\n", sfd->fd, i);
			continue;
		}

		if (_do_ssl_handshake(sfd, wait) < 0) {
			FLOW(1, "client %d active-renegotiate(%d) failed %s\n", 
			     sfd->fd, i, 
			     ssl_get_errstr(ssl_get_error(sfd->ssl)));
			return -1;
		}

		FLOW(1, "client %d active-renegotiate(%d) success\n", sfd->fd, i);
	}

	return 0;
}

static int 
_do_ssl_send(ssl_fd_t *sfd)
{
	int n;
	int len;
	char *ptr;
	struct pollfd pfd;
	
	if (_g_msglen < 1)
		return 0;

	memmove(sfd->outbuf, _g_msg, _g_msglen);
	sfd->outlen = _g_msglen;

	ptr = sfd->outbuf;
	len = sfd->outlen;
	do {
		pfd.fd = sfd->fd;
		pfd.events = POLLOUT;
		pfd.revents = 0;

		n = poll(&pfd, 1, 1);
		if (n < 0) {
			printf("send poll error: %s\n", strerror(errno));
			return -1;
		}

		if (n == 0)
			continue;

		if (pfd.revents & POLLERR) {
			FLOW(1, "client %d send poll error\n", sfd->fd);
			return -1;
		}

		n = ssl_send(sfd->ssl, ptr, len);
		if (n < 0) {
			FLOW(1, "client %d send %d bytes failed\n", sfd->fd, len);
			return -1;
		}

		if (n != len) {
			FLOW(1, "client %d send %d bytes blocked(%d)\n", sfd->fd, len, n);
		}

		ptr += n;
		len -= n;
	} while (len > 0);

	FLOW(1, "client %d send %d bytes: %s\n", sfd->fd, sfd->outlen, sfd->outbuf);

	return 0;
}

static int 
_do_ssl_recv(ssl_fd_t *sfd)
{
	int n;
	int len;
	int ret;
	char *ptr;
	int closed;
	int handshake;
	struct pollfd pfd;

	if (_g_msglen < 1)
		return 0;

	ptr = sfd->inbuf;
	len = sizeof(sfd->inbuf) - sfd->inlen;
	while (sfd->inlen < _g_msglen) {
		pfd.fd = sfd->fd;
		pfd.events = POLLIN;
		pfd.revents = 0;

		n = poll(&pfd, 1, 1);
		if (n < 0) {
			printf("recv poll error: %s\n", strerror(errno));
			return -1;
		}
		else if (n == 0)
			continue;

		if (pfd.revents & POLLERR) {
			FLOW(1, "client %d recv poll error\n", sfd->fd);
			return -1;
		}

		n = ssl_recv(sfd->ssl, ptr, len, &closed, &handshake);
		if (handshake) {
			FLOW(1, "client %d recv handshake\n", sfd->fd);
			ret = _do_ssl_handshake(sfd, SSL_WT_READ);
			if (ret < 0)
				return -1;
			continue;
		}
		if (closed) {
			FLOW(1, "client %d recv shutdown\n", sfd->fd);
			return 0;			
		}

		if (n < 0) {
			FLOW(1, "client %d recv data failed\n", sfd->fd);
			return -1;
		}
		
		ptr += n;
		len -= n;
		sfd->inlen += n;		
	}

	FLOW(1, "client %d recv %d types: %s\n", sfd->fd, sfd->inlen, sfd->inbuf);	
	len = sfd->inlen;
	memset(sfd->inbuf, 0, sizeof(sfd->inbuf));
	sfd->inlen = 0;

	return len;
}

static int 
_do_ssl_shutdown(ssl_fd_t *sfd)
{
	int n;
	ssl_wt_e wait;
	struct pollfd pfd;

	if (ssl_shutdown(sfd->ssl, &wait)) {
		FLOW(1, "client %d shutdown failed\n", sfd->fd);
		return -1;
	}
	
	if (wait == 0) {
		FLOW(1, "client %d shutdown success\n", sfd->fd);
		return 0;
	}

	FLOW(1, "client %d shutdown need wait\n", sfd->fd);

	while (wait) {
		pfd.fd = sfd->fd;
		pfd.events = (wait == SSL_WT_READ) ? POLLIN : POLLOUT;
		pfd.revents = 0;

		n = poll(&pfd, 1, 1);
		if (n < 0) {
			printf("shutdown poll error: %s\n", strerror(errno));
			return -1;
		}
		else if (n == 0)
			continue;

		if (pfd.revents & POLLERR) {
			FLOW(1, "client %d shutdown poll error\n", sfd->fd);
			return -1;
		}

		if (ssl_shutdown(sfd->ssl, &wait)) {
			FLOW(1, "client %d shutdown failed\n", sfd->fd);
			return -1;
		}
	}

	FLOW(1, "client %d shutdown success\n", sfd->fd);

	return 0;
}

static int 
_do_client_nb(void)
{
	ssl_fd_t sfd;

	memset(&sfd, 0, sizeof(sfd));

	if (_do_ssl_connect(&sfd)){
		_ssl_fd_free(&sfd);
		return -1;
	}

	if (_do_ssl_renegotiate(&sfd)) {
		_ssl_fd_free(&sfd);
		return -1;
	}

	if (_do_ssl_send(&sfd)) {
		_ssl_fd_free(&sfd);
		return -1;
	}

	if (_do_ssl_recv(&sfd)) {
		_ssl_fd_free(&sfd);
		return -1;
	}

	if (_do_ssl_shutdown(&sfd)) {
		_ssl_fd_free(&sfd);
		return -1;
	}

	_ssl_fd_free(&sfd);

	return 0;
}

static int 
_do_client(void)
{
	int i;
	int fd;
	int ret;
	SSL *ssl;
	int closed;
	int handshake;
	ssl_wt_e wait;	
	char buf[BUFLEN];
	ip_port_t cliaddr;
	char ipstr1[IP_STR_LEN];
	char ipstr2[IP_STR_LEN];

	fd = sk_tcp_client(&_g_svraddr, &cliaddr, 0);
	if (fd < 0) {
		FLOW(1, "client connect to server(%s) failed\n", 
				ip_port_to_str(&_g_svraddr, ipstr1, IP_STR_LEN));
		return -1;
	}

	FLOW(1, "client %d connect to server(%s->%s)\n", fd, 
			ip_port_to_str(&cliaddr, ipstr1, IP_STR_LEN),
			ip_port_to_str(&_g_svraddr, ipstr2, IP_STR_LEN));

	ssl = ssl_alloc(_g_sslctx);
	if (ssl == NULL) {
		FLOW(1, "client %d alloc ssl failed\n", fd);
		close(fd);
		return -1;
	}

	if (ssl_connect(ssl, fd, &wait)) {
		FLOW(1, "client %d connect failed: %s\n", fd, 
		     ssl_get_errstr(ssl_get_error(ssl)));
		close(fd);
		ssl_free(ssl);
		FLOW(1, "client %d closed\n", fd);
		return -1;
	}

	FLOW(1, "client %d ssl connect success\n", fd);

	/* active renegotiate */
	for (i = 0; i < _g_renegotiate_cnt; i++) {
		ret = ssl_renegotiate(ssl, &wait);
		if (ret < 0 || wait) {
			FLOW(1, "client %d active-renegotiate failed:(%s)\n", fd, 
			     ssl_get_errstr(ssl_get_error(ssl)));
			ssl_free(ssl);
			FLOW(1, "client %d ssl freed\n", fd);
			close(fd);
			FLOW(1, "client %d closed\n", fd);
			return -1;
		}

		FLOW(1, "client %d active-renegotiate success\n", fd);
	}

	if (_g_msglen) {
		ret = ssl_send(ssl, _g_msg, _g_msglen);
		if (ret < 0 || wait) {
			FLOW(1, "client %d send %d bytes failed\n", fd, _g_msglen);
			ssl_free(ssl);
			FLOW(1, "client %d ssl freed\n", fd);
			close(fd);
			FLOW(1, "client %d closed\n", fd);
			return -1;
		}
		FLOW(1, "client %d send %d bytes: %s\n", fd, ret, _g_msg);

		while (1) {
			memset(buf, 0, sizeof(buf));
			ret = ssl_recv(ssl, buf, sizeof(buf), &closed, &handshake);
			if (ret < 0) {
				FLOW(1, "client %d recv %d bytes failed: %s\n", 
				     fd, _g_msglen, 
				     ssl_get_errstr(ssl_get_error(ssl)));
				
				close(fd);
				FLOW(1, "client %d closed\n", fd);
				ssl_free(ssl);
				return -1;
			}

			if (closed) {
				FLOW(1, "client %d ssl shutdown\n", fd);
				break;
			}

			if (handshake) {
				FLOW(1, "client %d passive-renegotiate\n", fd);
				continue;
			}
			FLOW(1, "client %d recv %d bytes: %s\n", fd, ret, buf);
		}
	}

	do {
		if (ssl_shutdown(ssl, &wait)) {
			FLOW(1, "client %d shutdown failed\n", fd);
			break;
		}
	} while (wait == 1);

	ssl_free(ssl);
	FLOW(1, "client %d ssl freed\n", fd);
	close(fd);
	FLOW(1, "client %d closed\n", fd);

	return 0;
}

#include <openssl/err.h>

int 
main(int argc, char **argv)
{
	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_initiate()) {
		_release();
		return -1;
	}

	if (_g_nonblock)
		_do_client_nb();
	else
		_do_client();

	_release();
	
	return 0;
}











