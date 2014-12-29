/*
 *  the test program for iflib
 *
 *  write by Forrest.zhang
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <netinet/ether.h>

#include "ioctl_util.h"
#include "ethtool_util.h"
#include "bonding_util.h"

/**
 *	convert IP address(uint32_t) to dot-decimal string
 *	like xxx.xxx.xxx.xxx 
 */
#ifndef NIPQUAD
#define NIPQUAD(addr)			\
	((unsigned char *)&addr)[0],	\
	((unsigned char *)&addr)[1],	\
	((unsigned char *)&addr)[2],	\
	((unsigned char *)&addr)[3]
#define NIPQUAD_FMT "%u.%u.%u.%u"
#endif	/* end of NIPQUAD */

/* test type */
enum {
	TEST_IOCTL,
	TEST_ETHTOOL,
	TEST_VLAN,
	TEST_BRIDGE,
	TEST_BONDING,
};

static int 		_g_type = 0;	/* test type */
static u_int32_t	_g_cmd = 0;	/* test commands */
static char		_g_name[IFNAMSIZ];/* interface name */
static char		_g_newname[IFNAMSIZ];/* interface new name */
static char		_g_phyname[IFNAMSIZ];/* physical interface */
static int		_g_vlanid;	/* the vlan id */
static int		_g_flags;	/* interface flags */
static int		_g_clear_flags;	/* clear interface flags if 1 */ 
static char		_g_hwaddr[20];	/* hardware address */
static u_int32_t	_g_inetaddr;	/* inet address */
static int		_g_mtu = 1500;	/* MTU value */
static bonding_arg_t	_g_bond;	/* bonding arg */

static void 
_ioctl_usage(void)
{
	printf("\tioctl options:\n");
	printf("\t\t-n\t<name>\tthe interface name\n");
	printf("\t\t-d\t\tget interface index\n");
	printf("\t\t-c\t\tget carrier\n");
	printf("\t\t-a\t\tget all IPv4 address\n");
	printf("\t\t-m\t\tget mtu\n");
	printf("\t\t-M\t<N>\tset mtu\n");
	printf("\t\t-h\t\tget hardware address\n");
	printf("\t\t-H\t<MAC>\tset hardware address\n");
	printf("\t\t-i\t\tget IP/netmask/broadcast address\n");
	printf("\t\t-I <IP/mask>\tset IP/netmask/broadcast address\n");
	printf("\t\t-f\tget flags\n");
	printf("\t\t-F <IFF_X>\tset flags\n");
	printf("\t\t-R\t<name>\trename the interface\n");
	printf("\t\t-h\t\tshow ioctl help message\n");
}

/* ioctl commands */
#define IOCTL_GET_INDEX			0x00000001
#define IOCTL_GET_CARRIER		0x00000002
#define IOCTL_GET_ALLADDR		0x00000004
#define	IOCTL_GET_MTU			0x00000010
#define	IOCTL_SET_MTU			0x00100000
#define	IOCTL_GET_HWADDR		0x00000020
#define	IOCTL_SET_HWADDR		0x00200000
#define	IOCTL_GET_INETADDR		0x00000040
#define	IOCTL_SET_INETADDR		0x00400000
#define IOCTL_GET_NETMASK		0x00000080
#define IOCTL_SET_NETMASK		0x00800000
#define IOCTL_GET_BRDADDR		0x00000100
#define IOCTL_SET_BRDADDR		0x01000000
#define IOCTL_GET_DSTADDR		0x00000200
#define IOCTL_SET_DSTADDR		0x02000000
#define IOCTL_GET_FLAGS			0x00000400
#define IOCTL_SET_FLAGS			0x04000000
#define	IOCTL_SET_NAME			0x08000000
#define	IOCTL_GET_ALL			0x0000ffff

