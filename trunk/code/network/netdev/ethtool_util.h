/**
 *	@file	ethtool_api.h
 *
 *	@brief	The ethtool APIs.
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_ETHTOOL_API_H
#define FZ_ETHTOOL_API_H

/**
 *	Get link speed of ethernet interface @ifname.
 *
 *	Return:
 *	10	(10Mbps)
 *	100	(100Mbps)
 *	1000	(1000Mbps)
 *	10000	(10000Mbps)
 *	0	(Not linked)
 *	-1 	Error.
 */
extern int 
eth_get_speed(const char *ifname);


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
eth_get_duplex(const char *ifname);

/**
 *	Check the interface @ifname is auto-negotiation or not.	
 * 	Return 1 if is autoneg, 0 not or failed.
 */
extern int 
eth_get_autoneg(const char *ifname);

/**
 *	Chech the interface @ifname carrier status.
 *
 *	Return 1 if is linkup, 0 not plugged, -1 on failed.
 */
extern int 
eth_get_carrier(const char *ifname);

/**
 *	Get eeprom size of ethernet @ifname.	
 * 
 */
extern int 
eth_get_eeprom_size(const char *ifname);

/**
 *	Read bytes from ethernet eeprom, the read offset is @off, length 
 *	is @len, store read bytes into buffer @buf, @buf length no less 
 *	than @len.
 *	
 *	Return 0 if read success, -1 on error. 
 */
extern int 
eth_get_eeprom(const char *ifname, char *buf, size_t off, size_t len);

#endif /* end of FZ_ETHTOOL_API_H  */

