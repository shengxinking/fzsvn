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

#define	_RTNL_DEBUG

/* define debug macro */
#ifdef	_RTNL_DEBUG
#define	_RTNL_DBG(f, a...)	printf("[rtnl-dbg]: "fmt, ##args)
#else
#define	_RTNL_DBG(f, a...)
#endif
#define	_RTNL_ERR(f, a...)	printf("[rtnl-err]: %s %d: "f, ##a)

/**
 *	Send a nlmsghdr to kernel 
 **/
static int 
_rtnl_send(rtnl_ctx_t *rtx, struct nlmsghdr *msg)
{
	int n;

	if (!rtx || !nlmsg)
		return -1;

	/* send blmmsghdr to peer */
	n = sendto(rtx->fd, msg, msg->nlmsg_len, 
		   (struct sockaddr *)&rtx->peer, sizeof(rtx->peer));
	if (n < 0)
		return -1;
	if (n != len)
		return -1;

	return 0;
}

static int  
rtnl_recv(struct rtnl_ctx *rtx, char *buf, size_t len)
{
	int n;
	struct nlmsghdr *nlh;

	if (!rtx || !nlmsg || !size)
		return -1;

	/* recv data */
	n = recv(rtx->fd, buf, len, 0);
	if (n <= 0) {
		if (errno != EINTR) {
			_RTNL_ERR("recv failed: %s\n", strerror(errno));
			return -1;
		}
		return 0;
	}

	nlh = (struct nlmsghdr *)buf;
	if (!NLMSG_OK(nlm, n)) {
		_RTNL_ERR("recved message is trunked\n");
		return 0;
	}

	return n;
}


int 
rtnl_open(struct rtnl_ctx* rtx)
{
	int len;
	struct sockaddr_nl nladdr;

	if (!rtx)
		return -1;

	memset(rtx, 0, sizeof(rtnl_ctx_t));

	/* create netlink socket */
	rtx->fd = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
	if (rtx->fd < 0) {
		_RTNL_ERR("create netlink socket error\n");
		return -1;
	}

	/* set local netlink address */
	rtx->local.nl_family = AF_NETLINK;
	rtx->local.nl_pid = getpid();
	rtx->local.nl_groups = 0;

	/* bind the netlink socket */
	len = sizeof(rtx->local);
	if (bind(rtx->fd, (struct sockaddr*)&rtx->local, len) ) {
		_RTNL_ERR("cannot bind netlink socket\n");
		return -1;
	}

	/* check bind success */
	if (getsockname(rtx->fd, (struct sockaddr*)&nladdr, &len)) {
		_RTNL_ERR("can't get netlink socket address\n");
		return -1;
	}
	if (len != sizeof(rtx->local)) {
		_RTNL_ERR("wrong address length %d\n", len);
		return -1;
	}
    
	/* set peer netlink address(normally is kernel) */
	rtx->peer.nl_family = AF_NETLINK;
	rtx->seq = time(NULL);
    
	return 0;
}


int 
rtnl_send_request(struct rtnl_handle *rtx, struct nlmsghdr *nlh)
{
	char buf[RTNL_REQ_LEN];
	struct nlmsghdr *nlh;
	int n;

	if (!rtx || !nlh)
		return -1;

	/* send message */
        if (rtnl_send(rtx, nlh)) {
		return -1;
	}

	/* recved message */
	n = rtnl_recv(rtx, buf, sizeof(buf))
	if (n <= 0)
		return -1;

	/* check message */
	nlh = (struct nlmsghdr *)buf;
	if (nlh->nlmsg_type == NLMSG_ERROR)
		return -1;
	
	return 0;
}

int 
rtnl_dump_request(struct rtnl_ctx *rtx, int family, int type)
{       
        struct {
                struct nlmsghdr nlh;
                struct rtgenmsg g;
        } req;
	size_t len;
	int n;

	len = sizeof(req);
	memset(&req, 0, len);

	/* set nlmsg */
        req.nlh.nlmsg_len = sizeof(req);
        req.nlh.nlmsg_type = type;
        req.nlh.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
        req.nlh.nlmsg_pid = 0;
        req.nlh.nlmsg_seq = rtx->dump_seq = ++rtx->seq;

	/* set rtgenmsg */
        req.g.rtgen_family = family;

	return rtnl_send(rtx, &req, len);
}

int 
rtnl_dump_filter(rtnl_ctx_t *rtx, rtnl_filter *f1, void *arg1)
{
	char buf[RTNL_DUMP_LEN];
	struct nlmsghdr *nlh;
	int n;
	int err;
	struct nlmsgerr *errmsg;

	while (1) {
		/* recv message */
		n = rtnl_recv(rtx, buf, sizeof(buf));
		if (n < 0) {
			if (errno == EINTR)
				continue;
			break;
		}

		/* check message */
		nlh = (struct nlmsghdr *)buf;
		while (NLMSG_OK(h, n)) {

			/* last message */
			if (nlh->nlmsg_type == NLMSG_DONE)
				return 0;

			/* error message */
			if (nlh->nlmsg_type == NLMSG_ERROR) {
				*errmsg = (struct nlmsgerr*)NLMSG_DATA(nlh);
				if (nlh->nlmsg_len >= NLMSG_LENGTH(sizeof(struct nlmsgerr))) {
					errno = -errmsg->error;
				}
				return -1;
			}

			/* call filter function */
			err = filter(nlh, arg1);
			if (err < 0)
				return err;

			h = NLMSG_NEXT(h, status);
		}
	}

	return 0;
}

int 
rtnl_add_attr(struct nlmsghdr *nl, size_t maxlen, int type, 
		void *data, size_t dlen)
{
	int len = RTA_LENGTH(dlen);
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

#if 0
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
	nlh.nlmsg_seq = rtx->dump = ++rtx->seq;

	return sendmsg(rtx->fd, &msg, 0);
}
#endif