static int 
_parse_ioctl_cmd(int argc, char **argv)
{
	char opt;
	char optstr[] = ":n:dcamM:hH:iI:fF:R:";

	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		switch (opt) 
		{		
		case 'n':
			strncpy(_g_name, optarg, IFNAMSIZ - 1);
			break;
		case 'd':
			_g_cmd |= IOCTL_GET_INDEX;
			break;
		case 'c':
			_g_cmd |= IOCTL_GET_CARRIER;
			break;
		case 'a':
			_g_cmd |= IOCTL_GET_ALLADDR;
			break;
		case 'm':
			_g_cmd |= IOCTL_GET_MTU;
			break;
		case 'M':
			_g_mtu = atoi(optarg);
			_g_cmd |= IOCTL_SET_MTU;
			break;
		case 'h':
			_g_cmd |= IOCTL_GET_HWADDR;
			break;
		case 'H':
			strncpy(_g_hwaddr, optarg, sizeof(_g_hwaddr) - 1);
			_g_cmd |= IOCTL_SET_HWADDR;
			break;		
		case 'i':
			_g_cmd |= IOCTL_GET_INETADDR;
			break;
		case 'I':
			_g_inetaddr = inet_addr(optarg);
			_g_cmd |= IOCTL_SET_INETADDR;
			break;
		case 'f':
			_g_cmd |= IOCTL_GET_FLAGS;
			break;
		case 'F':
			if (optarg[0] == '~') {
				_g_clear_flags = 1;
				_g_flags = ioc_str2flags(optarg + 1);
			}
			else {
				_g_flags = ioc_str2flags(optarg);
			}

			if (_g_flags < 0) {
				printf("invalid flags %s\n", optarg);
				return -1;
			}
			_g_cmd |= IOCTL_SET_FLAGS;
			break;
		case 'R':
			strncpy(_g_newname, optarg, IFNAMSIZ - 1);
			_g_cmd |= IOCTL_SET_NAME;
			break;
		case ':':
			printf("Option %c missing argument\n", optopt);
			return -1;
		case '?':
			printf("Unknowed option %c\n", optopt);
			return -1;
		}
	}

	if (_g_name[0] == 0) {
		_ioctl_usage();
		return -1;
	}

	if (optind != argc) {
		_ioctl_usage();
		return -1;
	}

	if (_g_cmd == 0)
		_g_cmd = IOCTL_GET_ALL;

	return 0;	
}

static void 
_ethtool_usage(void)
{
	printf("\tethtool options:\n");
	printf("\t\t-n\t<name>\tthe interface name\n");
	printf("\t\t-s\t\tget speed\n");
	printf("\t\t-p\t\tget port media\n");
	printf("\t\t-c\t\tget carrier status\n");
	printf("\t\t-e\t\tget eeprom\n");
	printf("\t\t-h\t\tshow ethtool help message\n");
}

/* ethtool commands */
#define	ETHTOOL_GET_SPEED		0x00000001
#define	ETHTOOL_GET_PORT		0x00000002
#define	ETHTOOL_GET_CARRIER		0x00000004
#define	ETHTOOL_GET_AUTONEG		0x00000008
#define	ETHTOOL_GET_EEPROM_SIZE		0x00000010
#define	ETHTOOL_GET_EEPROM		0x00000020
#define ETHTOOL_GET_ALL			0x0000ffff

static int 
_parse_ethtool_cmd(int argc, char **argv)
{
	char opt;
	char optstr[] = ":n:spceh";

	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		switch (opt) 
		{
		case 'n':
			strncpy(_g_name, optarg, IFNAMSIZ - 1);
			break;
		case 's':
			_g_cmd |= ETHTOOL_GET_SPEED;
			break;
		case 'p':
			_g_cmd |= ETHTOOL_GET_PORT;
			break;
		case 'c':
			_g_cmd |= ETHTOOL_GET_CARRIER;
			break;
		case 'e':
			_g_cmd |= ETHTOOL_GET_EEPROM;
			break;
		case 'h':
			_ethtool_usage();
			exit(0);
		case ':':
			printf("Option %c missing argument\n", optopt);
			return -1;
		case '?':
			printf("Unknowed option %c\n", optopt);
			return -1;
		}
	}

	if (_g_name[0] == 0) {
		_ethtool_usage();
		return -1;
	}

	/* set value */
	if (optind != argc)
	{
		_ethtool_usage();
		return -1;
	}

	if (_g_cmd == 0)
		_g_cmd = ETHTOOL_GET_ALL;

	return 0;
}

