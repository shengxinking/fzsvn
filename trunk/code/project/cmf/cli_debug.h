/*
 *
 *
 */

#ifndef CLI_DEBUG_H
#define CLI_DEBUG_H

#include <stdio.h>

#define	CLI_DEBUG	1

#ifdef	CLI_DEBUG
#define CLI_ERR(fmt, args...)		printf("[%s:%d]-[CLI-ERR]: "fmt, ##args)
#else
#define	CLI_ERR(fmt, args...)
#endif

extern void 
cli_print(const char *fmt, args...);


extern void 
cli_debug(int level, const char *fmt, args...);



#endif /* end of CLI_DEBUG_H */

