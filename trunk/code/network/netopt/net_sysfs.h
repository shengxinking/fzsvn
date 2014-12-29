/**
 *	@file	net_sysfs.h
 *
 *	@brief	Get network device information from /sys filesystem.
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_NET_SYSFS_H
#define FZ_NET_SYSFS_H

/**
 *	Get error message when nets_xxxx API failure.
 *
 *	Return string to error message.
 */
extern const char *
nets_get_error(void);

/**
 *	Check interface %ifname is exist or not.
 *
 *	Return 1 if exist, 0 if not exist.
 */
extern int 
nets_is_exist(const char *ifname);

/**
 *	Get interface @ifname flags(IFF_XXXX in <net/if.h>)
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
nets_get_flags(const char *ifname);

/**
 *	Get interface @ifname vendor ID and return it.
 *
 *	Return >=0 if success, -1 on error.
 */
extern int 
nets_get_vendor_id(const char *ifname);

/**
 *	Get interface @ifname device ID and return it.
 *
 *	Return >=0 if success, -1 on error.
 */
extern int 
nets_get_device_id(const char *ifname);

/**
 *	Get the interface @ifname index.
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
nets_get_index(const char *ifname);

/**
 *	Get the MTU size of give interface @ifname
 *
 *	Return > 0 if success, -1 on error.
 */
extern int 
nets_get_mtu(const char *ifname);

/**
 *	Get the link speed of give interface @ifname.
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
nets_get_speed(const char *speed);

/**
 *	Get the interface @ifname carrier status.
 *
 *	Return 1 if plugged, 0 if not plugged, -1 on error.
 */
extern int 
nets_get_carrier(const char *ifname);

/**
 *	Get the MAC address of give interface @ifname.
 *
 *	Return pointer to addr if success, -1 on error.
 */
extern char * 
nets_get_hwaddr(const char *ifname, char *addr, size_t len);

/**
 *	Get the IRQ number count of give interface @ifname.
 *
 *	Return > 0 if success, -1 on error.
 */
extern int 
nets_get_irq_count(const char *ifname);

/**
 *	Get the all IRQ number of give interface @ifname.
 *	The IRQ number stored in @irqs.
 *
 *	Return >0 means interrupts number, -1 on error.
 */
extern int 
nets_get_irqs(const char *ifname, int *irqs, int nirq);

/**
 *	Get RPS queue count of give interface @ifname.
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
nets_get_rps_count(const char *ifname);

/**
 *	Get RPS value of give interface @ifname, only support 64 CPUs.
 *
 * 	Return >=0 if success, -1 on error.
 */
extern int 
nets_get_rps_cpu(const char *ifname, u_int64_t *masks, int nmask);

/**
 *	Set interface @ifname RPS cpu flags. only support 
 *	64 CPUs. The cpu flags on @masks.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
nets_set_rps_cpu(const char *ifname, u_int64_t *masks, int nmask);

/**
 *	Get interface @ifname's RPS flow count value.
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
nets_get_rps_flow(const char *ifname, int *flows, int nflow);

/**
 *	Set interface @ifname's RPS flow count as @flow_cnt
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
nets_set_rps_flow(const char *ifname, int *flows, int nflow);


#endif /* end of FZ_NETSYS_H  */