static void 
_vlan_usage(void)
{
	printf("\tvlan options:\n");
	printf("\t\t-n\t<name>\tthe interface name\n");
	printf("\t\t-p\t<name>\tthe vlan physical interface name\n");
	printf("\t\t-i\t<NNN>\tthe vlan id\n");
	printf("\t\t-A\t\tadd new vlan\n");
	printf("\t\t-D\t\tdelete vlan\n");
	printf("\t\t-h\t\tshow vlan help message\n");
}

#define	VLAN_ADD_VLAN			0x00010000
#define VLAN_DEL_VLAN			0x00020000

static int 
_parse_vlan_cmd(int argc, char **argv)
{
	char opt;
	char optstr[] = ":n:p:i:ADh";

	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		switch (opt) 
		{
		case 'n':
			strncpy(_g_name, optarg, IFNAMSIZ - 1);
			break;
		case 'p':
			strncpy(_g_phyname, optarg, IFNAMSIZ - 1);
			break;
		case 'i':
			_g_vlanid = atoi(optarg);
			break;
		case 'A':
			_g_cmd |= VLAN_ADD_VLAN;
			break;
		case 'D':
			_g_cmd |= VLAN_DEL_VLAN;
			break;
		case 'h':
			_vlan_usage();
			exit(0);
		case ':':
			printf("Option %c missing argument\n", optopt);
			return -1;
		case '?':
			printf("Unknowed option %c\n", optopt);
			return -1;
		}
	}

	if (_g_name[0] == 0) {
		_vlan_usage();
		return -1;
	}

	if (optind != argc) {
		_vlan_usage();
		return -1;
	}

	return 0;
}


static void 
_bridge_usage(void)
{
	printf("\tbridge options:\n");
	printf("\t\t-A\t\tadd a new bridge\n");
	printf("\t\t-D\t\tdelete a bridge\n");
	printf("\t\t-n\t<name>\tthe bridge name\n");
	printf("\t\t-s\t\tget slave interface\n");
	printf("\t\t-S\t<name>\tset slave interface\n");
	printf("\t\t-h\t\tshow bridge help message\n");
}

#define	BRIDGE_GET_STP			0x00000001
#define	BRIDGE_SET_STP			0x00010000
#define BRIDGE_GET_HAIRPIN		0x00000002
#define BRIDGE_SET_HAIRPIN		0x00020000
#define BRIDGE_GET_AGE			0x00000004
#define BRIDGE_SET_AGE			0x00040000
#define BRIDGE_GET_FD			0x00000008
#define BRIDGE_SET_FD			0x00080000
#define BRIDGE_GET_PATHCOST		0x00000010
#define BRIDGE_SET_PATHCOST		0x00100000
#define BRIDGE_GET_HELLO		0x00000020
#define BRIDGE_SET_HELLO		0x00200000
#define BRIDGE_GET_IF			0x00000040
#define BRIDGE_ADD_IF			0x00400000
#define BRIDGE_DEL_IF			0x00800000
#define	BRIDGE_ADD_BR			0x01000000
#define BRIDGE_DEL_BR			0x02000000
#define BRIDGE_GET_ALL			0x0000ffff

