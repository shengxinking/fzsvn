/**
 *	@file	tcp_nat.c
 *
 *	@brief	do tcp dnat+snat, it conversion incoming packet from 
 *		sip:sport->dip:sport to sip:sport->new_dip:new_dport,
 *		and convert outgoing packet in this TCP session from
 *		new_dip:new_dport->sip:sport to dip:dport->sip:sport.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2012-09-17
 */

#include "tcp_nat.h"

#include <linux/netlink.h>
#include <linux/net.h>
#include <linux/skbuff.h>
#include <net/sock.h>
#include <linux/socket.h>
#include <net/net_namespace.h>
#include <linux/module.h>

static struct sock	*tcp_nat_nlsk;
static tcp_nat_ip_t	tcp_nat_ip;
static int		tcp_nat_ip_num;
static tcp_nat_port_t	tcp_nat_port;


static int 
tcp_nat_add_ip(u32 ip)
{

	return 0;
}

static int 
tcp_nat_del_ip(u32 ip)
{

	return 0;
}

static int 
tcp_nat_get_ip(void)
{
	return 0;
}

static int 
tcp_nat_clear_ip(void)
{
	return 0;
}

static int 
tcp_nat_set_port(u16 port, int type)
{
	return 0;
}


static void 
tcp_nat_recv(struct sk_buff *skb)
{
	struct nlmsghdr *nlh;
	u32 ip;
	u16 port;

	nlh = nlmsg_hdr(skb);
	if (!nlh) {
		printk("invalid netlink message\n");
		return;
	}

	if (skb->len < NLMSG_SPACE(0) || skb->len < nlh->nlmsg_len) {
		return;
	}

	switch (nlh->nlmsg_type) {

	case TCP_NAT_SET_HTTP:
	case TCP_NAT_SET_HTTPS:
	case TCP_NAT_SET_TELNET:
	case TCP_NAT_SET_SSH:

		printk("netlink command %d\n", nlh->nlmsg_type);
		//write_lock_bh(&tcp_nat_lock);
		
		//write_unlock_bh(&tcp_nat_lock);
		break;

	case TCP_NAT_ADD_IP:
		printk("netlink add ip\n");
		break;

	case TCP_NAT_DEL_IP:
		printk("netlink del ip\n");
		break;

	case TCP_NAT_GET_IP:
		printk("netlink get ip\n");
		break;

	case TCP_NAT_CLEAR_IP:
		printk("netlink clear ip\n");
		break;

	default:
		printk("unknowed nlmsg type %d\n", nlh->nlmsg_type);
		break;
	}

	return;
}

static int __init 
_module_init(void)
{
	/* create netlink for recv user space changes */
	tcp_nat_nlsk = netlink_kernel_create(&init_net, TCP_NAT_NETLINK, 0, 
					     tcp_nat_recv, NULL, THIS_MODULE);
	if (!tcp_nat_nlsk) {
		printk("create netlink %d failed\n", TCP_NAT_NETLINK);
		return -1;
	}

	printk("create netlink %d success\n", TCP_NAT_NETLINK);	
	/* add netfilter hook */
	
	return 0;
}

static int __exit 
_module_exit(void)
{
	if (tcp_nat_nlsk) {
		netlink_kernel_release(tcp_nat_nlsk);
		tcp_nat_nlsk = NULL;
	}

	return -1;
}

