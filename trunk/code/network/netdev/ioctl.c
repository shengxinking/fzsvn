/*
 *  implement function to get interface index, mac, speed,
 *  ip and other attribute
 *
 *  write by Forrest.zhang
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/sockios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <errno.h>
#include <netinet/ether.h>

#include "netdev.h"


#define INF_ERR_LEN    1024

static char _inf_errbuf[INF_ERR_LEN + 1] = {0};

static int _inf_ioctl(struct ifreq *ifr, int request)
{
    int          sockfd;
    
    if (!ifr)
	return -1;
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
	memset(_inf_errbuf, 0, sizeof(_inf_errbuf));
	snprintf(_inf_errbuf, INF_ERR_LEN, 
		 "create socket error: %s", strerror(errno));
	return -1;
    }

    if (ioctl(sockfd, request, ifr) < 0) {
	memset(_inf_errbuf, 0, sizeof(_inf_errbuf));
	snprintf(_inf_errbuf, INF_ERR_LEN, 
		 "ioctl error: %s", strerror(errno));
	return -1;
    }

    close(sockfd);
    return 0;
}

/**
 *  get interface flags, flags contains a bitmask of following values:
 *
 *  IFF_UP | IFF_BROADCAST | IFF_DEBUG | IFF_LOOPBACK | IFF_POINTOPOINT
 *  IFF_RUNNING | IFF_NOARP | IFF_PROMISC | IFF_NOTRAILERS | IFF_ALLMULTI
 *  IFF_MASTER | IFF_MULTICAST | IFF_PORTSEL | IFF_AUTOMEDIA | IFF_DYNAMIC
 */
static short _inf_getflags(const char *ifname)
{
    struct ifreq        ifr;

    if (!ifname)
	return -1;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	
    if (_inf_ioctl(&ifr, SIOCGIFFLAGS))
	return -1;

    return ifr.ifr_flags;
}

/*
 *  set interface @ifname flags, see _inf_getflags
 */
static int _inf_setflags(const char *ifname, short flags)
{
    struct ifreq        ifr;

    if (!ifname)
	return -1;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    ifr.ifr_flags = flags;
	
    if (_inf_ioctl(&ifr, SIOCSIFFLAGS))
	return -1;

    return 0;
}


/*
 *  get the last error message of if_XXX APIs
 */
const char *if_error(void)
{
    return _inf_errbuf;
}

/*
 *  get index of interface @ifname
 */
int if_get_index(const char *ifname)
{
    struct ifreq        ifr;

    if (!ifname)
	return -1;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	
    if (_inf_ioctl(&ifr, SIOCGIFINDEX))
	return -1;

    return ifr.ifr_ifindex;	
}


/*
 *  get interface name using interface index @index
 */
const char *if_get_name(int index, char *ifname, size_t size)
{
    struct ifreq        ifr;

    if (!ifname || !size)
	return NULL;

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_ifindex = index;
	
    if (_inf_ioctl(&ifr, SIOCGIFNAME))
	return NULL;

    memcpy(ifname, &ifr.ifr_name, size);
    return ifname;
}

/*
 *  change interface name from @name to @newname
 */
int if_set_name(const char *name, const char *newname)
{
    struct ifreq        ifr;
    int                 up = 0;

    if (!name || !newname)
	return -1;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, name, IFNAMSIZ);
    strncpy(ifr.ifr_newname, newname, IFNAMSIZ);

    up = if_is_up(name);
    if (up)
	if_down(name);

    if (_inf_ioctl(&ifr, SIOCSIFNAME))
	return -1;

    if (up)
	if_up(name);

    return 0;
}

/*
 *  get the HWADDR of interface @ifname.
 *
 *  store the HWADDR in @hwaddr, it use common address struct sockaddr.
 *  HWADDR stored in member data of struct sockaddr.
 */
struct sockaddr *if_get_hwaddr(const char *ifname, struct sockaddr *hwaddr)
{
    struct ifreq        ifr;

    if (!ifname || !hwaddr)
	return NULL;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	
    if (_inf_ioctl(&ifr, SIOCGIFHWADDR))
	return NULL;

    memcpy(hwaddr, &ifr.ifr_hwaddr, sizeof(struct sockaddr));

    return hwaddr;
}

/*
 *  set HWADDR of interface @ifname
 *
 *  @hwaddr format is 
 */
