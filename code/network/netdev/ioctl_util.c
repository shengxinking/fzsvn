/**
 *	@file	ioctl_util.c
 *
 *	@brief	implement IOCTL function to get/set
 *		interface attributes.
 *
 *	@author	Forrest.zhang
 *
 *	@date	
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

#include "ioctl_util.h"

#define _IOC_ERRLEN			1023
static char 				_ioc_errbuf[_IOC_ERRLEN + 1];

/**
 *	Call ioctl() syscall and return value.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_ioc_do_ioctl(struct ifreq *ifr, int request)
{
	int sockfd;

	if (!ifr)
		return -1;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		snprintf(_ioc_errbuf, _IOC_ERRLEN, 
			 "<%s:%d> create socket error: %s", 
			 __FILE__, __LINE__,
			 strerror(errno));
		return -1;
	}

	if (ioctl(sockfd, request, ifr) < 0) {
		snprintf(_ioc_errbuf, _IOC_ERRLEN, 
			 "<%s:%d> ioctl(%d) error: %s", 
			 __FILE__, __LINE__, 
			 request, strerror(errno));
		return -1;
	}

	close(sockfd);
	return 0;
}


const char *
ioc_get_error(void)
{
	return _ioc_errbuf;
}

int 
ioc_get_index(const char *ifname)
{
	struct ifreq ifr;

	if (!ifname) {
		snprintf(_ioc_errbuf, _IOC_ERRLEN, 
			 "<%s:%d> invalid argument", 
			 __FILE__, __LINE__);
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

	if (_ioc_do_ioctl(&ifr, SIOCGIFINDEX))
		return -1;

	return ifr.ifr_ifindex;	
}

int 
ioc_get_carrier(const char *ifname)
{
	struct ifreq ifr;
	u_int16_t *data, mii;

	if (!ifname) {
		snprintf(_ioc_errbuf, _IOC_ERRLEN, 
			 "<%s:%d> invalid argument", 
			 __FILE__, __LINE__);
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

	if (_ioc_do_ioctl(&ifr, SIOCGMIIPHY))
		return -1;

	data = (u_int16_t *)(&ifr.ifr_data);
	data[1] = 1;
	if (_ioc_do_ioctl(&ifr, SIOCGMIIREG))
		return -1;

	mii = data[3];

	return (((mii & 0x0016) == 0x0004) ? 1 : 0);
}

int 
ioc_get_alladdr(const char *ifname, struct sockaddr *addrs, size_t naddr)
{
	size_t len;
	int sockfd;
	char buf[8192];
	struct ifconf ifc;
	struct sockaddr *addr;
	int ret;
	int n;
	int count;
	int i;

	if (!ifname || !addrs || naddr < 1) {
		snprintf(_ioc_errbuf, _IOC_ERRLEN, 
			 "<%s:%d> invalid argument", 
			 __FILE__, __LINE__);
		return -1;
	}

	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		snprintf(_ioc_errbuf, _IOC_ERRLEN, 
			 "<%s:%d> create SOCK_DGRAM socket error: %s", 
			 __FILE__, __LINE__,
			 strerror(errno));
		return -1;
	}

	len = sizeof(buf);
	memset(buf, 0, len);
	memset(&ifc, 0, sizeof(ifc));
	ifc.ifc_buf = buf;
	ifc.ifc_len = len;

	ret = ioctl(sockfd, SIOCGIFCONF, &ifc);
	close(sockfd);

	if ( ret< 0) {
		snprintf(_ioc_errbuf, _IOC_ERRLEN, 
			 "<%s:%d> ioctl SIOCGIFCONF error: %s", 
			 __FILE__, __LINE__, 
			 strerror(errno));
		return -1;
	}

	if (len < ifc.ifc_len) {
		snprintf(_ioc_errbuf, _IOC_ERRLEN, 
			 "<%s:%d> ifc_len is small to store all address",
			 __FILE__, __LINE__);
		return -1;
	}

	/* how many IPs in return */
	n = ifc.ifc_len / sizeof(struct ifreq);
	count = 0;

	/* copy @ifname's IP into @ips */
	for (i = 0; i < n; i++)
	{
		if (strcmp(ifname, ifc.ifc_req[i].ifr_name) == 0)
		{
			addr = (struct sockaddr *)&(ifc.ifc_req[i].ifr_addr);
			memcpy(&addrs[count], addr, sizeof(struct sockaddr));
			count++;

			if (count == naddr)
				break;
		}
	}

	return count;
}

