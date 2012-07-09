/*
 *
 *
 */

#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/crypto.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <signal.h>
#include <sys/resource.h>

#define CLIENT_MAX	(40 * 1024 + 3)
#define NOFILE_MAX	(40 * 1024 + 3)
#define BUFLEN		1024
#define POLL_TIME	(1024 * 1024)

struct ssl_socket {
	int fd;
	SSL *ssl;
};

static unsigned short g_svr_port = 1433;
static struct ssl_socket g_conns[CLIENT_MAX];
static struct pollfd g_polls[CLIENT_MAX];
static int g_conn_num = 0;
static SSL_CTX *g_ssl_ctx;

static int g_begin;
static int g_end;
static int g_nconns = 0;
static long long g_nbytes = 0;
static int g_stop = 0;

static void _usage(void)
{
	printf("ssl_server [options]\n\n");
	printf("-p\tserver port\n");
	printf("-h\tshow help message\n");
}

static int _parse_cmd(int argc, char **argv)
{
	char c;
	int port;
	char optstr[] = ":p:h";

	opterr = 0;
	while ( (c = getopt(argc, argv, optstr)) != -1) {
	
		switch (c) {

		case 'p':
			port = atoi(optarg);
			if (port < 1 || port > 65535) {
				printf("port <1 - 65535>\n");
				return -1;
			}
			g_svr_port = port;
			break;

		case 'h':
			return -1;

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
	

	return 0;
}

static void _sig_int(int signo)
{
	if (signo == SIGINT) {
		printf("user stopped!\n");
		g_stop = 1;
	}
}

static int _init(void)
{
	int i;
	struct rlimit rlim;

	SSL_load_error_strings();
	SSLeay_add_ssl_algorithms();

        g_ssl_ctx = SSL_CTX_new(SSLv23_server_method());
	if (!g_ssl_ctx) {
		printf("SSL_CTX_new error\n");
		return -1;
	}

        if (SSL_CTX_use_certificate_file(g_ssl_ctx, "./server.crt", SSL_FILETYPE_PEM) <= 0) {
                printf("Unable to use certificate file\n");
                return -1;;
        }

        if (SSL_CTX_use_PrivateKey_file(g_ssl_ctx, "./server.key", SSL_FILETYPE_PEM) <= 0) {
                printf("Unable to use private key file\n");
                return -1;
        }

        if (!SSL_CTX_check_private_key(g_ssl_ctx))  {
                printf("Check private key failed\n");
                return -1;
        }

        SSL_CTX_set_verify(g_ssl_ctx, SSL_VERIFY_NONE, NULL);

	for (i = 0; i < CLIENT_MAX; i++) {
		g_conns[i].fd = -1;
		g_conns[i].ssl = SSL_new(g_ssl_ctx);
		if (!g_conns[i].ssl) {
			printf("SSL_new error\n");
			return -1;
		}
	}

	signal(SIGINT, _sig_int);

	rlim.rlim_cur = NOFILE_MAX;
	rlim.rlim_max = NOFILE_MAX;

	if (setrlimit(RLIMIT_NOFILE, &rlim)) {
		printf("rlimit error: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}

static void _cleanup(void)
{
	int i;

	for (i = 0; i < CLIENT_MAX; i++) {
		if (g_conns[i].fd > 0)
			close(g_conns[i].fd);
		if (g_conns[i].ssl)
			SSL_free(g_conns[i].ssl);
	}

	if (g_ssl_ctx)
		SSL_CTX_free(g_ssl_ctx);
}

static int _create_svr_socket(void)
{
	struct sockaddr_in svraddr;
	int fd;	
	int sockopt;
	long fdflags;

	memset(&svraddr, 0, sizeof(svraddr));
	svraddr.sin_family = AF_INET;
	svraddr.sin_port = htons(g_svr_port);
	svraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		printf("socket error: %s\n", strerror(errno));
		return -1;
	}

	/* reuse address */
	sockopt = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt))) {
		printf("setsockopt SO_REUSEADDR error: %s\n", strerror(errno));
		return -1;
	}

	/* listen socket non-block */
	fdflags = fcntl(fd, F_GETFL, 0);
	fdflags |= O_NONBLOCK;
	fcntl(fd, F_SETFL, fdflags);

	if (bind(fd, (struct sockaddr*)&svraddr, sizeof(svraddr))) {
		printf("bind error: %s\n", strerror(errno));
		return -1;
	}

	if (listen(fd, 1024)) {
		printf("listen error: %s\n", strerror(errno));
		return -1;
	}

	return fd;
}

static int _do_ssl_accept(int fd)
{
	int status = 0;
	struct ssl_socket *sk;
	
	sk = &g_conns[fd];
	sk->fd = fd;
	if (!sk->ssl) {
		printf("g_conns[%d] no ssl\n", fd);
		return -1;
	}
	
	if(SSL_get_verify_result(sk->ssl) != X509_V_OK) {
              	printf("SSL_verify_result error\n");
		SSL_clear(sk->ssl);
               	close(sk->fd);
		sk->fd = -1;
		return -1;
       	}

       	if (SSL_set_fd(sk->ssl, fd) != 1) {
        	printf("SSL_set_fd failed\n");
		SSL_clear(sk->ssl);
		close(sk->fd);
		sk->fd = -1;
		return -1;
	}

        SSL_set_mode(sk->ssl, SSL_MODE_AUTO_RETRY);

        status = SSL_accept(sk->ssl);
        if (status < 0) {
               	printf("ssl accept error\n");
		SSL_clear(sk->ssl);
		close(sk->fd);
		sk->fd = -1;
		return -1;
        }
        else if (status == 0) {
               	printf("handshake error........\n");
		SSL_clear(sk->ssl);
		close(sk->fd);
		sk->fd = -1;
		return -1;
        }
	
	g_nconns ++;
	g_conn_num++;
	return 0;
}

static int _do_accept(int fd)
{
	int clifd;
	struct sockaddr_in cliaddr;
	socklen_t clilen = sizeof(cliaddr);

	while (1) {
		clifd = accept(fd, (struct sockaddr*)&cliaddr, &clilen);
		if (clifd < 0) {
			if (errno == EINTR)
				continue;
			if (errno == EAGAIN)
				break;
			printf("accept error: %s\n", strerror(errno));
			return -1;
		}

		if (clifd >= CLIENT_MAX) {
			printf("too many client connect to SSL server\n");
			close(clifd);
			return -1;
		}

		if (g_conns[clifd].fd > 0) {
			printf("%d fd is not closed\n", g_conns[clifd].fd);
			return -1;
		}

		_do_ssl_accept(clifd);
	}
	
	return 0;
}

static int _do_read(int fd)
{
	char buf[BUFLEN] = {0};
	int n;
	struct ssl_socket *sk;
	
	if (fd < 0)
		return -1;

	sk = &g_conns[fd];	

	if (sk->fd < 0)
		return -1;

	n = SSL_read(sk->ssl, buf, BUFLEN - 1);
	if (n > 0) {
		g_nbytes += n;
	}
	else {
//		printf("client closed\n\n");

//		SSL_shutdown(sk->ssl);
		SSL_clear(sk->ssl);

		close(sk->fd);
		sk->fd = -1;
		g_conn_num--;
	}
		
	return 0;
}

static int _fill_poll_set(int fd) 
{
	int i, j;
	
	g_polls[0].fd = fd;
	g_polls[0].events = POLLIN;
	
	j = 1;
	for (i = 0; i < CLIENT_MAX; i++) {
		if (g_conns[i].fd > 0) {
			g_polls[j].fd = g_conns[i].fd;
			g_polls[j].events = POLLIN;
			j++;
		}
	}

	return j;
}

static int _do_loop(int fd)
{
	int n, i;
	int nfds;

	if (fd < 0)
		return -1;

	g_begin = time(NULL);
	while (!g_stop) {

		nfds = _fill_poll_set(fd);

		n = poll(g_polls, nfds, POLL_TIME);
		if (n < 0) {
			if (errno == EINTR)
				continue;
			printf("select error: %s\n", strerror(errno));
			break;
		}

		if (n == 0)
			continue;

		/* have client connect */
		if (g_polls[0].revents & POLLIN) {
			_do_accept(fd);
		}

		/* have client data */
		for (i = 1; i < nfds; i++) {
			if (g_polls[i].revents & POLLIN) {
				_do_read(g_polls[i].fd);
			}
		}

	}
	g_end = time(NULL);	

	return 0;
}

int main(int argc, char **argv)
{
	int fd;
	int intval;

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	_init();

	fd = _create_svr_socket();
	if (fd < 0)
		return -1;
	
	_do_loop(fd);

	_cleanup();

	intval = g_end - g_begin;
	printf("total %d connections\n", g_nconns);
	printf("total %lld bytes recv\n", g_nbytes);

	return 0;
}

