/**
 *	@file	cmf_ipc.h
 *
 *	@brief	cmf communicate APIs. it's for query/event.
 *	
 *	@author	Forrest.zhang
 *
 *	@date	2013-06-16
 */


#ifndef FZ_CMF_IPC_H
#define FZ_CMF_IPC_H

#define CLI_MSGLEN		4096
#define CLI_IPCPATH		"/tmp/cmf/"

/* cmf ipc */
typedef struct cmf_ipc {
	int		fd;
	int		lfd;
} cmf_ipc_t;

/* cmf ipc message type. */
enum {
	CMF_MSG_CMD = 1,	/* CLI command message */
	CMF_MSG_TREE,		/* CLI get tree message */
	CMF_MSG_QUERY_REQ,	/* query request */
	CMF_MSG_QUERY_RES,	/* query response */
	CMF_MSG_EVENT,		/* event message */
	CMF_MSG_OK,		/* OK */
	CMF_MSG_ERROR,		/* ERROR */
	CMF_MSG_MAX,
};


/* cmf ipc message */
typedef struct cmf_msg {
	uint16		type;	/* message type, see above */
	uint16		len;	/* the message total length */
	char		buf[0];	/* the message body */
} cli_msg_t;


/**
 *	Create a listen socket on @ipc.
 *
 *	Return 0 if success, -1 on error.
 */
extern cmf_ipc_t * 
cmf_ipc_server(void);

/**
 *	Connect to CLI server using @ipc.
 *
 *	Return 0 if success, -1 on error.
 */
extern cmf_ipc_t * 
cmf_ipc_connect(void);

/**
 *	Send message @msg to peer using @ipc.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
cli_ipc_sendmsg(cli_ipc_t *ipc, cli_msg_t *msg);


/**
 *	Recv message from @ipc.
 *
 *	Return the message if success, NULL on error.
 */
extern cli_msg_t * 
cli_ipc_recvmsg(cli_ipc_t *ipc);

/**
 *	Close IPC and free all resources.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
cli_ipc_close(cli_ipc_t *ipc);


#endif /* end of FZ_CLI_IPC_H  */

