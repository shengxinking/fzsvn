/*
 *
 *
 *
 */

#ifndef CLI_COMMON_H
#define CLI_COMMON_H

#ifndef OFFSET
#define OFFSET(type, member)	((unsigned long)&((type*)0)->member)
#endif

#ifndef CONTAIN_OF
#define CONTAIN_OF(type, member, ptr) \
	((type *)(char*)ptr - OFFSET(type, member))
#endif

#endif /* end of CLI_COMMON_H */

