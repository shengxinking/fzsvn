/**
 *	@file	tproxy.h
 *
 *	@brief	Tranparent proxy module.
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_TPROXY_H
#define FZ_TPROXY_H

#define	TP_MAX_PSERVER	128
#define	TP_MAX_NAME	36

#ifndef	__KERNEL__

typedef struct tp_pserver {
	u_int32_t	ip;
	u_int16_t	port;
	u_int16_t	nport;
} tp_pserver_t;

typedef struct tp_policy {
	char		name[TP_MAX_NAME];
	char		ifname[IFNAMSIZ];
	u_int32_t	vip;
	tp_pserver_t	pservers[TP_MAX_PSERVER];
	int		npserver;
} tp_policy_t

extern int 
tp_add_policy(tp_policy_t *tp);

extern int 
tp_del_policy(tp_policy_t *tp);

#else

typedef struct tp_pserver {
	u32		ip;
	u16		port;
	u16		nat_port;
	u8		mac[6];
} tp_pserver_t;

typedef struct tp_policy {
	char		name[TP_MAX_NAME];
	char		ifname[IFNAMSIZ];
	u32		vip;
	tp_pserver_t	pservers[TP_MAX_PSERVER];
	int		npserver;
} tp_policy_t;

#endif

#endif /* end of FZ_TPROXY_H */