static int 
_parse_bridge_cmd(int argc, char **argv)
{
	char opt;
	char optstr[] = ":n:ADsS:h";

	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		switch (opt) 
		{		
		case 'n':
			strncpy(_g_name, optarg, IFNAMSIZ - 1);
			break;
		case 'A':
			_g_cmd |= BRIDGE_ADD_BR;
			break;
		case 'D':
			_g_cmd |= BRIDGE_DEL_BR;
			break;
		case 's':
			_g_cmd |= BRIDGE_GET_IF;
			break;
		case 'S':
			_g_cmd |= BRIDGE_ADD_IF;
			break;
		case 'h':
			_bridge_usage();
			exit(0);
		case ':':
			printf("Option %c missing argument\n", optopt);
			return -1;
		case '?':
			printf("Unknowed option %c\n", optopt);
			return -1;
		}
	}

	if (_g_name[0] == 0)
	{
		_bridge_usage();
		return -1;
	}

	if (optind != argc)
	{
		_bridge_usage();
		return -1;
	}

	if (_g_cmd == 0)
		_g_cmd = BRIDGE_GET_ALL;

	return 0;
}

static void 
_bonding_usage(void)
{
	printf("\tbonding options:\n");
	printf("\t\t-n\t<name>\tbonding name\n");
	printf("\t\t-s\t\tget slave interface\n");
	printf("\t\t-S\t<name>\tset slave interface\n");
	printf("\t\t-o\t\tget mode\n");
	printf("\t\t-O\t<N>\tset mode\n");
	printf("\t\t-l\t\tget lacp_rate\n");
	printf("\t\t-L\t<N>\tset lacp_rate\n");
	printf("\t\t-p\t\tget xmit_hash_policy\n");
	printf("\t\t-P\t<N>\tset xmit_hash_policy\n");
	printf("\t\t-i\t\tget miimon interval value\n");
	printf("\t\t-I\t<NNN>\tset miimon interval value\n");
	printf("\t\t-A\t\tadd a new bonding\n");
	printf("\t\t-D\t\tdelete a bonding\n");
	printf("\t\t-h\t\tshow bonding help message\n");
}

/* bonding commands */
#define	BONDING_GET_MODE		0x00000001
#define	BONDING_SET_MODE		0x00010000
#define	BONDING_GET_XMIT_HASH_POLICY	0x00000002
#define	BONDING_SET_XMIT_HASH_POLICY	0x00020000
#define	BONDING_GET_LACP_RATE		0x00000004
#define	BONDING_SET_LACP_RATE		0x00040000
#define	BONDING_GET_MIIMON		0x00000008
#define	BONDING_SET_MIIMON		0x00080000
#define BONDING_GET_IF			0x00000010
#define BONDING_ADD_IF			0x00100000
#define BONDING_DEL_IF			0x00200000
#define BONDING_ADD_BD			0x00400000
#define BONDING_DEL_BD			0x00800000
#define	BONDING_GET_ALL			0x0000ffff

