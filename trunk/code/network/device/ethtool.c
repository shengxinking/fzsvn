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
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <linux/ethtool.h>

#include "ethtool.h"

static int
_if_get_ethtool(const char * ifname, struct ethtool_cmd *ecmd)
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

static int 
_if_set_ethtool(const char *ifname, struct ethtool_cmd *ecmd)
{
	return 0;
}

int 
if_get_speed(const char *ifname)
{
	int speed = 0;
        struct ethtool_cmd ecmd;

        if (_if_get_ethtool(ifname, &ecmd))
                return -1;

	speed = ethtol_cmd_speed(ecmd);

        return speed;
}

int 
if_get_port(const char *ifname)
{
        struct ethtool_cmd ecmd;

        if (_if_get_ethtool(ifname, &ecmd))
                return -1;

        return ecmd.port;
}

int 
if_is_duplex(const char *ifname)
{       
        struct ethtool_cmd ecmd;                                  
                                                                  
        if (_if_get_ethtool(ifname, &ecmd))
                return -1;

        return ecmd.duplex;
}

int 
if_is_autoneg(const char *ifname)
{
        struct ethtool_cmd ecmd;

        if (_if_get_ethtool(ifname, &ecmd))
                return -1;

        return ecmd.autoneg;
}


