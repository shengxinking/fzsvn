/**
 *	@file	cmf_common.h
 *
 *	@brief	CMF common macro, data struct, typedefines.
 */

#ifndef	CMF_COMMON_H
#define	CMF_COMMON_H

#define	CMF_FROM_CLI	0
#define	CMF_FROM_GUI	1
#define	CMF_FROM_CMDB	2
#define	CMF_FROM_USER	3

#define	CMF_ADD_EVENT	(1 << 0)
#define	CMF_DEL_EVENT	(1 << 1)
#define	CMF_EDIT_EVENT	(1 << 2)
#define	CMF_MOVE_EVENT	(1 << 3)
#define	CMF_FLUSH_EVENT	(1 << 4)

#define	CMF_DETAIL_EVENT (1 << 0)

typedef	unsigned int	cmf_ptr_t;	/* memory ptr */
typedef	unsigned int	cmf_oid_t;	/* object id */


#endif	/* end of CMF_COMMON_H */