char *
ioc_get_name(int index, char *ifname, size_t size)
{
	struct ifreq ifr;

	if (!ifname || !size) {
		snprintf(_ioc_errbuf, _IOC_ERRLEN, 
			 "<%s:%d> invalid argument", 
			 __FILE__, __LINE__);
		return NULL;
	}

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_ifindex = index;

	if (_ioc_do_ioctl(&ifr, SIOCGIFNAME))
		return NULL;

	memcpy(ifname, &ifr.ifr_name, size);
	return ifname;
}

int 
ioc_set_name(const char *name, const char *newname)
{
	struct ifreq ifr;
	int setflag = 0;
	int flags;

	if (!name || !newname) {
		snprintf(_ioc_errbuf, _IOC_ERRLEN, 
			 "<%s:%d> invalid argument", 
			 __FILE__, __LINE__);
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, name, IFNAMSIZ);
	strncpy(ifr.ifr_newname, newname, IFNAMSIZ);

	flags = ioc_get_flags(name);
	if (flags < 0)
		return -1;

	/* if interface up, set it down before set new name */
	if (flags & IFF_UP) {
		if (ioc_set_flags(name, flags & ~IFF_UP))
			return -1;
		setflag = 1;
	}

	if (_ioc_do_ioctl(&ifr, SIOCSIFNAME))
		return -1;

	/* up interface if needed, use @newname */
	if (setflag) {
		if (ioc_set_flags(newname, flags))
			return -1;
	}

	return 0;
}

