/**
 *	@file
 *
 *	@brief
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_TRAPT_UTIL_H
#define FZ_TRAPT_UTIL_H

#include <sys/types.h>
#include <sys/ioctl.h>

#define	TRAPT_DEVNAME		"/dev/trapt_proxy"
#define	TRAPT_NAMELEN		97

#define	TRAPT_IOC_MAJOR		124
#define	TRAPT_IOC_ADD_POLICY	_IOR(TRAPT_IOC_MAJOR, 1, char *)
#define	TRAPT_IOC_DEL_POLICY	_IOR(TRAPT_IOC_MAJOR, 2, char *)
#define	TRAPT_IOC_FLUSH_POLICY	_IOR(TRAPT_IOC_MAJOR, 3, char *)
#define	TRAPT_SO_GET_DST	213
#define	TRAPT_SO_SET_DST	214

typedef struct tat_addr {
	u_int32_t	addr;
	u_int16_t	port;
	u_int16_t	is_ssl;
} tat_addr_t;

typedef struct tat_policy {
	char		name[TRAPT_NAMELEN];
	u_int32_t	vsaddr;
	u_int16_t	vsport;

	unsigned int	http_num;
	unsigned int	http_bytes;
	unsigned int	ssl_num;

	unsigned int	syn_cookie;
	unsigned int	syn_halfopen;

	char		brname[65];

	u_int16_t	npsaddr;
	tat_addr_t	psaddrs[0];
} tat_policy_t;

int 
tat_add_policy(const tat_policy_t *pl);

int 
tat_get_policy(const tat_addr_t *vsaddr, tat_policy_t *pl);

int 
tat_del_policy(const tat_addr_t *vsaddr);

int 
tat_flush_policies(void);

int 
tat_get_dstaddr(int fd, tat_addr_t *addr);

int 
tat_set_dstaddr(int fd, const tat_addr_t *addr);

#endif /* end of FZ_TRAPT_UTIL_H */


