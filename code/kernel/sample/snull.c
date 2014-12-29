/*
 *   the net driver driver, it's a software ethernet device
 *
 *   write by Forrest.zhang
 */

#define _DEBUG_              1

#include <linux/module.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/if_arp.h>

#include "debug.h"

#define SNULL_MTU            (8*1024)
#define SNULL_QLEN           1024


struct snull_priv {
    struct net_device_stats stats;
    spinlock_t              lock;
};

static struct net_device    *snull_devs[2] = {NULL, NULL};

/*   
 *  open function called when interface is UP
 */
static int snull_open(struct net_device *dev)
{
    DBG("call %s\n", __FUNCTION__);

    memcpy(dev->dev_addr, "\0SNUL0", dev->addr_len);
    if (dev == snull_devs[1])
	dev->dev_addr[dev->addr_len - 1]++;

    netif_start_queue(dev);

    return 0;
}

/*
 *  stop function called when interface is DOWN
 */
static int snull_stop(struct net_device *dev)
{
    DBG("call %s\n", __FUNCTION__);

    netif_stop_queue(dev);

    return 0;
}

/*
 *  the set_mac_address is called when set interface hw address
 */
static int snull_set_mac_address(struct net_device *dev, void *addr)
{
    struct sockaddr *mac = addr;

    DBG("call %s\n", __FUNCTION__);

    memcpy(dev->dev_addr, mac->sa_data, dev->addr_len);

    return 0;
}

/*
 *  the send packet function
 */
static int snull_xmit(struct sk_buff *skb, struct net_device *dev)
{
    DBG("call %s\n", __FUNCTION__);

    DBG("send a packet, len is %d\n", skb->len);
    
    printf("packet header is %x:%x:%x:%x:%x:%x", skb->)

    return 0;
}

/*
 *   setup function is called in alloc_netdev
 */
static void snull_setup(struct net_device *dev)
{
    DBG("call %s\n", __FUNCTION__);

    dev->mtu             = SNULL_MTU;
    dev->type            = ARPHRD_ETHER;
    dev->hard_header_len = ETH_HLEN;
    dev->addr_len        = ETH_ALEN;
    dev->tx_queue_len    = SNULL_QLEN;
    dev->flags           = IFF_NOARP | IFF_BROADCAST | IFF_MULTICAST;
    memset(dev->broadcast, 0xFF, ETH_ALEN);

    /* functions */
    dev->open            = snull_open;
    dev->stop            = snull_stop;
    dev->set_mac_address = snull_set_mac_address;
    dev->hard_start_xmit = snull_xmit;
    /* set device mac address */
}

static int __init snull_dev_init(struct net_device **dev)
{
    DBG("call %s\n", __FUNCTION__);

    *dev = alloc_netdev(sizeof(struct snull_priv),
			"snull%d",
			snull_setup);
    if (!(*dev))
	return -1;

    if (register_netdev(*dev)) {
	ERR("%s call register_netdev error\n", __FUNCTION__);
	goto ERROR;
    }

    return 0;

  ERROR:
    free_netdev(*dev);
    dev = NULL;
    
    return -1;
}

static void snull_dev_free(struct net_device **dev)
{
    DBG("call %s\n", __FUNCTION__);
    
    unregister_netdev(*dev);
}

static int __init snull_init(void)
{
    DBG("call %s\n", __FUNCTION__);

    if (snull_dev_init(&snull_devs[0]))
	return -1;

    if (snull_dev_init(&snull_devs[1]))
	goto ERROR;

    return 0;

  ERROR:
    snull_dev_free(&snull_devs[0]);
    return -1;
}

static void __exit snull_exit(void)
{
    DBG("call %s\n", __FUNCTION__);

    snull_dev_free(&snull_devs[0]);
    snull_dev_free(&snull_devs[1]);
}


module_init(snull_init);
module_exit(snull_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Forrest.zhang");