static int 
_parse_bonding_cmd(int argc, char **argv)
{
	char opt;
	char optstr[] = ":n:sS:oO:lL:pP:iI:ADh";

	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		switch (opt) 
		{		
		case 'n':
			strncpy(_g_name, optarg, IFNAMSIZ - 1);
			break;

		case 's':
			_g_cmd |= BONDING_GET_IF;
			break;
		case 'S':
			strncpy(_g_bond.slaves[_g_bond.nslave], 
				optarg, IFNAMSIZ - 1);
			_g_bond.nslave++;
			_g_cmd |= BONDING_ADD_IF;
			break;
		case 'm':
			_g_cmd |= BONDING_GET_MODE;
			break;
		case 'M':
			_g_bond.mode = atoi(optarg);
			_g_cmd |= BONDING_SET_MODE;
			break;
		case 'p':
			_g_cmd |= BONDING_GET_XMIT_HASH_POLICY;
			break;
		case 'P':
			_g_bond.xmit_hash_policy = atoi(optarg);
			_g_cmd |= BONDING_SET_XMIT_HASH_POLICY;
			break;
		case 'l':
			_g_cmd |= BONDING_GET_LACP_RATE;
			break;
		case 'L':
			_g_bond.lacp_rate = atoi(optarg);
			_g_cmd |= BONDING_SET_LACP_RATE;
			break;
		case 'i':
			_g_cmd |= BONDING_GET_MIIMON;
			break;
		case 'I':
			_g_bond.miimon = atoi(optarg);
			_g_cmd |= BONDING_SET_MIIMON;
			break;
		case 'A':
			_g_cmd |= BONDING_ADD_BD;
			break;
		case 'D':
			_g_cmd |= BONDING_DEL_BD;
			break;
		case 'h':
			_bonding_usage();
			exit(0);

		case ':':
			printf("Option %c missing argument\n", optopt);
			_bonding_usage();
			return -1;

		case '?':
			printf("Unknowed option %c\n", optopt);
			_bonding_usage();
			return -1;
		}
	}

	if (_g_name[0] == 0)
	{
		_bonding_usage();
		return -1;
	}

	/* set value */
	if (optind != argc)
	{
		_bonding_usage();
		return -1;
	}

	return 0;
}

static void 
_usage(void) 
{
	printf("netdev_test <ioctl|ethtool|vlan|bridge|bonding> <options>\n\n");
	printf("\n");
	printf("\t-h\t\tshow full help message\n");
	printf("\n");
	_ioctl_usage();
	printf("\n");
	_ethtool_usage();
	printf("\n");
	_vlan_usage();
	printf("\n");
	_bridge_usage();
	printf("\n");
	_bonding_usage();
}

static int 
_parse_cmd(int argc, char **argv)
{
	if (argc < 2) {
		_usage();
		return -1;
	}

	if (strcmp(argv[1], "ioctl") == 0) 
		_g_type = TEST_IOCTL;
	else if (strcmp(argv[1], "ethtool") == 0)
		_g_type = TEST_ETHTOOL;
	else if (strcmp(argv[1], "vlan") == 0)
		_g_type = TEST_VLAN;
	else if (strcmp(argv[1], "bridge") == 0)
		_g_type = TEST_BRIDGE;
	else if (strcmp(argv[1], "bonding") == 0)
		_g_type = TEST_BONDING;
	else if (strcmp(argv[1], "-h") == 0) {
		_usage();
		exit(0);
	}
	else {
		printf("invalid test type %s\n\n", argv[1]);
		_usage();
		return -1;
	}

	argc--;
	argv++;

	switch (_g_type) {

	case TEST_IOCTL:
		return _parse_ioctl_cmd(argc, argv);
	case TEST_ETHTOOL:
		return _parse_ethtool_cmd(argc, argv);
	case TEST_VLAN:
		return _parse_vlan_cmd(argc, argv);
	case TEST_BRIDGE:
		return _parse_bridge_cmd(argc, argv);
	case TEST_BONDING:
		return _parse_bonding_cmd(argc, argv);
	default:
		break;
	}

	return -1;
}

