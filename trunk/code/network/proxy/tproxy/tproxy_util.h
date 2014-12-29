/**
 *	@file
 *
 *	@brief
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_TPROXY_H
#define FZ_TPROXY_H

#include <sys/types.h>
#include <sys/ioctl.h>

#include "ip_addr.h"
#include "proxy_common.h"

#define	TPROXY_DEVNAME			"/dev/tproxy_file"
#define	TPROXY_MAJOR			129
#define	TPROXY_IOC_ADD_POLICY		_IOR(TPROXY_MAJOR, 2, char *)
#define	TPROXY_IOC_GET_POLICY		_IOR(TPROXY_MAJOR, 4, char *)
#define	TPROXY_IOC_DEL_POLICY		_IOR(TPROXY_MAJOR, 3, char *)
#define	TPROXY_IOC_FLUSH_POLICIES	_IOR(TPROXY_MAJOR, 1, char *)
#define	TPROXY_IOC_SET_POLICIES		_IOR(TPROXY_MAJOR, 0, char *)

/**
 *	Transparent kernel policy structure.
 */
typedef struct tp_policy {
	char		name[MAX_NAME];
	ip_port_t	vsaddr;
	int		npsaddr;
	ip_port_t	psaddrs[0];
} tp_policy_t;

typedef struct tp_policies {
	int		size;
	int		npolicy;
	tp_policy_t	policies[0];
} tp_policies_t;

extern int 
tp_add_policy(const tp_policy_t *kpl);

extern int 
tp_get_policy(const ip_port_t *vsaddr, tp_policy_t *kpl);

extern int 
tp_del_policy(const ip_port_t *vsaddr);

extern int 
tp_flush_policies(void);

#endif /* end of FZ_TRAPT_H */


