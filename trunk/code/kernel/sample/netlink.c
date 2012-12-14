/*
 *  a simple netlink kernel module in 2.6
 *
 *  write by Forrest.zhang
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/netlink.h>
#include <net/sock.h>

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Forrest.zhang");

#define NETLINK_TEST          18
#define _DEBUG_               1

#define ERR(fmt, arg...)      printk(KERN_ERR "netlink_test: " fmt, ##arg)

#if defined(_DEBUG_)
#   define DBG(fmt, arg...)   printk(KERN_DEBUG "netlink_test: " fmt, ##arg)
#else
#   define DBG(fmt, arg...)
#endif

static struct sock           *test_nl = NULL;

static void test_output(
    struct sock *sk,
    char        *buf,
    int         size)
{

}

static void test_input(
    struct sock *sk,
    int         len)
{
    struct sk_buff  *skb;
    struct nlmsghdr *nlh = NULL;
    u8              *payload = NULL;

    while ( (skb = skb_dequeue(&sk->sk_receive_queue)) != NULL) {
	nlh = (struct nlmsghdr *)skb->data;
	payload = NLMSG_DATA(nlh);
	/* process netlink message with header pointed by nlh and payload */
	DBG("receive a message: %s\n", payload);

	kfree_skb(skb);
    }
}


static int __init test_init(void)
{
    test_nl = netlink_kernel_create(NETLINK_TEST, 0, test_input, THIS_MODULE);
    if (!test_nl) {
	ERR("create netlink sock error\n");
	return -1;
    }

    DBG("init success\n");

    return 0;
}

static void __exit test_exit(void)
{
    /* release sock struct */
    sock_release(test_nl->sk_socket);
    
    DBG("release success\n");
}

module_init(test_init);
module_exit(test_exit);