int 
_test_ioctl(void)
{
	struct sockaddr addr;
	int ret = 0;
	struct sockaddr addrs[64];
	int naddr;
	int flags;

	/* get index */
	if (_g_cmd & IOCTL_GET_INDEX)
		printf("<%s> ifindex: %d\n", _g_name, 
		       ioc_get_index(_g_name));
      
	
	/* get carrier */
	if (_g_cmd & IOCTL_GET_CARRIER) {
		ret = ioc_get_carrier(_g_name);
		if (ret < 0) {
			printf("<%s> get carrier failed: %s\n", _g_name,
			       ioc_get_error());
		}
		else {
			printf("<%s> carrier: %d\n", _g_name, ret);
		}
	}

	/* get all INET address */
	if (_g_cmd & IOCTL_GET_ALLADDR) {
		naddr = sizeof(addrs)/sizeof(addrs[0]);
		ret = ioc_get_alladdr(_g_name, addrs, naddr); 
		if (ret < 0) {
			printf("<%s> get alladdr error: %s\n", _g_name, 
			       ioc_get_error());
			return -1;
		}
	}

	/* get MTU */
	if (_g_cmd & IOCTL_GET_MTU) 
		printf("<%s> mtu: %d\n", _g_name, ioc_get_mtu(_g_name));

	/* set MTU */
	if (_g_cmd & IOCTL_SET_MTU) {
		if (ioc_set_mtu(_g_name, _g_mtu))
			printf("<%s> set mtu error: %s\n", _g_name, 
			       ioc_get_error());
		else 
			printf("<%s> set mtu %d success\n", _g_name, _g_mtu);
	}

	/* get HWADDR */
	if (_g_cmd & IOCTL_GET_HWADDR) {
		if (!ioc_get_hwaddr(_g_name, &addr))
			printf("<%s> get hwaddr error: %s\n", _g_name, 
			       ioc_get_error());
		else
			printf("<%s> hwaddr: %s\n", _g_name, 
			       ether_ntoa((struct ether_addr*)addr.sa_data));
	}

	/* set HWADDR */
	if (_g_cmd & IOCTL_SET_HWADDR) {
		memset(&addr, 0, sizeof(addr));
		addr.sa_family = ARPHRD_ETHER;
		memcpy(addr.sa_data, ether_aton(_g_hwaddr), 
			sizeof(struct ether_addr));
		if (ioc_set_hwaddr(_g_name, &addr))
			printf("<%s> set hwaddr error: %s\n", _g_name, 
			       ioc_get_error());
		else 
			printf("<%s> set hwaddr %s success\n", _g_name,
			       ether_ntoa((struct ether_addr*)addr.sa_data));
	}			

	/* get INET address */
	if (_g_cmd & IOCTL_GET_INETADDR) {

	}
	
	/* set INET address */
	if (_g_cmd & IOCTL_SET_INETADDR) {

	}

	/* get IFF_FLAGS */
	if (_g_cmd & IOCTL_GET_FLAGS) {
		flags = ioc_get_flags(_g_name);
		if (flags < 0) {
			printf("<%s> get flags failed: %s\n", _g_name, 
			       ioc_get_error());
			return -1;
		}
		
		printf("<%s> up: %s\n", _g_name,
		       flags & IFF_UP ? "UP" : "DOWN");
		printf("<%s> broadcast: %s\n", _g_name, 
		       flags & IFF_BROADCAST ? "BROADCAST" : "UNICAST");
		printf("<%s> debug: %s\n", _g_name, 
		       flags & IFF_DEBUG ? "DEBUG" : "not DEBUG");
		printf("<%s> loopback: %s\n", _g_name, 
		       flags & IFF_LOOPBACK ? "LOOPBACK" : "not LOOPBACK");
		printf("<%s> ppp: %s\n", _g_name, 
		       flags & IFF_POINTOPOINT? "PPP" : "not PPP");
		printf("<%s> notrailers: %s\n", _g_name, 
		       flags & IFF_NOTRAILERS ? "not TRAILERS" : "TRAILERS");
		printf("<%s> running: %s\n", _g_name, 
		       flags & IFF_RUNNING ? "RUNNING" : "not RUNNING");
		printf("<%s> noarp: %s\n", _g_name, 
		       flags & IFF_NOARP ? "not ARP" : "ARP");
		printf("<%s> promisc: %s\n", _g_name, 
		       flags & IFF_PROMISC ? "PROMISC" : "not PROMISC");
		printf("<%s> allmulti: %s\n", _g_name, 
		       flags & IFF_ALLMULTI? "ALLMULTI" : "not ALLMULTI");
		printf("<%s> master: %s\n", _g_name, 
		       flags & IFF_MASTER? "MASTER" : "not MASTER");
		printf("<%s> slave: %s\n", _g_name, 
		       flags & IFF_SLAVE ? "SLAVE" : "not SLAVE");
		printf("<%s> multicast: %s\n", _g_name, 
		       flags & IFF_MULTICAST ? "MULTICAST" : "not MULTICAST");
		printf("<%s> portsel: %s\n", _g_name, 
		       flags & IFF_PORTSEL ? "PORTSEL" : "not PORTSEL");
		printf("<%s> automedia: %s\n", _g_name, 
		       flags & IFF_AUTOMEDIA ? "AUTOMEDIA" : "not AUTOMEDIA");
		printf("<%s> dynamic: %s\n", _g_name, 
		       flags & IFF_DYNAMIC ? "DYNAMIC" : "not DYNAMIC");
	}
	
	if (_g_cmd & IOCTL_SET_FLAGS) {
		flags = ioc_get_flags(_g_name);
		if (flags < 0) {
			printf("<%s> get flags failed: %s\n", _g_name, 
			       ioc_get_error());
			return -1;
		}

		if (_g_clear_flags)
			flags &= ~_g_flags;
		else
			flags |= _g_flags;

		if (ioc_set_flags(_g_name, flags)) {
			if (_g_clear_flags)
				printf("<%s> clear flags %x failed: %s\n", 
				       _g_name, _g_flags, ioc_get_error());
			else
				printf("<%s> set flags %x failed: %s\n", 
				       _g_name, _g_flags, ioc_get_error());
			return -1;
		}
		else {
			if (_g_clear_flags)
				printf("<%s> clear flags %s success\n", 
				       _g_name, ioc_flags2str(_g_flags));
			else
				printf("<%s> set flags %s success\n", 
				       _g_name, ioc_flags2str(_g_flags));
			return -1;
		}
	}
	
	return 0;
}


