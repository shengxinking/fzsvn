/**
 *	@file	network.c
 *
 *	@brief	the network init function.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date
 */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "util.h"

/**
 *	Change the network interface name.
 *
 *	Return 0 if success, -1 on error.
 */
int 
nw_chname(void)
{
	return 0;
}


/**
 *	Set the kernel tcp parameter.
 *
 *	Return 0 if success, -1 on error.
 */
int 
nw_set_tcp(void)
{
	return 0;
}


/**
 *	Set the lo device IP.
 *
 *	Return 0 if success, -1 on error.
 */
int 
nw_set_lo(void)
{
	int ret;
	char cmd[] = "/bin/busybox ifconfig lo 127.0.0.1 netmask 255.255.255.0 up";

	ret = ut_runcmd(cmd);

	return ret;
}



#define  IP_FORWARD_FILE    "/proc/sys/net/ipv4/ip_forward"
/**
 *	Enable IP-forware function.
 *
 *	No return.
 */
int  
nw_ipforward(void)
{
        FILE         *fp;

        fp = fopen(IP_FORWARD_FILE, "w");
        if (!fp) {
                printf("open %s error\n", IP_FORWARD_FILE);
                return -1;
        }

        fputs("1", fp);

        fclose(fp);

	return 0;
}