int if_set_hwaddr(const char *ifname, const struct sockaddr *addr)
{
    struct ifreq        ifr;

    if (!ifname || !addr)
	return -1;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    memcpy(&ifr.ifr_hwaddr, addr, sizeof(struct sockaddr));

    if (_inf_ioctl(&ifr, SIOCSIFHWADDR))
	return -1;

    return 0;
}

/*
 *  get MTU of interface @ifname and return it
 */
int if_get_mtu(const char *ifname)
{
    struct ifreq        ifr;

    if (!ifname)
	return -1;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	
    if (_inf_ioctl(&ifr, SIOCGIFMTU))
	return -1;

    return ifr.ifr_mtu;
}

/*
 *  set MTU of interface @ifname and return old mtu
 */
int if_set_mtu(const char *ifname, int mtu)
{
    struct ifreq        ifr;
    int                 oldmtu;

    if (!ifname || mtu <= 0)
	return -1;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    ifr.ifr_mtu = mtu;

    oldmtu = if_get_mtu(ifname);

    if (_inf_ioctl(&ifr, SIOCSIFMTU))
	return -1;

    return oldmtu;
}

/*
 *  get INET address of interface @ifname
 */
struct sockaddr *if_get_addr(const char *ifname, struct sockaddr *addr)
{
    struct ifreq        ifr;

    if (!ifname || !addr)
	return NULL;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	
    if (_inf_ioctl(&ifr, SIOCGIFADDR))
	return NULL;

    memcpy(addr, &ifr.ifr_addr, sizeof(struct sockaddr));
    return addr;
}

/*
 *  set IP address of interface @ifname
 */
int if_set_addr(const char *ifname, const struct sockaddr *addr)
{
    struct ifreq        ifr;

    if (!ifname || !addr)
	return -1;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    memcpy(&ifr.ifr_addr, addr, sizeof(struct sockaddr));
	
    if (_inf_ioctl(&ifr, SIOCSIFADDR))
	return -1;

    return 0;
}

/*
 *  get interface @ifname netmask and return it
 */
struct sockaddr *if_get_netmask(const char *ifname, struct sockaddr *addr)
{
    struct ifreq        ifr;

    if (!ifname || !addr)
	return NULL;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	
    if (_inf_ioctl(&ifr, SIOCGIFNETMASK))
	return NULL;

    memcpy(addr, &ifr.ifr_netmask, sizeof(struct sockaddr));
    return addr;
}

/*
 *  set interface @ifname netmask
 */
int if_set_netmask(const char *ifname, const struct sockaddr *addr)
{
    struct ifreq       ifr;

    if (!ifname || !addr)
	return -1;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    memcpy(&ifr.ifr_netmask, addr, sizeof(struct sockaddr));

    if (_inf_ioctl(&ifr, SIOCSIFNETMASK))
	return -1;

    return 0;
}

/*
 *  get interface @ifname INET broadcast address and return it
 */
struct sockaddr *if_get_brdaddr(const char *ifname, struct sockaddr *addr)
{
    struct ifreq        ifr;

    if (!ifname || !addr)
	return NULL;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	
    if (_inf_ioctl(&ifr, SIOCGIFBRDADDR))
	return NULL;

    memcpy(addr, &ifr.ifr_addr, sizeof(struct sockaddr));
    return addr;
}

/*
 *  set interface @ifname INET broadcast address
 */
int if_set_brdaddr(const char *ifname, const struct sockaddr *addr)
{
    struct ifreq        ifr;

    if (!ifname || !addr)
	return -1;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    memcpy(&ifr.ifr_addr, addr, sizeof(struct sockaddr));
    
    if (_inf_ioctl(&ifr, SIOCSIFBRDADDR))
	return -1;
    
    return 0;
}


/*
 *  get PPP interface @ifname peer INET address
 */
struct sockaddr *if_get_dstaddr(const char *ifname, struct sockaddr *addr)
{
    struct ifreq        ifr;

    if (!ifname || !addr)
	return NULL;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	
    if (_inf_ioctl(&ifr, SIOCGIFDSTADDR))
	return NULL;

    memcpy(addr, &ifr.ifr_dstaddr, sizeof(struct sockaddr));
    return addr;
}

/*
 *  set PPP interface @ifname peer INET address
 */
int if_set_dstaddr(const char *ifname, const struct sockaddr *addr)
{
    struct ifreq        ifr;

    if (!ifname || !addr)
	return -1;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    memcpy(&ifr.ifr_dstaddr, addr, sizeof(struct sockaddr));
	
    if (_inf_ioctl(&ifr, SIOCSIFDSTADDR))
	return -1;

    return 0;
}

