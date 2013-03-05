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
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>

#include "netdev.h"

static int
_eth_get_ethtool(const char * ifname, struct ethtool_cmd *ecmd)
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
        ifr.ifr_data = (char *)ecmd;

        ret = ioctl(fd, SIOCETHTOOL, &ifr);
        if (ret < 0) {
                printf("ioctl SIOCETHTOOL failed: %s\n", strerror(errno));
        }                                                         
                                                                  
        close(fd);

        return ret;
}

int 
_eth_set_ethtool(const char *ifname, struct ethtool_cmd *ecmd)
{
	return 0;
}

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
	ifr.ifr_data = dinfo;
	ret = ioctl(fd, SIOCETHTOOL, &ifr);
	if (ret < 0) 
		printf("ioctl SIOCETHTOOL failed: %s\n", strerror(errno));

	close(fd);
	return ret;
}

int 
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
	ifr.ifr_data = eeprom;
	ret = ioctl(fd, SIOCETHTOOL, &ifr);
	if (ret < 0) 
		printf("ioctl SIOCETHTOOL failed: %s\n", strerror(errno));

	close(fd);
	return ret;
}

int 
eth_get_speed(const char *ifname)
{
	int speed = 0;
        struct ethtool_cmd ecmd;

        if (_eth_get_ethtool(ifname, &ecmd))
                return -1;

	speed = ethtool_cmd_speed(&ecmd);

        return speed;
}

int 
eth_get_port(const char *ifname)
{
        struct ethtool_cmd ecmd;

        if (_eth_get_ethtool(ifname, &ecmd))
                return -1;

        return ecmd.port;
}

int 
eth_is_duplex(const char *ifname)
{       
        struct ethtool_cmd ecmd;                                  
                                                                  
        if (_eth_get_ethtool(ifname, &ecmd))
                return -1;

        return ecmd.duplex;
}

int 
eth_is_autoneg(const char *ifname)
{
        struct ethtool_cmd ecmd;

        if (_eth_get_ethtool(ifname, &ecmd))
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
eth_read_eeprom(const char *ifname, char *buf, size_t off, size_t len)
{
	int ret = 0;
	size_t dlen;
	size_t elen;
	struct ethtool_drvinfo dinfo;
	struct ethtool_eeprom *eeprom;

	if (_eth_get_drvinfo(ifname, &dinfo))
		return -1;

	dlen = dinfo.eedump_len;
	printf("off %lu, len %lu, off+len %lu, dlen %lu\n", off, len, off+len,dlen);
	if ((off + len) > dlen) {
		printf("off+len oversize eeprom length %lu\n", dlen);
		return -1;
	}

	elen = len + sizeof(struct ethtool_eeprom);
	eeprom = malloc(elen);
	if (!eeprom) {
		printf("malloc memory for eeprom failed\n");
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

