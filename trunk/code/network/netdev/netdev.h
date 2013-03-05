/*
 *  file: libinf_ioctl.h
 *  it defined API for manipulate NIC interface using ioctl command
 *
 *  write by Forrest.zhang
 */

#ifndef __NETDEVICE_H__
#define __NETDEVICE_H__

#include <netinet/in.h>
#include <linux/if.h>


/*********************************************************
 *	ioctl APIs. 
 ********************************************************/

extern const char *
if_error(void);

extern int 
if_get_index(const char *ifname);

extern const char *
if_get_name(int index, char *ifname, size_t size);

extern int 
if_set_name(const char *name, const char *newname);

extern struct sockaddr *
if_get_hwaddr(const char *ifname, struct sockaddr *addr);

extern int 
if_set_hwaddr(const char *ifname, const struct sockaddr *addr);

extern int 
if_get_mtu(const char *ifname);

extern int 
if_set_mtu(const char *ifname, int mtu);

extern struct sockaddr *
if_get_addr(const char *ifname, struct sockaddr *addr);

extern int 
if_set_addr(const char *ifname, const struct sockaddr *addr);

extern struct sockaddr *
if_get_netmask(const char *ifname, struct sockaddr *addr);

extern int 
if_set_netmask(const char *ifname, const struct sockaddr *addr);

extern struct sockaddr *
if_get_brdaddr(const char *ifname, struct sockaddr *addr);

extern int 
if_set_brdaddr(const char *ifname, const struct sockaddr *addr);

extern struct sockaddr *
if_get_dstaddr(const char *ifname, struct sockaddr *addr);

extern int 
if_set_dstaddr(const char *ifname, const struct sockaddr *addr);

extern int 
if_get_alladdr(const char *ifname, struct ifconf *ifc);

extern int 
if_is_up(const char *ifname);

extern int 
if_up(const char *ifname);

extern int 
if_down(const char *ifname);  

/* detect network cable is plugged */
extern int 
if_is_link_up(const char *ifname);

extern int 
if_is_noarp(const char *ifname);

extern int 
if_noarp(const char *ifname);

extern int 
if_arp(const char *ifname);

extern int 
if_is_debug(const char *ifname);

extern int 
if_debug(const char *ifname);

extern int 
if_nodebug(const char *ifname);

extern int 
if_is_promisc(const char *ifname);

extern int 
if_promisc(const char *ifname);

extern int 
if_nopromisc(const char *ifname);

extern int 
if_is_dynamic(const char *ifname);

extern int 
if_dynamic(const char *ifname);

extern int 
if_nodynamic(const char *ifname);

extern int 
if_is_broadcast(const char *ifname);

extern int 
if_is_loopback(const char *ifname);

extern int 
if_is_ppp(const char *ifname);

extern int 
if_is_running(const char *ifname);

extern int 
if_is_notrailers(const char *ifname);

extern int 
if_is_allmulti(const char *ifname);

extern int 
if_is_master(const char *ifname);

extern int 
if_is_slave(const char *ifname);

extern int 
if_is_multicast(const char *ifname);

extern int 
if_is_portsel(const char *ifname);

extern int 
if_is_automedia(const char *ifname);

/*********************************************************
 *	ethtool APIs. 
 ********************************************************/
/**
 *	Get link speed of ethernet interface @ifname.
 *
 *	Return:
 *	10	(10Mbps)
 *	100	(100Mbps)
 *	1000	(1000Mbps)
 *	0	(Not linked)
 *	-1 	Error.
 */
extern int 
eth_get_speed(const char *ifname);

/**
 *	Get eeprom size of ethernet @ifname.	
 * 
 */
extern int 
eth_get_eeprom_size(const char *ifname);

/**
 *	Set link speed of ethernet interface @ifname.	
 * 
 *	Return 0 if success, -1 on error.
 */
extern int 
eth_set_speed(const char *ifname);

/**
 *	Get the port media of ethernet interface @ifname.	
 * 
 *	Return:
 *	PORT_TP		(copper port, twist-pair)
 *	PORT_FIBER	(fiber port)
 *	PORT_OTHER	(other port)
 *	-1		(error)
 */
extern int 
eth_get_port(const char *ifname);

/**
 *	Check the interface @ifname is duplex port or not.	
 * 
 *	Return 1 if duplex, 0 not or failed.
 */
extern int 
eth_is_duplex(const char *ifname);

/**
 *	Check the interface @ifname is auto-negotiation or not.	
 * 	Return 1 if is autoneg, 0 not or failed.
 */
extern int 
eth_is_autoneg(const char *ifname);

/**
 *	Read bytes from ethernet eeprom, the read offset is @off, length 
 *	is @len, store read bytes into buffer @buf, @buf length no less than
 *	@len.
 *	
 *	Return 0 if read success, -1 on error. 
 */
extern int 
eth_read_eeprom(const char *ifname, char *buf, size_t off, size_t len);

#endif /* end of __LIBINF_IOCTL_H__ */

