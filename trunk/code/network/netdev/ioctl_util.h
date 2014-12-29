/**
 *	@file	ioctl_util.h
 *	@brief  Network device ioctl functions.
 *
 *	@author	Forrest.zhang
 */

#ifndef FZ_IOCTL_UTIL_H
#define FZ_IOCTL_UTIL_H

#include <netinet/in.h>
#include <net/if.h>

/**
 *	Get error message of ioc_xxxx() functions.
 *
 *	Return error message string.
 */
extern const char *
ioc_get_error(void);

/**
 *	Get interface @ifname index.
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
ioc_get_index(const char *ifname);

/**
 *	Get interface @ifname's carrier value.
 *
 *	Return value:
 *	1: carrier is plugged.
 *	0: carrier is not plugged.
 *	-1: error occured.
 */
extern int 
ioc_get_carrier(const char *ifname);

/**
 *	Get interface index @index's name and stored 
 *	in @ifname, the @ifname length is @size.
 *
 *	Return pointer to @ifname is success, NULL on 
 *	error.
 */
extern char *
ioc_get_name(int index, char *ifname, size_t size);

/**
 *	Set interface @ifname to new name @newname.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
ioc_set_name(const char *ifname, const char *newname);

/**
 *	Get interface @ifname's MTU value.
 *
 *	Return > 0 if success, -1 on error.
 */
extern int 
ioc_get_mtu(const char *ifname);

/**
 *	Set interface @ifname's MTU value as @mtu.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
ioc_set_mtu(const char *ifname, int mtu);

/**
 *	Get interface @ifname's hardware address(MAC) and
 *	stored in @addr.
 *
 *	Return pointer to @addr if success, -1 on error.
 */
extern struct sockaddr *
ioc_get_hwaddr(const char *ifname, struct sockaddr *addr);

/**
 *	Set interface @ifname's hardware address(MAC) as
 *	@addr.
 *
 *	return 0 if success, -1 on error.
 */
extern int 
ioc_set_hwaddr(const char *ifname, const struct sockaddr *addr);

/**
 *	Get interface @ifname's IPv4 address stored in @addr
 *
 *	Return pointer to @addr if success, NULL on error.
 */
extern struct sockaddr *
ioc_get_addr(const char *ifname, struct sockaddr *addr);

/**
 *	Set interface @ifname's IPv4 address as @addr.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
ioc_set_addr(const char *ifname, const struct sockaddr *addr);

/**
 *	Get interface @ifname's IPv4 netmask stored in @addr
 *
 *	Return pointer to @addr if success, NULL on error.
 */
extern struct sockaddr *
ioc_get_netmask(const char *ifname, struct sockaddr *addr);

/**
 *	Set interface @ifname's IPv4 netmask as @addr.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
ioc_set_netmask(const char *ifname, const struct sockaddr *addr);

/**
 *	Get interface @ifname's IPv4 broadcast stored in @addr
 *
 *	Return pointer to @addr if success, NULL on error.
 */
extern struct sockaddr *
ioc_get_brdaddr(const char *ifname, struct sockaddr *addr);

/**
 *	Set interface @ifname's IPv4 broadcase as @addr.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
ioc_set_brdaddr(const char *ifname, const struct sockaddr *addr);

/**
 *	Get interface @ifname's IPv4 destination address 
 *	stored in @addr, only PPP interface support it.
 *
 *	Return pointer to @addr if success, NULL on error.
 */
extern struct sockaddr *
ioc_get_dstaddr(const char *ifname, struct sockaddr *addr);

/**
 *	Set interface @ifname's IPv4 destination address
 *	 as @addr. only PPP interface support it.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
ioc_set_dstaddr(const char *ifname, const struct sockaddr *addr);

/**
 *	Convert string "IFF_XX" to IFF_XXX flags. 
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
ioc_str2flags(const char *name);

/**
 *	Convert flags IFF_XXX to string and return it.
 *
 *	Return pointer to string if success, NULL on error.
 */
extern const char *
ioc_flags2str(int flags);

/**
 *	Get interface @ifname's all IPv4 addresses and 
 *	stored in array @addrs, the @addrs count is @naddr.
 *
 *	Return number of address if success, -1 on error.
 */
extern int 
ioc_get_alladdr(const char *ifname, struct sockaddr *addrs, size_t naddr);

/**
 *	get interface @ifname flags, the flags contains a bitmask 
 *	of following values:
 *
 *	IFF_UP | IFF_BROADCAST | IFF_DEBUG | IFF_LOOPBACK | 
 *	IFF_POINTOPOINT | IFF_RUNNING | IFF_NOARP | IFF_PROMISC | 
 *	IFF_NOTRAILERS | IFF_ALLMULTI | IFF_MASTER | IFF_MULTICAST | 
 *	IFF_PORTSEL | IFF_AUTOMEDIA | IFF_DYNAMIC
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
ioc_get_flags(const char *ifname);

/**
 *	set interface @ifname flags, the flags type see ioc_get_flags()
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
ioc_set_flags(const char *ifname, int flags);

#endif /* end of FZ_IOCTL_UTIL_H */

