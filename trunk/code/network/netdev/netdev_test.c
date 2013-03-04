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
#include <stdio.h>
#include <stdlib.h>
#include <netinet/ether.h>

#include "netdev.h"

int main(int argc, char **argv)
{
    char               ifname[10] = { 0 };
    char               newname[10] = {0};
    int                index = 0;
    struct sockaddr    addr;
    struct sockaddr_in *addr_in = NULL;
    char               buf[1024] = {0};
    int                mtu = 0;
    struct ifconf      ifc;
    int                ret = 0;
    char               address[20] = {0};
    int                i;

    if (argc != 3)
	return -1;

    /* get index */
    strncpy(ifname, argv[1], 10);
    index = if_get_index(ifname);
    if (index < 0)
	printf("if_get_index error: %s\n", if_error());
    printf("%s index is %d\n", ifname, if_get_index(ifname));

#if 0

    /* get name */
    printf("%s name is %s\n", ifname, if_get_name(if_get_index(ifname), buf, 512));
    
    /* set name */
    strncpy(newname, argv[2], 10);
    if (if_set_name(ifname, newname))
	printf("if_set_name error: %s\n", if_error());

    printf("%s new name is %s\n", ifname, if_get_name(index, buf, 512));


    /* get HWADDR */
    if (!if_get_hwaddr(ifname, &addr))
	printf("if_get_hwaddr error: %s\n", if_error());
    printf("%s hwaddr is %s\n", ifname, ether_ntoa((const struct ether_addr*)addr.sa_data));

    /* set HWADDR */
    strncpy(buf, argv[2], 20);
    memset(&addr, 0, sizeof(addr));
    addr.sa_family = ARPHRD_ETHER;
    memcpy(addr.sa_data, ether_aton(buf), sizeof(struct ether_addr));
    if (if_set_hwaddr(ifname, &addr))
	printf("if_set_hwaddr error: %s\n", if_error());

    if (!if_get_hwaddr(ifname, &addr))
	printf("if_get_hwaddr error: %s\n", if_error());
    printf("%s hwaddr is %s\n", ifname, ether_ntoa((const struct ether_addr*)addr.sa_data));

    /* get MTU */
    printf("%s mtu is %d\n", ifname, if_get_mtu(ifname));

    /* set MTU */
    mtu = atoi(argv[2]);
    if (if_set_mtu(ifname, mtu) < 0)
	printf("if_set_mtu error: %s\n", if_error());
    printf("%s new mtu is %d\n", ifname, if_get_mtu(ifname));

    /* get INET address */
    if (!if_get_addr(ifname, &addr))
	printf("if_get_addr error: %s\n", if_error());
    addr_in = (struct sockaddr_in *)&addr;
    printf("%s address is %s\n", ifname, 
	   inet_ntop(AF_INET, &addr_in->sin_addr, buf, 20));

    /* set INET address */
    strncpy(buf, argv[2], 20);
    memset(&addr, 0, sizeof(addr));
    addr_in = (struct sockaddr_in *)&addr;
    addr_in->sin_family = AF_INET;
    if (inet_pton(AF_INET, buf, &addr_in->sin_addr) <= 0) {
	printf("invalid INET address: %s\n", argv[2]);
	return -1;
    }
    if (if_set_addr(ifname, &addr))
	printf("if_set_addr error: %s\n", if_error());

    if (!if_get_addr(ifname, &addr))
	printf("if_get_addr error: %s\n", if_error());
    addr_in = (struct sockaddr_in *)&addr;
    printf("%s new address is %s\n", ifname, 
	   inet_ntop(AF_INET, &addr_in->sin_addr, buf, 20));


    /* get INET netmask address */
    if (!if_get_netmask(ifname, &addr))
	printf("if_get_netmask error: %s\n", if_error());
    addr_in = (struct sockaddr_in *)&addr;
    printf("%s netmask is %s\n", ifname, 
	   inet_ntop(AF_INET, &addr_in->sin_addr, buf, 20));

    /* set INET netmask address */
    strncpy(buf, argv[2], 20);
    memset(&addr, 0, sizeof(addr));
    addr_in = (struct sockaddr_in *)&addr;
    addr_in->sin_family = AF_INET;
    if (inet_pton(AF_INET, buf, &addr_in->sin_addr) <= 0) {
	printf("invalid INET address: %s\n", argv[2]);
	return -1;
    }
    if (if_set_netmask(ifname, &addr))
	printf("if_set_netmask error: %s\n", if_error());

    if (!if_get_netmask(ifname, &addr))
	printf("if_get_netmask error: %s\n", if_error());
    addr_in = (struct sockaddr_in *)&addr;
    printf("%s new netmask is %s\n", ifname, 
	   inet_ntop(AF_INET, &addr_in->sin_addr, buf, 20));


/* get INET broadcast address */
    if (!if_get_brdaddr(ifname, &addr))
	printf("if_get_brdaddr error: %s\n", if_error());
    addr_in = (struct sockaddr_in *)&addr;
    printf("%s broadcast address is %s\n", ifname, 
	   inet_ntop(AF_INET, &addr_in->sin_addr, buf, 20));

    /* set INET broadcast address */
    strncpy(buf, argv[2], 20);
    memset(&addr, 0, sizeof(addr));
    addr_in = (struct sockaddr_in *)&addr;
    addr_in->sin_family = AF_INET;
    if (inet_pton(AF_INET, buf, &addr_in->sin_addr) <= 0) {
	printf("invalid INET address: %s\n", argv[2]);
	return -1;
    }
    if (if_set_brdaddr(ifname, &addr))
	printf("if_set_brdaddr error: %s\n", if_error());

    if (!if_get_brdaddr(ifname, &addr))
	printf("if_get_brdaddr error: %s\n", if_error());
    addr_in = (struct sockaddr_in *)&addr;
    printf("%s new broadcast address is %s\n", ifname, 
	   inet_ntop(AF_INET, &addr_in->sin_addr, buf, 20));

#endif

    /* test IFF_FLAGS */
    printf("%s is %s\n", ifname, if_is_up(ifname) ? "UP" : "DOWN");
    printf("%s is %s\n", ifname, if_is_broadcast(ifname) ? "BROADCAST" : "UNICAST");
    printf("%s is %s\n", ifname, if_is_promisc(ifname) ? "PROMISC" : "NO PROMISC");
    printf("%s is %s\n", ifname, if_is_noarp(ifname) ? "NOARP" : "ARP");
    printf("%s is %s\n", ifname, if_is_dynamic(ifname) ? "DYNAMIC" : "NO DYNAMIC");

    printf("%s is %s\n", ifname, if_is_loopback(ifname) ? "LOOPBACK" : "NO LOOPBACK");
    printf("%s is %s\n", ifname, if_is_running(ifname) ? "RUNNING" : "NO RUNNING");
    printf("%s is %s\n", ifname, if_is_ppp(ifname) ? "PPP device" : "no PPP device");
    printf("%s is %s\n", ifname, if_is_notrailers(ifname) ? "NOTRAILERS" : "TRAILERS");
    printf("%s if %s\n", ifname, if_is_debug(ifname) ? "DEBUG" : "NO DEBUG");
    printf("%s if %s\n", ifname, if_is_master(ifname) ? "MASTER" : "NO MASTER");
    printf("%s if %s\n", ifname, if_is_slave(ifname) ? "SLAVE" : "NO SLAVE");
    printf("%s if %s\n", ifname, if_is_multicast(ifname) ? "MULTICAST" : "NO MULTICAST");
    printf("%s if %s\n", ifname, if_is_portsel(ifname) ? "PORTSEL" : "NO PORTSEL");
    printf("%s if %s\n", ifname, if_is_automedia(ifname) ? "AUTOMEDIA" : "NO AUTOMEDIA");
    printf("%s if %s\n", ifname, if_is_allmulti(ifname) ? "ALLMULTI" : "NO ALLMULTI");
    printf("%s if %s\n", ifname, if_is_link_up(ifname) ? "LINK UP" : "LINK DOWN");

    /* get all address */
    memset(buf, 0, sizeof(buf));
    ifc.ifc_buf = buf;
    ifc.ifc_len = 40;
    if ( (ret = if_get_alladdr(ifname, &ifc)) < 0)
	printf("if_get_alladdr error: %s\n", if_error());

    for (i = 0; i < ret; i++) {
	addr_in = (struct sockaddr_in*)&(ifc.ifc_req[i].ifr_addr);
	printf("%s address is %s\n", 
	       ifc.ifc_req[i].ifr_name, inet_ntop(AF_INET, &(addr_in->sin_addr), address, 20));
    }
/*
    if (if_nodynamic(ifname))
	printf("if_noarp error: %s\n", if_error());
*/
    return 0;
}