/*
 *  get all INET address of interface @ifname
 *
 *  if the give @ifc->len is less than contain all INET address, it'll retain some 
 *  of address and set @ifc->len to large enough for containing all INET address
 *  and return -1. if success, return number of address.
 */
int if_get_alladdr(const char *ifname, struct ifconf *ifc)
{
    int             len = 0;
    int             sockfd = -1;

    if (!ifname || !ifc)
	return -1;

    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	memset(_inf_errbuf, 0, sizeof(_inf_errbuf));
	snprintf(_inf_errbuf, INF_ERR_LEN, "create SOCK_DGRAM socket error: %s", strerror(errno));
	return -1;
    }

    len = ifc->ifc_len;

    if (ioctl(sockfd, SIOCGIFCONF, ifc) < 0) {
	memset(_inf_errbuf, 0, sizeof(_inf_errbuf));
	snprintf(_inf_errbuf, INF_ERR_LEN, "create SOCK_DGRAM socket error: %s", strerror(errno));
	close(sockfd);
	return -1;
    }
    
    if (len < ifc->ifc_len) {
	memset(_inf_errbuf, 0, sizeof(_inf_errbuf));
	snprintf(_inf_errbuf, INF_ERR_LEN, "ifc_len is not enough to contain all address");
	close(sockfd);
	return -1;
    }

    close(sockfd);
    return ((ifc->ifc_len) / sizeof(struct ifreq));
}

/*******************************************************************************
 *      functions manipulate IFF_FLAGS in netdevice                            *
 ******************************************************************************/

/*
 *  return non-zero if interface @ifname is up
 *  else return zero
 */
int if_is_up(const char *ifname)
{
    return _inf_getflags(ifname) & IFF_UP;
}

/*
 *  set interface @ifname UP
 */
int if_up(const char *ifname)
{
    short      flags;

    flags = _inf_getflags(ifname);
    if (flags == -1)
	return -1;

    flags = flags | IFF_UP;
    if (_inf_setflags(ifname, flags))
	return -1;

    return 0;
}

/*
 *  set interface @ifname DOWN
 */
int if_down(const char *ifname)
{
    short      flags;

    flags = _inf_getflags(ifname);
    if (flags == -1)
	return -1;

    flags = flags & ~IFF_UP;
    if (_inf_setflags(ifname, flags))
	return -1;

    return 0;
}



/*
 *  return non-zero if interface @ifname is debug enabled
 *  else return zero
 */
int if_is_debug(const char *ifname)
{
    return _inf_getflags(ifname) & IFF_DEBUG;
}

/*
 *  turn on interface @ifname debug
 */
int if_debug(const char *ifname)
{
    short      flags;

    flags = _inf_getflags(ifname);
    if (flags == -1)
	return -1;

    flags = flags | IFF_DEBUG;
    if (_inf_setflags(ifname, flags))
	return -1;

    return 0;
}

/*
 *  turn on interface @ifname debug
 */
int if_nodebug(const char *ifname)
{
    short      flags;

    flags = _inf_getflags(ifname);
    if (flags == -1)
	return -1;

    flags = flags & ~IFF_DEBUG;
    if (_inf_setflags(ifname, flags))
	return -1;

    return 0;
}


/*
 *  return non-zero if interface @ifname L2 address is not set
 *  else return zero
 */
int if_is_noarp(const char *ifname)
{
    return _inf_getflags(ifname) & IFF_NOARP;
}

/*
 *  set interface @ifname no arp reply
 */
int if_noarp(const char *ifname)
{
    short      flags;

    flags = _inf_getflags(ifname);
    if (flags == -1)
	return -1;

    flags = flags | IFF_NOARP;
    if (_inf_setflags(ifname, flags))
	return -1;

    return 0;
}

/*
 *  set interface @ifname arp reply
 */
int if_arp(const char *ifname)
{
    short      flags;

    flags = _inf_getflags(ifname);
    if (flags == -1)
	return -1;

    flags = flags & ~IFF_NOARP;
    if (_inf_setflags(ifname, flags))
	return -1;

    return 0;
}

/*
 *  return non-zero if interface @ifname is in promisc mode
 *  else return zero
 */
int if_is_promisc(const char *ifname)
{
    return _inf_getflags(ifname) & IFF_PROMISC;
}

/*
 *  set interface @ifname promisc mode
 */
int if_promisc(const char *ifname)
{
    short      flags;

    flags = _inf_getflags(ifname);
    if (flags == -1)
	return -1;

    flags = flags | IFF_PROMISC;
    if (_inf_setflags(ifname, flags))
	return -1;

    return 0;
}

