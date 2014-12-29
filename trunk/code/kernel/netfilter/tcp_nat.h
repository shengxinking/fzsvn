/**
 *	@file	tcp_nat.h
 *
 *	@brief	TCP nat module, it nat incoming packet to a designed IP:port.
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_TCP_NAT_H
#define FZ_TCP_NAT_H

#define	TCP_NAT_NETLINK	25		/* the netlink number */
#define	TCP_NAT_MAX	32		/* max IP nat */

#include <linux/types.h>

enum {
	TCP_NAT_ADD_IP = 1,
	TCP_NAT_DEL_IP,
	TCP_NAT_GET_IP,
	TCP_NAT_CLEAR_IP,
	TCP_NAT_SET_HTTP,
	TCP_NAT_SET_HTTPS,
	TCP_NAT_SET_TELNET,
	TCP_NAT_SET_SSH,
	TCP_NAT_GET_PORT,
};


#ifdef	__KERNEL__

typedef struct tcp_nat_ip {
	u32		ip[TCP_NAT_MAX];/* the IPs need nat */
} tcp_nat_ip_t;

typedef struct tcp_nat_port {
	u16		http_port;
	u16		https_port;
	u16		telnet_port;
	u16		ssh_port;
} tcp_nat_port_t;

#else /* not __KERNEL__ */

typedef struct tcp_nat_ip {
	u_int32_t	ip[TCP_NAT_MAX];/* the IPs need nat */
} tcp_nat_ip_t;

typedef struct tcp_nat_port {
	u_int16_t	http_port;
	u_int16_t	https_port;
	u_int16_t	telnet_port;
	u_int16_t	ssh_port;
} tcp_nat_port_t;

extern int 
tcp_nat_add_ip(u_int32_t ip);

extern int 
tcp_nat_del_ip(u_int32_t ip);

extern int 
tcp_nat_get_ip(u_int32_t *ips, size_t len);

extern int 
tcp_nat_clear_ip(void);

extern int 
tcp_nat_set_port(u_int16_t port, int type);

extern int 
tcp_nat_get_port(tcp_nat_port_t *tp);

#endif	/* end of __KERNEL__ */

#endif /* end of FZ_TCP_NAT_H  */

