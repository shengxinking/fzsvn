/*
 *  send a messsage to kernel and received from kernel
 *
 *  write by Forrest.zhang
 */

#include <sys/socket.h>
#include <linux/netlink.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#define NETLINK_TEST     18
#define MAX_PAYLOAD      1024           /* maximun payload size */

static void usage(void)
{
    printf("test_unicast <string>\n");
}

int main(int argc, char **argv)
{
    struct sockaddr_nl        src_addr;
    struct sockaddr_nl        dest_addr;
    struct iovec              iov;
    struct nlmsghdr           *nlh = NULL;
    struct msghdr             msg;
    int                       fd;

    if (argc != 2) {
	usage();
	return -1;
    }

    /* create socket */
    fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_TEST);
    if (fd < 0) {
	printf("create NETLINK_TEST socket error: %s", strerror(errno));
	return -1;
    }

    memset(&src_addr, 0, sizeof(struct sockaddr_nl));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid();
    src_addr.nl_groups = 0;

    bind(fd, (struct sockaddr*)&src_addr, sizeof(src_addr));

    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0;
    dest_addr.nl_groups = 0;
    
    nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
    nlh->nlmsg_pid = getpid();
    nlh->nlmsg_flags = 0;

    strcpy(NLMSG_DATA(nlh), argv[1]);

    iov.iov_base = (void *)nlh;
    iov.iov_len = nlh->nlmsg_len;

    msg.msg_name = (void *)&dest_addr;
    msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    
    sendmsg(fd, &msg, 0);

    memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));

//    recvmsg(fd, &msg, 0);
//    printf("Received message payload: %s\n", (char*)NLMSG_DATA(nlh));

    free(nlh);
    close(fd);

    return 0;
}

