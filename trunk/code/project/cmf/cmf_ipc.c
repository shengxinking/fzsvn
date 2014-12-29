/**
 *	@file	cli_ipc.c
 *
 *	@brief	The CLI IPC implement.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date
 */


/**
 *	Create a listen socket on @ipc.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
cli_ipc_listen(cli_ipc_t *ipc)
{

	return 0;
}

/**
 *	Connect to CLI server using @ipc.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
cli_ipc_connect(cli_ipc_t *ipc)
{
	return 0;
}


/**
 *	Send message @msg to peer using @ipc.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
cli_ipc_send(cli_ipc_t *ipc, cli_msg_t *msg)
{
	return 0;
}


/**
 *	Recv message from @ipc.
 *
 *	Return the message if success, NULL on error.
 */
extern cli_msg_t * 
cli_ipc_recv(cli_ipc_t *ipc)
{
	return 0;
}


/**
 *	Close IPC and free all resources.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
cli_ipc_close(cli_ipc_t *ipc)
{
	return 0;
}



