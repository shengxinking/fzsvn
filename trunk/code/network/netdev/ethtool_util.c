/**
 *	@file	
 *
 *	@brief
 *	
 *	@author	Forrest.zhang	
 *
 *	@date
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>

#include "ethtool_util.h"

#define	_ETH_ERR(fmt, args...)	\
	fprintf(stderr, "%s:%d "fmt, __FILE__, __LINE__, ##args)

#define _ETH_ERRLEN		2047
static char			_eth_errbuf[_ETH_ERRLEN + 1];

/**
 *	Ethtool command ETHTOOL_GSET to get settings.
 *	
 *	Return 0 if success, -1 on error.
 */
static int
_eth_gset_cmd(const char * ifname, struct ethtool_cmd *ecmd)
{       
        int fd;
        int ret = -1;
        struct ifreq ifr;

        if (!ifname || !ecmd)
                return -1;

        fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (fd < 0)
                return -1;

        memset(ecmd, 0, sizeof(*ecmd));
        ecmd->cmd = ETHTOOL_GSET;

        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
        ifr.ifr_data = (void *)ecmd;

        ret = ioctl(fd, SIOCETHTOOL, &ifr);
        close(fd);

        if (ret < 0) 
                _ETH_ERR("ioctl SIOCETHTOOL %s failed: %s\n", 
			 ifname, strerror(errno));
                                                                  
        return ret;
}

/**
 *	Ethtool command ETHTOOL_SSET to set settings.
 *	
 *	Return 0 if success, -1 on error.
 */
int 
_eth_sset_cmd(const char *ifname, struct ethtool_cmd *ecmd)
{
	return 0;
}

/**
 *	Ethtool command ETHTOOL_GLINK to get link status.
 *	
 *	Return 0 if success, -1 on error.
 */
int 
_eth_glink_cmd(const char *ifname)
{
	int fd;
	struct ifreq ifr;
	struct ethtool_value edata;

	int ret;

	if (!ifname)
		return -1;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
		return -1;
	
	memset(&edata, 0, sizeof(edata));
	edata.cmd = ETHTOOL_GLINK;
	memset(&ifr, 0, sizeof (ifr));
	strcpy(ifr.ifr_name, ifname);
	ifr.ifr_data = (caddr_t)&edata;

	ret = ioctl(fd, SIOCETHTOOL, &ifr);
	close(fd);

	if (ret < 0) {
		_ETH_ERR("ioctl SIOCETHTOOL %s failed: %s\n", 
			 ifname, strerror(errno));
		return -1;
	}

	return edata.data;
}

/**
 *	Ethtool command ETHTOOL_GDRVINFO to get driver info.
 *	
 *	Return 0 if success, -1 on error.
 */
int 
_eth_get_drvinfo(const char *ifname, struct ethtool_drvinfo *dinfo)
{
	int fd;
	struct ifreq ifr;
	int ret;

	if (!ifname || !dinfo)
		return -1;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
		return -1;
	
	memset(dinfo, 0, sizeof(struct ethtool_drvinfo));
	dinfo->cmd = ETHTOOL_GDRVINFO;
	memset(&ifr, 0, sizeof (ifr));
	strcpy(ifr.ifr_name, ifname);
	ifr.ifr_data = (void *)dinfo;
	ret = ioctl(fd, SIOCETHTOOL, &ifr);
	if (ret < 0) {
		_ETH_ERR("ioctl SIOCETHTOOL %s failed: %s\n", 
			 ifname, strerror(errno));
	}

	close(fd);
	return ret;
}

/**
 *	Get eeprom content of interface @ifname.
 *
 *	Return >= 0 if success, -1 on error.
 */
static int 
_eth_read_eeprom(const char *ifname, struct ethtool_eeprom *eeprom)
{
	int fd;
	struct ifreq ifr;
	int ret;

	if (!ifname || !eeprom)
		return -1;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
		return -1;
	
	memset(&ifr, 0, sizeof (ifr));
	strcpy(ifr.ifr_name, ifname);
	ifr.ifr_data = (void *)eeprom;
	ret = ioctl(fd, SIOCETHTOOL, &ifr);
	if (ret < 0) 
		printf("ioctl SIOCETHTOOL failed: %s\n", strerror(errno));

	close(fd);
	return ret;
}

const char *
eth_get_error(void)
{
	return _eth_errbuf;
}

int 
eth_get_speed(const char *ifname)
{
	int speed = 0;
        struct ethtool_cmd ecmd;

        if (_eth_gset_cmd(ifname, &ecmd))
                return -1;

	speed = ethtool_cmd_speed(&ecmd);

        return speed;
}

int 
eth_get_port(const char *ifname)
{
        struct ethtool_cmd ecmd;

        if (_eth_gset_cmd(ifname, &ecmd))
                return -1;

        return ecmd.port;
}

int 
eth_get_duplex(const char *ifname)
{       
        struct ethtool_cmd ecmd;                                  
                                                                  
        if (_eth_gset_cmd(ifname, &ecmd))
                return -1;

        return ecmd.duplex;
}

int eth_get_carrier(const char *ifname)
{
	if (!ifname)
		return -1;

	return _eth_glink_cmd(ifname);
}

int 
eth_get_autoneg(const char *ifname)
{
        struct ethtool_cmd ecmd;

        if (_eth_gset_cmd(ifname, &ecmd))
                return -1;

        return ecmd.autoneg;
}

int 
eth_get_eeprom_size(const char *ifname)
{
	int elen;
	struct ethtool_drvinfo dinfo;

	if (_eth_get_drvinfo(ifname, &dinfo))
		return -1;

	elen = dinfo.eedump_len;
	return elen;
}

int 
eth_get_eeprom(const char *ifname, char *buf, size_t off, size_t len)
{
	int ret = 0;
	size_t dlen;
	size_t elen;
	struct ethtool_drvinfo dinfo;
	struct ethtool_eeprom *eeprom;

	if (_eth_get_drvinfo(ifname, &dinfo))
		return -1;

	dlen = dinfo.eedump_len;
	if ((off + len) > dlen) {
		_ETH_ERR("off+len oversize eeprom length %lu\n", dlen);
		return -1;
	}

	elen = len + sizeof(struct ethtool_eeprom);
	eeprom = malloc(elen);
	if (!eeprom) {
		_ETH_ERR("malloc memory for eeprom failed\n");
		return -1;
	}
	memset(eeprom, 0, elen);
	eeprom->cmd = ETHTOOL_GEEPROM;
	eeprom->len = len;
	eeprom->offset = off;
	ret = _eth_read_eeprom(ifname, eeprom);
	if (ret < 0) {
		free(eeprom);
		return -1;
	}
	
	memcpy(buf, eeprom->data, len);

	return 0;
}


