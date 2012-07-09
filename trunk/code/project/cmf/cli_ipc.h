/**
 *	@file
 *
 *	@brief
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#define CLI_IPC_MSG_LEN		4096

#ifndef FZ_CLI_IPC_H
#define FZ_CLI_IPC_H

#define CLI_IPC_PATH	"/tmp/cli_ipc"

/**
 *	CLI IPC struct.
 */
typedef struct cli_ipc {
	int		fd;
	int		lfd;
} cli_ipc_t;


/**
 *	CLI IPC message type.
 */
typedef enum cli_msg_type {
	CLI_MSG_CMD = 1,	/* CLI command message */
	CLI_MSG_TREE,		/* CLI get tree message */
	CLI_MSG_OK,		/* CLI command/tree OK */
	CLI_MSG_ERROR,		/* CLI command/tree error message */
	CLI_MSG_MAX,
} cli_msg_type_t;


/**
 *	CLI IPC message.
 */
typedef struct cli_msg {
	uint16		type;	/* message type, see above */
	uint16		len;	/* the message total length */
	char		buf[0];	/* the message body */
} cli_msg_t;


/**
 *	Create a listen socket on @ipc.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
cli_ipc_listen(cli_ipc_t *ipc);

/**
 *	Connect to CLI server using @ipc.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
cli_ipc_connect(cli_ipc_t *ipc);

/**
 *	Send message @msg to peer using @ipc.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
cli_ipc_send(cli_ipc_t *ipc, cli_msg_t *msg);


/**
 *	Recv message from @ipc.
 *
 *	Return the message if success, NULL on error.
 */
extern cli_msg_t * 
cli_ipc_recv(cli_ipc_t *ipc);


/**
 *	Close IPC and free all resources.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
cli_ipc_close(cli_ipc_t *ipc);


#endif /* end of FZ_CLI_IPC_H  */