int 
ioc_get_mtu(const char *ifname)
{
	struct ifreq ifr;

	if (!ifname) {
		snprintf(_ioc_errbuf, _IOC_ERRLEN, 
			 "<%s:%d> invalid argument", 
			 __FILE__, __LINE__);
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

	if (_ioc_do_ioctl(&ifr, SIOCGIFMTU))
		return -1;

	return ifr.ifr_mtu;
}

int 
ioc_set_mtu(const char *ifname, int mtu)
{
	struct ifreq ifr;

	if (!ifname || mtu <= 0)
		return -1;

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	ifr.ifr_mtu = mtu;

	if (_ioc_do_ioctl(&ifr, SIOCSIFMTU))
		return -1;

	return 0;
}

struct sockaddr * 
ioc_get_hwaddr(const char *ifname, struct sockaddr *hwaddr)
{
	struct ifreq ifr;

	if (!ifname || !hwaddr) {
		snprintf(_ioc_errbuf, _IOC_ERRLEN, 
			 "<%s:%d> invalid argument", 
			 __FILE__, __LINE__);
		return NULL;
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

	if (_ioc_do_ioctl(&ifr, SIOCGIFHWADDR))
		return NULL;

	memcpy(hwaddr, &ifr.ifr_hwaddr, sizeof(struct sockaddr));

	return hwaddr;
}

int 
ioc_set_hwaddr(const char *ifname, const struct sockaddr *addr)
{
	struct ifreq ifr;

	if (!ifname || !addr) {
		snprintf(_ioc_errbuf, _IOC_ERRLEN, 
			 "<%s:%d> invalid argument", 
			 __FILE__, __LINE__);
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	memcpy(&ifr.ifr_hwaddr, addr, sizeof(struct sockaddr));

	if (_ioc_do_ioctl(&ifr, SIOCSIFHWADDR))
		return -1;

	return 0;
}

struct sockaddr * 
ioc_get_addr(const char *ifname, struct sockaddr *addr)
{
	struct ifreq ifr;

	if (!ifname || !addr) {
		snprintf(_ioc_errbuf, _IOC_ERRLEN, 
			 "<%s:%d> invalid argument", 
			 __FILE__, __LINE__);
		return NULL;
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

	if (_ioc_do_ioctl(&ifr, SIOCGIFADDR))
		return NULL;

	memcpy(addr, &ifr.ifr_addr, sizeof(struct sockaddr));
	return addr;
}

int 
ioc_set_addr(const char *ifname, const struct sockaddr *addr)
{
	struct ifreq ifr;

	if (!ifname || !addr) {
		snprintf(_ioc_errbuf, _IOC_ERRLEN, 
			 "<%s:%d> invalid argument", 
			 __FILE__, __LINE__);
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	memcpy(&ifr.ifr_addr, addr, sizeof(struct sockaddr));

	if (_ioc_do_ioctl(&ifr, SIOCSIFADDR))
		return -1;

	return 0;
}

struct sockaddr * 
ioc_get_netmask(const char *ifname, struct sockaddr *addr)
{
	struct ifreq ifr;

	if (!ifname || !addr) {
		snprintf(_ioc_errbuf, _IOC_ERRLEN, 
			 "<%s:%d> invalid argument", 
			 __FILE__, __LINE__);
		return NULL;
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

	if (_ioc_do_ioctl(&ifr, SIOCGIFNETMASK))
		return NULL;

	memcpy(addr, &ifr.ifr_netmask, sizeof(struct sockaddr));
	return addr;
}

int 
ioc_set_netmask(const char *ifname, const struct sockaddr *addr)
{
	struct ifreq ifr;

	if (!ifname || !addr) {
		snprintf(_ioc_errbuf, _IOC_ERRLEN, 
			 "<%s:%d> invalid argument", 
			 __FILE__, __LINE__);
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	memcpy(&ifr.ifr_netmask, addr, sizeof(struct sockaddr));

	if (_ioc_do_ioctl(&ifr, SIOCSIFNETMASK))
		return -1;

	return 0;
}

struct sockaddr * 
ioc_get_brdaddr(const char *ifname, struct sockaddr *addr)
{
	struct ifreq ifr;

	if (!ifname || !addr) {
		snprintf(_ioc_errbuf, _IOC_ERRLEN, 
			 "<%s:%d> invalid argument", 
			 __FILE__, __LINE__);
		return NULL;
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

	if (_ioc_do_ioctl(&ifr, SIOCGIFBRDADDR))
		return NULL;

	memcpy(addr, &ifr.ifr_addr, sizeof(struct sockaddr));
	return addr;
}

int 
ioc_set_brdaddr(const char *ifname, const struct sockaddr *addr)
{
	struct ifreq ifr;

	if (!ifname || !addr) {
		snprintf(_ioc_errbuf, _IOC_ERRLEN, 
			 "<%s:%d> invalid argument", 
			 __FILE__, __LINE__);
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	memcpy(&ifr.ifr_addr, addr, sizeof(struct sockaddr));

	if (_ioc_do_ioctl(&ifr, SIOCSIFBRDADDR))
		return -1;

	return 0;
}

struct sockaddr * 
ioc_get_dstaddr(const char *ifname, struct sockaddr *addr)
{
	struct ifreq ifr;

	if (!ifname || !addr) {
		snprintf(_ioc_errbuf, _IOC_ERRLEN, 
			 "<%s:%d> invalid argument", 
			 __FILE__, __LINE__);
		return NULL;
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

	if (_ioc_do_ioctl(&ifr, SIOCGIFDSTADDR))
		return NULL;

	memcpy(addr, &ifr.ifr_dstaddr, sizeof(struct sockaddr));
	return addr;
}

int 
ioc_set_dstaddr(const char *ifname, const struct sockaddr *addr)
{
	struct ifreq ifr;

	if (!ifname || !addr) {
		snprintf(_ioc_errbuf, _IOC_ERRLEN, 
			 "<%s:%d> invalid argument", 
			 __FILE__, __LINE__);
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	memcpy(&ifr.ifr_dstaddr, addr, sizeof(struct sockaddr));

	if (_ioc_do_ioctl(&ifr, SIOCSIFDSTADDR))
		return -1;

	return 0;
}


int 
ioc_str2flags(const char *name)
{
	if (!name) {
		snprintf(_ioc_errbuf, _IOC_ERRLEN, 
			 "<%s:%d> invalid argument", 
			 __FILE__, __LINE__);
		return -1;
	}

	if (strcmp(name, "IFF_UP") == 0)
		return IFF_UP;
	else if (strcmp(name, "IFF_BROADCAST") == 0)
		return IFF_BROADCAST;
	else if (strcmp(name, "IFF_DEBUG") == 0)
		return IFF_DEBUG;
	else if (strcmp(name, "IFF_LOOPBACK") == 0)
		return IFF_LOOPBACK;
	else if (strcmp(name, "IFF_POINTOPOINT") == 0)
		return IFF_POINTOPOINT;
	else if (strcmp(name, "IFF_NOTRAILERS") == 0)
		return IFF_NOTRAILERS;
	else if (strcmp(name, "IFF_RUNNING") == 0)
		return IFF_RUNNING;
	else if (strcmp(name, "IFF_NOARP") == 0)
		return IFF_NOARP;
	else if (strcmp(name, "IFF_PROMISC") == 0)
		return IFF_PROMISC;
	else if (strcmp(name, "IFF_ALLMULTI") == 0)
		return IFF_ALLMULTI;
	else if (strcmp(name, "IFF_MASTER") == 0)
		return IFF_MASTER;
	else if (strcmp(name, "IFF_SLAVE") == 0)
		return IFF_SLAVE;
	else if (strcmp(name, "IFF_MULTICAST") == 0)
		return IFF_MULTICAST;
	else if (strcmp(name, "IFF_PORTSEL") == 0)
		return IFF_PORTSEL;
	else if (strcmp(name, "IFF_AUTOMEDIA") == 0)
		return IFF_AUTOMEDIA;
	else if (strcmp(name, "IFF_DYNAMIC") == 0)
		return IFF_DYNAMIC;
	else
		return -1;
}

const char *
ioc_flags2str(int flags)
{
	switch (flags) {
	case IFF_UP:
		return "IFF_UP";
	case IFF_BROADCAST:
		return "IFF_BROADCAST";
	case IFF_DEBUG:
		return "IFF_DEBUG";
	case IFF_LOOPBACK:
		return "IFF_LOOPBACK";
	case IFF_POINTOPOINT:
		return "IFF_POINTOPOINT";
	case IFF_NOTRAILERS:
		return "IFF_NOTRAILERS";
	case IFF_RUNNING:
		return "IFF_RUNNING";
	case IFF_NOARP:
		return "IFF_NOARP";
	case IFF_PROMISC:
		return "IFF_PROMISC";
	case IFF_ALLMULTI:
		return "IFF_ALLMULTI";
	case IFF_MASTER:
		return "IFF_MASTER";
	case IFF_SLAVE:
		return "IFF_SLAVE";
	case IFF_MULTICAST:
		return "IFF_MULTICAST";
	case IFF_PORTSEL:
		return "IFF_PORTSEL";
	case IFF_AUTOMEDIA:
		return "IFF_AUTOMEDIA";
	case IFF_DYNAMIC:
		return "IFF_DYNAMIC";
	default:
		return NULL;
	}
	return NULL;
}

int   
ioc_get_flags(const char *ifname)
{
	struct ifreq ifr;
	int flags;

	if (!ifname) {
		snprintf(_ioc_errbuf, _IOC_ERRLEN, 
			 "<%s:%d> invalid argument", 
			 __FILE__, __LINE__);
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

	if (_ioc_do_ioctl(&ifr, SIOCGIFFLAGS))
		return -1;

	flags = (unsigned short)ifr.ifr_flags;

	return flags;
}

int 
ioc_set_flags(const char *ifname, int flags)
{
	struct ifreq ifr;

	if (!ifname) {
		snprintf(_ioc_errbuf, _IOC_ERRLEN, 
			 "<%s:%d> invalid argument", 
			 __FILE__, __LINE__);
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	ifr.ifr_flags = (unsigned short)flags;

	if (_ioc_do_ioctl(&ifr, SIOCSIFFLAGS))
		return -1;

	return 0;
}

