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

#define	RTNL_DEBUG

/* define debug macro */
#ifdef	RTNL_DEBUG
#define	RTNL_DBG(f, a...)	printf("[rtnl-dbg]: "fmt, ##args)
#else
#define	RTNL_DBG(f, a...)
#endif
#define	RTNL_ERR(f, a...)	printf("[rtnl-err]: %s %d: "f, ##a)


int 
rtnl_open(struct rtnl_ctx* rth)
{
	int len;
	struct sockaddr_nl nladdr;

	if (!rth)
		return -1;

	memset(rth, 0, sizeof(struct rtnl_ctx));

	/* create netlink socket */
	rth->fd = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
	if (rth->fd < 0) {
		perror("cannot open netlink socket\n");
		return -1;
	}

	/* set local netlink address */
	rth->local.nl_family = AF_NETLINK;
	rth->local.nl_pid = getpid();
	rth->local.nl_groups = 0;

	/* bind the netlink socket */
	len = sizeof(rth->local);
	if (bind(rth->fd, (struct sockaddr*)&rth->local, len) ) {
		RTNL_ERR("cannot bind netlink socket\n");
		return -1;
	}

	if (getsockname(rth->fd, (struct sockaddr*)&nladdr, &len)) {
		RTNL_ERR("can't get netlink socket address\n");
		return -1;
	}

	if (len != sizeof(rth->local)) {
		RTNL_ERR("wrong address length %d\n", len);
		return -1;
	}
    
	/* set peer netlink address(normally is kernel) */
	rth->peer.nl_family = AF_NETLINK;
	rth->peer.nl_pid = 0;
	rth->peer.nl_groups = 0;
	rth->seq = time(NULL);
    
	return 0;
}

int 
rtnl_send(rtnl_ctx_t *rth, const struct nlmsghdr *nlmsg)
{
	int n;

	if (!rth || !nlmsg)
		return -1;

	/* sendto peer */
	n = sendto(rth->fd, (struct sockaddr *)&rth->peer, 
		   nlmsg, nlmsg->nlmsg_len, 0);
	if (n < 0)
		return -1;
	if (n != nlmsg->nlmsg_len)
		return -1;

	return 0;
}

int 
rtnl_recv(struct rtnl_ctx *rth, struct nlmsghdr *nlmsg, size_t size)
{
	int n;
	int total = 0;
	char *buf;

	if (!rth || !nlmsg || !size)
		return -1;

	buf = (char *)nlmsg;

	do {
		n = recv(rth->fd, buf, size, 0);
		if (n < 0) {
			if (errno == EAGAIN || errno == EINTR)
				return 0;
			return -1;
		if (n == 0)
			return 0;

		total += n;
		if (NLMSG_OK(nlmsg, total)) {
			return 0;
		}
	}

	return 0;
}

/*
 *  add a rtattr message into nlmsghdr if have enought room 
 *
 *  return 0 if OK, -1 on error
 */
int 
rtnl_add_rtattr(struct nlmsghdr *nl, size_t maxlen, int type, 
		void *data, size_t datalen)
{
	int len = RTA_LENGTH(datalen);
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


