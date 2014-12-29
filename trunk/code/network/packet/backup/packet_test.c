/**
 *	@file	packet_test.c
 *
 *	@brief	Packet test program.
 *	
 *	@author	
 *
 *	@data	
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/if_ether.h>

#include "netpkt.h"
#include "packet_eth.h"
#include "packet_arp.h"
#include "packet_ipv4.h"
#include "packet_ipv6.h"
#include "packet_udp.h"
#include "packet_tcp.h"


int main(void)
{
    u_int8_t          buf[100];
    u_int8_t          pkt[100];
    int               fd1, fd2;
    int               ifindex;
    int               buf_len;
    int               pkt_len;
    u_int32_t         sip, dip;

    sip = packet_aton("192.168.1.5");
    dip = packet_aton("192.168.1.1");

    ifindex = if_get_index("eth0");
    if (ifindex < 0) {
	printf("%s\n", if_error());
	return -1;
    }

#if 0

    fd1 = packet_socket(SOCK_DGRAM, ETH_P_ARP);
    if (fd1 < 0) {
	printf("%s\n", packet_error());
	return -1;
    }

    fd2 = packet_socket(SOCK_RAW, ETH_P_ARP);
    if (fd2 < 0) {
	printf("%s\n", packet_error());
	return -1;
    }

    /* send ARP request using SOCK_RAW and SOCK_DGRAM */
    buf_len = packet_arp(buf, 100, "00:0b:6a:ce:a7:b4", 
			 sip, NULL, dip, ARPOP_REQUEST);
    if (buf_len < 0) {
	printf("%s\n", packet_error());
	return -1;
    }

    pkt_len = packet_ether(buf, buf_len, pkt, 100, "00:0b:6a:ce:a7:b4",
			   "ff:ff:ff:ff:ff:ff", ETH_P_ARP);
    if (pkt_len < 0) {
	printf("%s\n", packet_error());
	return -1;
    }
    
    if (packet_send_raw(fd2, pkt, 60, ifindex, "ff:ff:ff:ff:ff:ff")) {
	printf("%s\n", packet_error());
	return -1;
    }


   if (packet_send_datagram(fd1, buf, 46, ifindex, "ff:ff:ff:ff:ff:ff", ETH_P_ARP)) {
	printf("%s\n", packet_error());
	return -1;
    }

#endif

   fd1 = packet_socket(SOCK_DGRAM, ETH_P_IP);
    if (fd1 < 0) {
	printf("%s\n", packet_error());
	return -1;
    }

    fd2 = packet_socket(SOCK_RAW, ETH_P_IP);
    if (fd2 < 0) {
	printf("%s\n", packet_error());
	return -1;
    }

    strncpy((char*)buf, "Hello for packet from packet library", 24);
   
    pkt_len = packet_udp(buf, 48, pkt, 100, 1000, 1000,sip, dip, IPPROTO_UDP);
    buf_len = packet_ip(pkt, pkt_len, buf, 100, sip, dip, IPPROTO_UDP);

    if (packet_send_datagram(fd1, buf, buf_len, ifindex, "00:11:95:fe:c0:75", ETH_P_IP)) {
	printf("%s\n", packet_error());
	return -1;
    }

    close(fd1);
    close(fd2);

    return 0;
}

