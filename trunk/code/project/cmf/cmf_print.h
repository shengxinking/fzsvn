/**
 *	@file	cmf_debug.h
 *
 *	@brief	cmf all print functions.
 */

#ifndef CMF_PRINT_H
#define CMF_PRINT_H

#include <stdio.h>

/**
 *	cmf print function in operator.
 *
 */
extern void 
cmf_print(const char *fmt, args...);

/**
 *	cmf debug print function.
 *
 */
extern void 
cmf_debug(int level, const char *fmt, args...);

/**
 *	cmf flow print function.
 *
 */
extern void 
cmf_flow(int level, const char *fmt, args...);

/**
 *	cmf error print function.
 *
 */
extern void 
cmf_err(const char *fmt, args...);


#endif /* end of CMF_PRINT_H */

