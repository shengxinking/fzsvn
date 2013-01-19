/*
 *  the basic function to implement route netlink
 *
 *  write by Forrest.zhang
 */

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <libnetlink.h>

/*
 *  open a netlink socket. and set local address and peer 
 *  address to kernel
 *
 *  return 0 if OK, -1 on error.
 */
int rtnl_open(struct rtnl_handle* rth)
{
	int len;

	if (!rth)
		return -1;

	memset(rth, 0, sizeof(struct rtnl_handle));

	// create netlink socket
	rth->fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (rth->fd < 0) {
		perror("cannot open netlink socket\n");
		return -1;
	}

	// set local netlink address
	rth->local.nl_family = AF_NETLINK;
	rth->local.nl_pid = getpid();
	rth->local.nl_groups = 0;

	// bind the netlink socket
	if ( bind(rth->fd, (struct sockaddr*)&rth->local, sizeof(rth->local) ) ) {
		perror("cannot bind netlink socket\n");
		return -1;
	}

	// test bind success
	len = sizeof(rth->local);
	if (getsockname(rth->fd, (struct sockaddr*)&rth->local, &addr_len)) {
		perror("cannot get netlink socket local address\n");
		return -1;
	}

	if (len != sizeof(rth->local)) {
		fprintf(stderr, "wrong address lenght %d\n", addr_len);
		return -1;
	}
    
	// set peer netlink address(normally is kernel)
	rth->peer.nl_family = AF_NETLINK;
	rth->peer.nl_pid = 0;
	rth->peer.nl_groups = 0;

	rth->seq = time(NULL);
    
	return 0;
}


/*
 *  send a message to kernel netlink
 *
 *  return 0 if OK, -1 on error
 */
int rtnl_send(struct rtnl_handle *rth, const struct nlmsghdr *nlmsg)
{
//    struct msghdr          msg;
    
    return 0;
}

/*
 *  receive a message from kernel netlink and store in buf
 *
 *  return the received size if OK, -1 on error
 */
int rtnl_recv(struct rtnl_handle *rth, struct nlmsghdr *nlmsg, size_t size)
{
    return 0;
}

/*
 *  add a rtattr message into nlmsghdr if have enought room 
 *
 *  return 0 if OK, -1 on error
 */
int add_rtattr(struct nlmsghdr *nl, 
	       size_t maxlen, 
	       int type, 
	       void *data, 
	       size_t datalen)
{
    int           len = RTA_LENGTH(datalen);
    struct rtattr *rta;

    // no enough room to add rtarrt
    if (NLMSG_ALIGN(nl->nlmsg_len) + len > maxlen)
	return -1;

    // set rtattr value
    rta = (struct rtattr*)(((char*)nl) + NLMSG_ALIGN(nl->nlmsg_len));
    rta->rta_type = type;
    rta->rta_len = len;
    
    // here is for some attr have no data
    if (datalen > 0)
	memcpy(RTA_DATA(rta), data, datalen);

    // recalc nl's length
    nl->nlmsg_len = NLMSG_ALIGN(nl->nlmsg_len) + len;

    return 0;
}

__u32 ip_u32 (const char* ip)
{
    __u32         addr;

    assert(ip);
    
    if(inet_pton(AF_INET, ip, &addr) < 0)
	return 0;

    return addr;
}
