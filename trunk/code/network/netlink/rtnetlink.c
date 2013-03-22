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


int 
rtnl_request(struct rtnl_handle *rtx, struct nlmsghdr *nlmsg)
{
	struct sockaddr_nl nladdr;

        memset(&nladdr, 0, sizeof(nladdr));
        nladdr.nl_family = AF_NETLINK;

        return sendto(rth->fd, buf, len, 0,
                      (struct sockaddr*)&nladdr, sizeof(nladdr));
}

int
rtnl_wilddump_request(struct rtnl_ctx *rth, int family, int type)
{       
        struct {
                struct nlmsghdr nlh;
                struct rtgenmsg g;
        } req;
        struct sockaddr_nl nladdr;

        memset(&nladdr, 0, sizeof(nladdr));
        nladdr.nl_family = AF_NETLINK;

        req.nlh.nlmsg_len = sizeof(req);
        req.nlh.nlmsg_type = type;
        req.nlh.nlmsg_flags = NLM_F_ROOT|NLM_F_MATCH|NLM_F_REQUEST;
        req.nlh.nlmsg_pid = 0;
        req.nlh.nlmsg_seq = rth->dump = ++rth->seq;
        req.g.rtgen_family = family;

        return sendto(rth->fd, (void*)&req, sizeof(req), 0,
                      (struct sockaddr*)&nladdr, sizeof(nladdr));
}


int 
rtnl_dump_request(rtnl_ctx_t *rtx, int type, void *req, int len)
{
	struct nlmsghdr nlh;
	struct sockaddr_nl nladdr;
	struct iovec iov[2] = {{ &nlh, sizeof(nlh)}, {req, len}};
	struct msghdr msg = {
		(void*)&nladdr,
		sizeof(nladdr),
		iov,
		2,
		NULL,
		0,
		0
	};

	memset(&nladdr, 0, sizeof(nladdr));
	nladdr.nl_family = AF_NETLINK;

	nlh.nlmsg_len = NLMSG_LENGTH(len);
	nlh.nlmsg_type = type;
	nlh.nlmsg_flags = NLM_F_ROOT|NLM_F_MATCH|NLM_F_REQUEST;
	nlh.nlmsg_pid = 0;
	nlh.nlmsg_seq = rth->dump = ++rth->seq;

	return sendmsg(rth->fd, &msg, 0);
}

int 
rtnl_dump_filter(rtnl_ctx_t *rtx, rtnl_filter *f1, void *arg1)
{
	char buf[RTNL_DUMP_LEN];
	struct sockaddr_nl nl;
	struct iovec iov = {buf, sizeof(buf)};
	struct nlmsghdr *h;
	struct msghdr msg = {(void *)&nl, sizeof(nladdr), &iov, 1, NULL, 0, 0};
	int status;
	int err;
	struct nlmsgerr *errmsg;

	while (1) {

		status = recvmsg(rth->fd, &msg, 0);
		if (status < 0) {
			if (errno == EINTR)
				continue;
			continue;
		}
		else if (status == 0) {
			return -1;
		}
		if (msg.msg_namelen != sizeof(nladdr)) {
			return -1;
		}
		
		h = (struct nlmsghdr*)buf;
		while (NLMSG_OK(h, status)) {

			if (h->nlmsg_pid != rth->local.nl_pid ||
					h->nlmsg_seq != rth->dump) {
				goto skip_it;
			}

			if (h->nlmsg_type == NLMSG_DONE)
				return 0;

			if (h->nlmsg_type == NLMSG_ERROR) {
				*errmsg = (struct nlmsgerr*)NLMSG_DATA(h);
				if (h->nlmsg_len >= NLMSG_LENGTH(sizeof(struct nlmsgerr))) {
					errno = -errmsg->error;
				}
				return -1;
			}
			err = filter(&nladdr, h, arg1);
			if (err < 0)
				return err;

skip_it:
			h = NLMSG_NEXT(h, status);
		}
		if (msg.msg_flags & MSG_TRUNC) {
			continue;
		}
		if (status) {
			return -1;
		}
	}
}

int 
rtnl_add_attr(struct nlmsghdr *nl, size_t maxlen, int type, 
		void *data, size_t dlen)
{
	int len = RTA_LENGTH(datalen);
	struct rtattr *rta;

	/* check argument */
	if (!nl || !data)
		return -1;

	/* no enough room to add rtattr */
	if (NLMSG_ALIGN(nl->nlmsg_len) + dlen > maxlen)
		return -1;

	/* set rtattr value */
	rta = (struct rtattr*)(((char*)nl) + NLMSG_ALIGN(nl->nlmsg_len));
	rta->rta_type = type;
	rta->rta_len = dlen;

	/* copy data */
	if (dlen > 0)
		memcpy(RTA_DATA(rta), data, dlen);

	/* recalc nl's length */
	nl->nlmsg_len = NLMSG_ALIGN(nl->nlmsg_len) + len;

	return 0;
}

int 
rtnl_parse_attr(struct rtattr *tb[], int max, struct rtattr *rta, int len)
{
	while (RTA_OK(rta, len)) {
		if (rta->rta_type <= max) {
			tb[rta->rta_type] = rta;
		}
		rta = RTA_NEXT(rta, len);
	}

	return 0;
}


