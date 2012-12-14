/**
 *	@file	netlink.h
 *
 *	@brief	netlink message.
 *	
 *	@author	Forrest.zhang
 *
 *	@date	2012-09-18
 */


#include <linux/string.h>
#include <linux/module.h>
#include <linux/types.h>

struct sock *nl_sk = NULL;

static void 
nl_data_ready(struct sock *sk, int len)
{
	wake_up_interruptible(sk->sk_sleep);
}

int 
netlink_alloc(void)
{
	struct sk_buff *skb = NULL;
	struct nlmsghdr *nlh = NULL;
	int err;
	u32 pid;

	nl_sk = netlink_kernel_create(28, 0, nl_data_ready, THIS_MODULE);
	if (!nl_sk) {
		printk("netlink_kernel_create failed\n");
		return -1;
	}

	skb = skb_recv_datagram(nl_sk, 0, 0, &err);
	nlh = (struct nlmsghdr *)skb->data;
	printk("%s: received netlink message payload: %s", 
	       __FUNCTION__, NLMSG_DATA(nlh));
	
	pid = nlh->nlmsg_pid;
	
	nlh->nlmsg_len = skb->len;
	NETLINK_CB(skb).pid = 0;
	NETLINK_CB(skb).dst_pid = pid;
	NETLINK_CB(skb).dst_group = 1;
	netlink_broadcast(nl_sk, skb, 0, 1, GFP_KERNEL);

	return 0;
}

static int __init
_module_init(void)
{
	printk("init netlink socket\n");
	if (netlink_test()) {
		printk("netlink test failed\n");
	}
	
	return 0;
}

static void __exit 
_module_exit(void)
{
	sock_release(nl_sk->sk_socket);
	printf("free netlink socket\n");
}

module_init(_module_init);

module_exit(_module_exit);


