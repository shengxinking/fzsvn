/*
 *  the test program for iflib
 *
 *  write by Forrest.zhang
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <netinet/ether.h>

#include "netdev.h"

/* command */
enum {
	NETDEV_GET,
	NETDEV_SET,
};

/* command type */
enum {
	NETDEV_HWADDR = 1,
	NETDEV_MTU,
	NETDEV_INET,
	NETDEV_FLAG,
	NETDEV_ETHTOOL,
};


static int	ifindex = 0;
static char	ifname[IFNAMSIZ + 1] = {0};
static int	ifcmd = NETDEV_GET;
static int 	iftype = 0;
static char	*ifarg = NULL;

static void 
_usage(void) 
{
	printf("netdev_test <options> <args>\n");
	printf("\t-n\tinterface name\n");
	printf("\t-g\tget value\n");
	printf("\t-s\tset value\n");
	printf("\t-a\thardware address\n");
	printf("\t-m\tmtu size\n");
	printf("\t-i\tINET address\n");
	printf("\t-f\tflags\n");
	printf("\t-e\teeprom\n");
	printf("\t-h\tshow help message\n");
}

static int 
_parse_cmd(int argc, char **argv)
{
	char opt;
	char optstr[] = ":n:gsamifpeh";

	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		switch (opt) {
		
		case 'n':
			strncpy(ifname, optarg, IFNAMSIZ);
			break;

		case 'g':
			ifcmd = NETDEV_GET;
			break;

		case 's':
			ifcmd = NETDEV_SET;
			break;

		case 'a':
			iftype = NETDEV_HWADDR;
			break;

		case 'm':
			iftype = NETDEV_MTU;
			break;

		case 'i':
			iftype = NETDEV_INET;
			break;

		case 'f':
			iftype = NETDEV_FLAG;
			break;

		case 'e':
			iftype = NETDEV_ETHTOOL;
			break;

		case 'h':
			return -1;
		case ':':
			printf("Option %c missing argument\n", optopt);
			return -1;

		case '?':
			printf("Unknowed option %c\n", optopt);
			return -1;
		}
	}

	if (ifname[0] == 0)
		return -1;

	/* set value */
	if (optind == argc)
		return 0;

	/* have last arg */
	ifarg = argv[optind];

	optind++;
	if (optind != argc)
		return -1;

	return 0;
}


int 
_do_ioctl(void)
{
	struct sockaddr    addr;
	struct sockaddr_in *addr_in = NULL;
	char               buf[1024] = {0};
	int                mtu = 0;
	struct ifconf      ifc;
	int                ret = 0;
	char               address[20] = {0};
	int                i;

	/* get interface index */
	ifindex = if_get_index(ifname);
	if (ifindex < 0)
		printf("if_get_index error: %s\n", if_error());
	printf("%s ifindex is %d\n", ifname, if_get_index(ifname));

	if (ifcmd == NETDEV_GET) {
	
		switch (iftype) {

		/* get HWADDR */
		case NETDEV_HWADDR:
			if (!if_get_hwaddr(ifname, &addr))
				printf("if_get_hwaddr error: %s\n", if_error());
			printf("%s hwaddr is %s\n", ifname, 
			       ether_ntoa((const struct ether_addr*)addr.sa_data));
			break;
			
		/* get MTU */
		case NETDEV_MTU:
			printf("%s mtu is %d\n", ifname, if_get_mtu(ifname));
			break;

		/* get all INET address */
		case NETDEV_INET:
			memset(buf, 0, sizeof(buf));
			ifc.ifc_buf = buf;
			ifc.ifc_len = 40;
			if ( (ret = if_get_alladdr(ifname, &ifc)) < 0)
				printf("if_get_alladdr error: %s\n", if_error());

			for (i = 0; i < ret; i++) {
				addr_in = (struct sockaddr_in*)&(ifc.ifc_req[i].ifr_addr);
				printf("%s address is %s\n", ifc.ifc_req[i].ifr_name, 
					inet_ntop(AF_INET, &(addr_in->sin_addr), address, 20));
			}

			break;

		/* get IFF_FLAGS */
		case NETDEV_FLAG:
			printf("%s is %s\n", ifname, 
				if_is_up(ifname) ? "UP" : "DOWN");
			printf("%s is %s\n", ifname, 
				if_is_broadcast(ifname) ? "BROADCAST" : "UNICAST");
			printf("%s is %s\n", ifname, 
				if_is_promisc(ifname) ? "PROMISC" : "NO PROMISC");
			printf("%s is %s\n", ifname, 
				if_is_noarp(ifname) ? "NOARP" : "ARP");
			printf("%s is %s\n", ifname, 
				if_is_dynamic(ifname) ? "DYNAMIC" : "NO DYNAMIC");
			printf("%s is %s\n", ifname, 
				if_is_loopback(ifname) ? "LOOP" : "not LOOP");
			printf("%s is %s\n", ifname, 
				if_is_running(ifname) ? "RUNNING" : "not RUNNING");
			printf("%s is %s\n", ifname, 
				if_is_ppp(ifname) ? "PPP" : "not PPP");
			printf("%s if %s\n", ifname, 
				if_is_debug(ifname) ? "DEBUG" : "not DEBUG");
			printf("%s if %s\n", ifname, 
				if_is_master(ifname) ? "MASTER" : "not MASTER");
			printf("%s if %s\n", ifname, 
				if_is_slave(ifname) ? "SLAVE" : "not SLAVE");
			printf("%s if %s\n", ifname, 
				if_is_multicast(ifname) ? "MCAST" : "NO MCAST");
			printf("%s if %s\n", ifname, 
				if_is_portsel(ifname) ? "PORTSEL" : "NO PORTSEL");
			printf("%s if %s\n", ifname, 
				if_is_link_up(ifname) ? "LINK UP" : "LINK DOWN");
			break;

		default:
			break;
		}

	}
	else {

		switch (iftype) {
		
		/* set HWADDR */
		case NETDEV_HWADDR:
			strncpy(buf, ifarg, 20);
			memset(&addr, 0, sizeof(addr));
			addr.sa_family = ARPHRD_ETHER;
			memcpy(addr.sa_data, ether_aton(buf), 
				sizeof(struct ether_addr));
			if (if_set_hwaddr(ifname, &addr))
				printf("if_set_hwaddr error: %s\n", if_error());
			break;

		/* set MTU */
		case NETDEV_MTU:
			mtu = atoi(ifarg);
			if (if_set_mtu(ifname, mtu) < 0)
				printf("if_set_mtu error: %s\n", if_error());


		default:
			break;
		}	


	}

	return 0;
}


int 
_do_ethtool(void)
{
	char buf[32768] = {0};

	printf("link speed %d\n", eth_get_speed(ifname));
	printf("port media %d\n", eth_get_port(ifname));
	printf("autoneg %d\n", eth_is_autoneg(ifname));
	printf("eeprom size %d\n", eth_get_eeprom_size(ifname));

	if (eth_read_eeprom(ifname, buf, 0x2580 + 0x190, 16))
		return -1;

	printf("SN is %s\n", buf);

#if 0
	int fd;
	fd = open("eeprom", O_RDWR|O_CREAT, 0644);
	if (fd < 0)
		return -1;

	write(fd, buf, sizeof(buf));
	close(fd);
#endif

	return 0;
}


int 
main(int argc, char **argv)
{
	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	switch (iftype) {

	case NETDEV_ETHTOOL:
		_do_ethtool();
		break;
	default:
		_do_ioctl();
		break;
	
	}

	return 0;
}

