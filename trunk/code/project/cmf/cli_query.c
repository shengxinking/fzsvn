/**
 *	@file	query.c
 *
 *	@brief	The CLI query implement.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2009-01-13
 */

#include "cli_query.h"


/**
 *	Create a query for CLI object @path.
 *
 *	Return the query if success, NULL on error.
 */
extern cli_query_t * 
cli_query_create(const char *path)
{
	return 0;
}

/**
 *	Update the query @query data to newly object value.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
cli_query_update(cli_query_t *query)
{
	return 0;
}


/**
 *	Commit the query @query's data to CLI.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
cli_query_commit(cli_query_t *query)
{
	return 0;
}


/**
 *	Free the query @query
 *
 *	No return.
 */
extern void 
cli_query_free(cli_query_t *query)
{
	
}


