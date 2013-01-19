/*
 *  show kernel route table
 *
 *  write by Forrest.zhang
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>

int show_route(int argc, char* argv[])
{
    int                     fd;
    struct sockaddr_nl      local, kernel;
    struct msghdr           msg;

    // create netlink socket
    fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (fd < 0) {
	printf("create netlink socket error: %s\n", strerror(errno));
	return -1;
    }

    // set local netlink address
    memset(&local, 0, sizeof(local));
    local.nl_family = AF_NETLINK;
    local.nl_pid = getpid();
    local.nl_groups = 0;

    // set kernel netlink address
    memset(&kernel, 0, sizeof(kernel));
    kernel.nl_family = AF_NETLINK;
    kernel.nl_pid = 0;
    kernel.nl_groups = 0;
    
    if (bind(fd, (struct sockaddr*)&local, sizeof(local))) {
	printf("bind netlink socket to local netlink address error: %s\n", strerror(errno));
	return -1;
    }
    
    msg.msg_name = (void*)&kernel;
    msg.msg_namelen = sizeof(kernel);
    
    return 0;
}