int 
_test_ethtool(void)
{
	int fd;
	char buf[32768] = {0};

	if (_g_cmd & ETHTOOL_GET_SPEED)
		printf("<%s> speed: %d\n", _g_name, 
		       eth_get_speed(_g_name));
	if (_g_cmd & ETHTOOL_GET_PORT)
		printf("<%s> media: %d\n", _g_name, 
		       eth_get_port(_g_name));
	if (_g_cmd & ETHTOOL_GET_AUTONEG)
		printf("<%s> autoneg: %d\n", _g_name, 
		       eth_get_autoneg(_g_name));
	if (_g_cmd & ETHTOOL_GET_CARRIER)
		printf("<%s> link: %d\n", _g_name, 
		       eth_get_carrier(_g_name));
	if (_g_cmd & ETHTOOL_GET_EEPROM_SIZE)
		printf("<%s> eeprom size: %d\n", _g_name, 
		       eth_get_eeprom_size(_g_name));
	if (_g_cmd & ETHTOOL_GET_EEPROM) {
		if (eth_get_eeprom(_g_name, buf, 0x2580 + 0x190, 16))
			return -1;
		fd = open("eeprom", O_RDWR|O_CREAT, 0644);
		if (fd < 0)
			return -1;

		write(fd, buf, sizeof(buf));
		close(fd);
	}

	return 0;
}

int 
_test_vlan()
{
	return 0;
}

int 
_test_bridge()
{
	return 0;
}

int 
_test_bonding()
{
	return 0;
}

int 
main(int argc, char **argv)
{
	if (_parse_cmd(argc, argv))
		return -1;

	switch (_g_type) {

	case TEST_IOCTL:
		_test_ioctl();
		break;		
	case TEST_ETHTOOL:
		_test_ethtool();
		break;
	case TEST_VLAN:
		_test_vlan();
		break;
	case TEST_BRIDGE:
		_test_bridge();
		break;
	case TEST_BONDING:
		_test_bonding();
		break;
	default:
		break;
	}

	return 0;
}

