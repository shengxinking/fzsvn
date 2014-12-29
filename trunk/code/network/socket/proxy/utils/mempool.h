/**
 *	@file	mem_pool.h
 *
 *	@brief	The memory pool data structure and APIs declare,
 *		it's like Apache apr_pool.
 *	
 *	@author	Forrest.zhang
 *
 *	@date	2012-07-20
 */

#ifndef FZ_MEM_POOL_H
#define FZ_MEM_POOL_H

typedef struct mp_data_s {
	char	*start;
	char	*end;
	
} mem_data_t;


#endif /* end of FZ_MEM_POOL_H */


