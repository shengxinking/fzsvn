/*
 *  libarp is using to implement ARP protocol
 *  
 *  write by Forrest.zhang in Fortinet Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "netdevice.h"
#include "packet.h"

static void _usage(void)
{
    printf("packet_arp <device> <source ip> <source mac> <dest ip>"
	   " [times]\n");
}

int main(int argc, char **argv)
{
    int                 fd = 0;
    u_int8_t            buf[100] = {0};
    int                 buf_len = 0;
    int                 sip = 0;
    int                 dip = 0;
    int                 ifindex = 0;
    int                 times = 1;
    int                 i = 0;

    /* parse command line parameter */
    if (argc != 5 && argc != 6) {
	_usage();
	return -1;
    }
    
    ifindex = if_get_index(argv[1]);
    if (ifindex < 0) {
	_usage();
	return -1;
    }

    sip = packet_aton(argv[2]);
    dip = packet_aton(argv[4]);    
    if (sip == -1 || dip == -1) {
	_usage();
	return -1;
    }

    if (argc == 6)
	times = atoi(argv[5]);

    fd = packet_socket(SOCK_DGRAM, ETH_P_ARP);
    if (fd < 0) {
	printf("create SOCK_DGRAM error: %s\n", packet_error());
	return -1;
    }

    buf_len = packet_arp(buf, 100, argv[3], sip, NULL, dip, ARPOP_REQUEST);
    if (buf_len == -1) {
	printf("create arp packet error: %s\n", packet_error());
	return -1;
    }

    for (i = 0; i < times; i++) {
	if (packet_send_datagram(fd, buf, 46, ifindex, "ff:ff:ff:ff:ff:ff", ETH_P_ARP)) {
	    printf("send packet error: %s\n", packet_error());
	}
	sleep(1);
    }

    return 0;
}
