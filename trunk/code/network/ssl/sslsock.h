/**
 *	@file	sslsock.h
 *
 *	@brief	The SSL socket APIs for create server/client SSL 
 *		socket, recv/send SSL data.
 *	
 *	@author	Forrest.zhang
 *
 *	@date	2009-08-10
 */

#ifndef FZ_SSLSOCK_H
#define FZ_SSLSOCK_H

#include <openssl/ssl.h>

/**
 *	SSL verify type
 */
enum ss_verifytype {
	SS_VERIFY_NONE = 0,
	SS_VERIFY_CLIENT,
};

/**
 *	sslsock_t object type
 */
enum ss_socktype {
	SS_CLIENT = 0,
	SS_SERVER,
};

/**
 *	sslsock error number
 */
enum ss_errtype {
	SS_ERR_NONE = 0,
	SS_ERR_NOPASS,
	SS_ERR_SOCKTYPE,
	SS_ERR_NOCERT,
	SS_ERR_MALLOC,
	SS_ERR_PARAM,
	SS_ERR_TCPCLI,
	SS_ERR_TCPSVR,
	SS_ERR_ACCEPT,
	SS_ERR_HANDSHAKE,
	SS_ERR_NOSSL,
	SS_ERR_SSL,
	SS_ERR_NEWSSL,
	SS_ERR_GETSOCKNAME,
	SS_ERR_ALGO,
	SS_ERR_NOFD,
	SS_ERR_RECV,
	SS_ERR_SEND,
};


/**
 *	SSL socket context struct.
 */
typedef struct sslsock_ctx {
	SSL_CTX	*ctx;		/* SSL_CTX object */
	char	certtype;	/* certificate type: PEM/DER */
	char	socktype;	/* socket type: server/client */
	char	verifytype;	/* verify type: client verify/none */
	int	errnum;		/* error number */
} sslsock_ctx_t;


/**
 *	SSL socket state.
 */
enum ss_state {
	SS_STE_CLOSED = 0,	/* the socket is freed */
	SS_STE_ACCEPT,		/* the server socket can accept */
	SS_STE_CONNECTED,	/* the socket connect to peer */
	SS_STE_TCPCONNECT,	/* in tcp connect state */
	SS_STE_SSLCONNECT,	/* in ssl handshake state */
};

/**
 *	SSL socket struct.
 */
typedef struct sslsock {
	sslsock_ctx_t	*ctx;		/* sslsock_ctx object */
	int		fd;		/* socket fd */
	SSL		*ssl;		/* SSL object */
	u_int32_t	sip;		/* source IP */
	u_int32_t	dip;		/* Dest IP */
	u_int16_t	sport;		/* Source port */
	u_int16_t	dport;		/* Dest port */
	char		type;		/* socket type */
	u_int8_t	blocked;	/* 1 is block mode, 0 nonblock */
	char		state;		/* the sslsock state, see above */
	int		errnum;		/* error number */
} sslsock_t;


/**
 *	Create a sslsock_ctx_t object and returned it. 
 *
 *	If @socktype is SS_SERVER, the returned sslsock_ctx_t object
 *	is for SSL server, else it's for SSL client.
 *
 *	If @verifytype is SS_NONE, means the SSL client didn't need 
 *	provide certificate, else the SSL client need certificate.
 *
 *	Return sslsock_ctx_t object if success, NULL on error.
 */
extern sslsock_ctx_t * 
ss_ctx_alloc(int socktype, int verifytype);


/**
 *	Load certificate @certfile and private key file @keyfile into
 *	sslsock_ctx_t object @sc, the private key file password 
 *	is @passwd if needed.
 *
 *	If @certtype is SSLSOCK_X509, using X509 certificate, else using 
 *	PKCS#12 certificate.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
ss_ctx_load_cert(sslsock_ctx_t *sc, const char *cert, 
		 const char *key, const char *passwd);


/**
 *	Load CA certificate for Client Authority. the CA's certificate
 *	is in @certfile, the CA filetype is @filetype.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
ss_ctx_load_ca(sslsock_ctx_t *sc, const char *cert);



/**
 *	Load CRL file @crlfile into sslsock_ctx object for client
 *      certificate authority.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
ss_ctx_load_crl(sslsock_ctx_t *sc, const char *crlfile);


/**
 *	Set algorithm for sslsock_ctx_t object, it'll used in 
 *	sslsock_t object which alloc from it.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
ss_ctx_set_cipher(sslsock_ctx_t *sc, const char *cipher);


/**
 *	Create a SSL server socket on sslsock_ctx_t @sc, it's
 *	listen on @ip:@port. If the @noblock is nonzero, using 
 *	nonblock mode, else using block mode.
 *
 *	Return sslsock_t object if success, NULL on error.
 */
extern sslsock_t *
ss_server(sslsock_ctx_t *sc, u_int32_t ip, u_int16_t port);


/**
 *	Create a SSL client socket on sslsock_ctx_t @sc, it 
 *	connect to SSL server @ip:@port. If @timeout < 0 using block
 *	mode, or else using non-block mode.
 *
 *	Return sslsock_t object if success, NULL on error.
 */
extern sslsock_t * 
ss_client(sslsock_ctx_t *sc, u_int32_t ip, u_int16_t port);


/**
 *	Set the block/nonblock mode according @nbio, if zero, using block,
 *	else use non-block mode.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
ss_set_nbio(sslsock_t *ss, int nbio);


/**
 *	Accept SSL client connection on @ss in @timeout second, 
 *	and return the ssl socket.
 *	If the @timeout < 0, it'll wait until a connect is coming.
 *	if @algo is set, we'll set the @algo in accept ssl connection.
 *	If @nonblock is not zero, the returned object using nonblock
 *	mode, or else using block mode.
 *
 *	Return sslsock_t object if success, NULL on error.
 */
extern sslsock_t *
ss_accept(sslsock_t *ss, int algo);


/**
 *	Do ssl handshake in @timeout second.
 * 
 *	Return 0 if connect success, -1 on error, 
 *	1 need call next time.
 */
extern int 
ss_handshake(sslsock_t *ss);


/**
 *	Recv data from @ss and stored in @buf. 
 *	If @blocked is non-zero, using block-recv, else using 
 *	non-block recv.
 *
 *	Return bytes number recved if success, -1 on error,
 *	-2 means peer closed.
 */
extern int 
ss_recv(sslsock_t *ss, char *buf, size_t size, int *closed);


/**
 *	Send data to @ss which stored in @buf, the send bytes 
 *	length is @blen. If @blocked is non-zero, using block send,
 *	else using non-block send.
 *
 *	Return 0 if send success, -1 on error.
 */
extern int 
ss_send(sslsock_t *ss, const char *buf, size_t size);


/**
 *	Check the sslsock have data need to read.
 *
 *	Return 1 if have data, 0 not.
 */
extern int 
ss_pending(sslsock_t *ss);

/**
 *	Print the cipher of sslsock_t object.
 *
 *	No return.
 */
extern void 
ss_print_ciphers(const sslsock_t *ss);


/**
 *	Free all resource in sslsock_ctx_t @sc.
 *
 *	No return.
 */
extern void 
ss_ctx_free(sslsock_ctx_t *sc);


/**
 *	Free all resource in sslsock_t @ss
 *
 *	No return.
 */
extern void 
ss_free(sslsock_t *ss);


#endif /* end of FZ_SSLSOCK_H  */

