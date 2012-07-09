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
#include <sys/select.h>
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

#define CLIENT_MAX	(40 * 1024)
#define NOFILE_MAX	(10 * 1024)
#define BUFLEN	1024

struct ssl_socket {
	int fd;
	SSL *ssl;
};

static unsigned short g_svr_port = 1433;
static struct ssl_socket g_conns[CLIENT_MAX];
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

static int _do_ssl_accept(int fd, struct ssl_socket *sk)
{
	int status = 0;

	if (!fd || ! sk)
		return -1;

	if (sk ->fd > 0)
		return -1;

	printf("ssl accept fd is %d\n", fd);

	sk->fd = fd;
	
	if (!sk->ssl) {
		printf("No ssl\n");
		close(sk->fd);
		sk->fd = -1;
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
               	printf("SSL_accept error\n");
		SSL_clear(sk->ssl);
		close(sk->fd);
		sk->fd = -1;
		return -1;
        }
        else if (status == 0) {
               	printf("SSL handshake error........\n");
		SSL_clear(sk->ssl);
		close(sk->fd);
		sk->fd = -1;
		return -1;
        }
	
	return 0;
}

static int _do_accept(int fd)
{
	int clifd;
	int i;
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

		if (g_conn_num >= CLIENT_MAX) {
			printf("too many client connect to SSL server\n");
			close(clifd);
			return -1;
		}

		if (clifd >= NOFILE_MAX - 1) {
			printf("too many file descripts, close it\n");
			close(clifd);
			continue;
		}

//		printf("a client connect to me, fd is %d\n", clifd);

		for (i = 0; i < CLIENT_MAX; i++) {
			if (g_conns[i].fd < 0) {
				if (_do_ssl_accept(clifd, &g_conns[i]))
					return -1;
				g_conn_num ++;
				g_nconns++;
				break;
			}
		}
	}
	
	return 0;
}

static int _do_read(struct ssl_socket *sk)
{
	char buf[BUFLEN] = {0};
	int n;
	
	if (sk->fd < 0)
		return -1;

	n = SSL_read(sk->ssl, buf, BUFLEN - 1);
	if (n > 0) {
//		printf("read from client: %s\n", buf);
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

static int _do_loop(int fd)
{
	fd_set rset;
	int maxfd1;
	int n, i;
	struct timeval tv;

	if (fd < 0)
		return -1;

	g_begin = time(NULL);
	while (!g_stop) {
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		maxfd1 = 0;

		/* set FDSET */
		FD_ZERO(&rset);
		for (i = 0; i < CLIENT_MAX; i++) {
			if (g_conns[i].fd > 0) {
//				printf("add %d to FD_SET\n", g_conns[i].fd);
				FD_SET(g_conns[i].fd, &rset);
				if (g_conns[i].fd >= maxfd1)
					maxfd1 = g_conns[i].fd + 1;
			}
		}
		
//		printf("add %d to FD_SET\n", fd);
		FD_SET(fd, &rset);
		if (fd >= maxfd1)
			maxfd1 = fd + 1;

		n = select(maxfd1, &rset, NULL, NULL, &tv);
		if (n < 0) {
			if (errno == EINTR)
				continue;
			printf("select error: %s\n", strerror(errno));
			break;
		}

		if (n == 0)
			continue;

		/* have client connect */
		if (FD_ISSET(fd, &rset)) {
			_do_accept(fd);
		}	

		/* have client data */
		for (i = 0; i < CLIENT_MAX; i++) {

			if ( g_conns[i].fd > 0 && FD_ISSET(g_conns[i].fd, &rset)) {
				_do_read(&g_conns[i]);
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