/*
 *  set interface @ifname no promisc mode
 */
int if_nopromisc(const char *ifname)
{
    short      flags;

    flags = _inf_getflags(ifname);
    if (flags == -1)
	return -1;

    flags = flags & ~IFF_PROMISC;
    if (_inf_setflags(ifname, flags))
	return -1;

    return 0;
}


/*
 *  return non-zero if interface @ifname address are lost when the 
 *  interface is down, else return zero
 */
int if_is_dynamic(const char *ifname)
{
    return _inf_getflags(ifname) & IFF_DYNAMIC;
}

/*
 *  set interface @ifname dynamic mode
 *
 *  dynamic mode is the addresses are lost when interface goes down.
 */
int if_dynamic(const char *ifname)
{
    short      flags;

    flags = _inf_getflags(ifname);
    if (flags == -1)
	return -1;

    flags = flags | IFF_DYNAMIC;
    if (_inf_setflags(ifname, flags))
	return -1;

    return 0;
}

/*
 *  set interface @ifname not in dynamic mode
 *
 *  dynamic mode is the addresses are lost when interface goes down.
 */
int if_nodynamic(const char *ifname)
{
    short      flags;

    flags = _inf_getflags(ifname);
    if (flags == -1)
	return -1;

    flags = flags & ~IFF_DYNAMIC;
    if (_inf_setflags(ifname, flags))
	return -1;

    return 0;
}

/*
 *  return non-zero if interface @ifname is a broadcast device
 *  else return zero
 */
int if_is_broadcast(const char *ifname)
{
    return _inf_getflags(ifname) & IFF_BROADCAST;
}

/*
 *  return non-zero if interface @ifname is a loopback device
 *  else return zero
 */
int if_is_loopback(const char *ifname)
{
    return _inf_getflags(ifname) & IFF_LOOPBACK;
}


/*
 *  return non-zero if interface @ifname is ppp device
 *  else return zero
 */
int if_is_ppp(const char *ifname)
{
    return _inf_getflags(ifname) & IFF_POINTOPOINT;
}

/*
 *  return non-zero if interface @ifname resource is alloced
 *  else return zero
 */
int if_is_running(const char *ifname)
{
    return _inf_getflags(ifname) & IFF_RUNNING;
}


/*
 *  return non-zero if interface @ifname avoid use of trailers
 *  else return zero
 */
int if_is_notrailers(const char *ifname)
{
    return _inf_getflags(ifname) & IFF_NOTRAILERS;
}

/*
 *  return non-zero if interface @ifname receive all multicast packets
 *  else return zero
 */
int if_is_allmulti(const char *ifname)
{
    return _inf_getflags(ifname) & IFF_ALLMULTI;
}


/*
 *  return non-zero if interface @ifname is master device of a 
 *  load balancing bundle, else return zero
 */
int if_is_master(const char *ifname)
{
    return _inf_getflags(ifname) & IFF_MASTER;
}

/*
 *  return non-zero if interface @ifname is slave device of a 
 *  load balancing bundle, else return zero
 */
int if_is_slave(const char *ifname)
{
    return _inf_getflags(ifname) & IFF_SLAVE;
}

/*
 *  return non-zero if interface @ifname is a multicast device
 *  else return zero
 */
int if_is_multicast(const char *ifname)
{
    return _inf_getflags(ifname) & IFF_MULTICAST;
}

/*
 *  return non-zero if interface @ifname can use ifmap select media type
 *  else return zero
 */
int if_is_portsel(const char *ifname)
{
    return _inf_getflags(ifname) & IFF_PORTSEL;
}

/*
 *  return non-zero if interface @ifname is automedia selection active
 *  else return zero
 */
int if_is_automedia(const char *ifname)
{
    return _inf_getflags(ifname) & IFF_AUTOMEDIA;
}

/*
 *  return 1 if interface @ifname is linked by cable
 *  if linked is down return 0, return -1 on error
 */
int if_is_link_up(const char *ifname)
{
    struct ifreq        ifr;
    u_int16_t           *data, mii;

    if (!ifname)
	return -1;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

    if (_inf_ioctl(&ifr, SIOCGMIIPHY))
	return -1;
    
    data = (u_int16_t *)(&ifr.ifr_data);
    data[1] = 1;
    if (_inf_ioctl(&ifr, SIOCGMIIREG))
	return -1;
   
    mii = data[3];
    
    return (((mii & 0x0016) == 0x0004) ? 1 : 0);
}
