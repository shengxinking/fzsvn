/**
 *	@file	tproxy.c
 *
 *	@brief	transparent proxy module
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2013-05-30
 */
#include <stddef.h>
#include <net/tcp.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/ip.h>
#include <linux/if_ether.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/socket.h>
#include <linux/if_bridge.h>
#include <linux/netfilter_bridge.h>
#include <linux/spinlock.h>
#include <linux/vmalloc.h>
#include <linux/inetdevice.h>

#include "tproxy.h"

static u32 
tp_pre_routing(u32 hooknum, struct sk_buff *skb, 
	       const struct net_device *in, 
	       const struct net_device *out, 
	       int (*okfn)(struct sk_buff *));

static u32 
tp_post_routing(u32 hooknum, struct sk_buff *skb, 
		const struct net_device *in, 
		const struct net_device *out, 
		int (*okfn)(struct sk_buff *));

enum {
	TP_CLIENT2PROXY,		/* client to proxy */
	TP_PROXY2CLIENT,		/* proxy to client */
	TP_PROXY2SERVER,		/* proxy to server */
	TP_SERVER2PROXY,		/* server to proxy */
	TP_UNKNOW,			/* not tp proxy traffic */
};

/**
 *	The nat information.
 */
typedef struct tp_tup {
	void		*sk_addr;	/* sock address */
	u32		ori_sip;	/* origin source ip */
	u32		ori_dip;	/* origin dest ip */
	u16		ori_sport;	/* origin source ip */
	u16		ori_dport;	/* origin dest port */
	u32		nat_sip;	/* NAT source ip */
	u32		nat_dip;	/* NAT dest ip */
	u32		nat_sport;	/* NAT source port */
	u32		nat_dport;	/* NAT dest port */
} tp_tup_t;

typedef struct tp_nat {
	char		name[TP_MAX_NAME];/* policy name */
	char		ifname[IFNAMSIZ];/* interface name */
	u32		vip;		/* policy ip */
	u32		pip;		/* pserver ip */
	u16		port;		/* pserver origin port */
	u16		nat_port;	/* pserver nat port */
} tp_nat_t;

static struct nf_hook_ops tp_post_routing_ops = {
	{NULL,NULL},		     
	tp_post_routing, 
	NULL, 
	PF_INET, 
	NF_INET_POST_ROUTING, 
	132
};

static struct nf_hook_ops tp_pre_routing_ops = {
	{NULL, NULL},
	tp_pre_routing,
	NULL,
	PF_BRIDGE, 
	NF_BR_PRE_ROUTING, 
	7
};

tp_policy_t	*tp_policy_list;
spinlock_t	tp_policy_lock;
int		tp_policy_count;
int		tp_policy_max;
int		tp_hash_size;

int		tp_pre_routing_hash;
int		tp_post_routing_hash;

static int 
tp_get_nat(tp_nat_t *nat, u32 sip, u32 dip, u16 sport, u16 dport, const char *indev)
{
	int i;
	int j;
	tp_policy_t *tp;

	if (unlikely(!nat))
		return TP_UNKNOW;

	spin_lock(&tp_policy_lock);

	for (i = 0; i < tp_policy_max; i++) {

		tp = &tp_policy_list[i];

		/* empty policy */
		if (strlen(tp->name) < 1)
			continue;

		/* input device exist, it's client->policy or server->policy */
		if (indev) {
			/* device name equal policy name */
			if (strcmp(indev, tp->ifname))
				continue;

			for (j = 0; j < tp->npserver; j++) {
				/* server to policy */
				if (tp->pservers[j].port == sport &&
				    tp->pservers[j].ip == sip)
				{
					nat->vip = tp->vip;
					nat->pip = sip;
					nat->port = sport;
					nat->nat_port = tp->pservers[j].nat_port;
					spin_unlock(&tp_policy_lock);
					return TP_SERVER2PROXY;
				}
				/* client to policy */
				if (tp->pservers[j].port == dport &&
				    tp->pservers[j].ip == dip) 
				{
					nat->vip = tp->vip;
					nat->pip = dip;
					nat->port = dport;
					nat->nat_port = tp->pservers[j].nat_port;
					spin_unlock(&tp_policy_lock);
					return TP_CLIENT2PROXY;
				}
			}
		}
		/* It's policy->client or policy->server */
		else {
			/* check policy->client */
			if (sip == tp->vip) {
				for (j = 0; j < tp->npserver; j++) {
					if (tp->pservers[j].nat_port == sport) {
						nat->vip = tp->vip;
						nat->pip = tp->pservers[j].ip;
						nat->port = dport;
						nat->nat_port = sport;
						spin_unlock(&tp_policy_lock);
						return TP_PROXY2CLIENT;
					}					
				}
			}
			/* check policy->server */
			else {
				for (j = 0; j < tp->npserver; j++) {
					if (tp->pservers[j].ip == dip &&
					    tp->pservers[j].port == dport)
					{
						nat->vip = tp->vip;
						nat->pip = tp->pservers[j].ip;
						nat->port = dport;
						nat->nat_port = tp->pservers[j].nat_port;
						spin_unlock(&tp_policy_lock);
						return TP_PROXY2SERVER;
					}
				}
			}
		}
	}

	spin_unlock(&tp_policy_lock);
	return TP_UNKNOW;
}

static void 
tp_set_checksum_in(struct sk_buff *skb, struct iphdr *iph, struct tcphdr *th)
{       
        if (unlikely(!skb || !iph || !th)) {
		printk(KERN_ERR"%d invalid argument\n", __LINE__);		
                return;
        }
        
        iph->check = 0;
        iph->check = ip_fast_csum((unsigned char*)iph, iph->ihl);
        skb->ip_summed = CHECKSUM_UNNECESSARY;
}

static int 
tp_nat_client2proxy(struct sk_buff *skb, 
		    struct ethhdr *eh, 
		    struct iphdr *iph, 
		    struct tcphdr *th, 
		    tp_nat_t *nat)
{
	if (unlikely(!skb || !eh || !iph || !th || !nat)) {
		printk(KERN_ERR"%d invalid argument\n", __LINE__);
		return NF_ACCEPT;
	}

	iph->daddr = nat->vip;
        th->dest = nat->nat_port;

	dev = dev_get_by_name(dev_net(in), nat->ifname);
        if (dev) {
                memcpy(eth_hdr(skb)->h_dest, dev->dev_addr, ETH_ALEN);
                dev_put(dev);
        }

        skb->pkt_type = PACKET_HOST;
        tp_set_checksum_in(skb, iph, th);

	return NF_ACCEPT;
}

static int 
tp_nat_proxy2client(struct sk_buff *skb, 
		    struct iphdr *iph, 
		    struct tcphdr *th, 
		    tp_nat_t *nat)
{
	if (unlikely(!skb || !eh || !iph || !th || !nat)) {
		printk(KERN_ERR"%d invalid argument\n", __LINE__);
		return NF_ACCEPT;
	}

	iph->daddr = nat->vip;
        th->dest = nat->nat_port;

	dev = dev_get_by_name(dev_net(in), nat->ifname);
        if (dev) {
                memcpy(eth_hdr(skb)->h_dest, dev->dev_addr, ETH_ALEN);
                dev_put(dev);
        }

        skb->pkt_type = PACKET_HOST;
        tp_set_checksum_in(skb, iph, th);

	return NF_ACCEPT;
}

static int 
tp_nat_proxy2server(struct sk_buff *skb, 
		    struct iphdr *iph, 
		    struct tcphdr *th, 
		    tp_nat_t *nat)
{
	if (unlikely(!skb || !eh || !iph || !th || !nat)) {
		printk(KERN_ERR"%d invalid argument\n", __LINE__);
		return NF_ACCEPT;
	}
	
	iph->saddr = nat->pip;
        th->source = nat->port;



	return NF_ACCEPT;
}

static int 
tp_nat_server2proxy(struct sk_buff *skb, 
		    struct ethhdr *eh, 
		    struct iphdr *iph, 
		    struct tcphdr *th,
		    tp_nat_t *nat)
{
	return NF_ACCEPT;
}

static u32 
tp_pre_routing(u32 hooknum, 
	       struct sk_buff *skb, 
	       const struct net_device *in, 
	       const struct net_device *out, 
	       int (*okfn)(struct sk_buff *))
{
	struct ethhdr *eh;
        struct iphdr *iph;
        struct tcphdr *th;
//	struct net_bridge_port *br_port;
	char br_name[IFNAMSIZ];
	tp_nat_t nat;
	int ret;
	int dir;
	
        if (in_interrupt() == 0) {
		printk("not in interrupt\n");
        }

	if (unlikely(skb == NULL || in == NULL)) {
                BUG();
		return NF_ACCEPT;
        }
        
	/* check eth header */
        eh = eth_hdr(skb);
        if (!eh)
                return NF_ACCEPT;

        if (ntohs(eh->h_proto) != ETH_P_IP) {
                return NF_ACCEPT;
        }

	/* check IP header */
        iph = ip_hdr(skb);
        if (iph == NULL)
                return NF_ACCEPT;

        if(iph->protocol != IPPROTO_TCP || skb->pkt_type != PACKET_OTHERHOST)
                return NF_ACCEPT;
	
	/* check TCP header */
	skb_set_transport_header(skb, iph->ihl * 4);
	th = tcp_hdr(skb);
	if (th == NULL) 
                return NF_ACCEPT;

#if 0
	/* get input device name */
	rcu_read_lock();

        br_port = br_port_get_rcu(in);
        if (br_port && br_port->br && br_port->br->dev) {
                snprintf(br_name, sizeof(br_name), "%s", br_port->br->dev->name);
        } 
	else {
                rcu_read_unlock();
                return NF_ACCEPT;
        }

        rcu_read_unlock();
#endif
	
	dir = tp_get_nat(&nat, iph->saddr, iph->daddr, th->source, th->dest, br_name);
	switch (dir) {
		
	case TP_CLIENT2PROXY:
		ret = tp_nat_client2proxy(skb, eh, iph, th, &nat);
		break;
	case TP_SERVER2PROXY:
		ret = tp_nat_server2proxy(skb, eh, iph, th, &nat);
		break;
	default:
		ret = NF_ACCEPT;
		break;
	}
	
	return ret;
}

static u32 
tp_post_routing(u32 hooknum, struct sk_buff *skb, 
		const struct net_device *in, 
		const struct net_device *out, 
		int (*okfn)(struct sk_buff *))
{
        struct iphdr *iph;
        struct tcphdr *th;
	tp_nat_t nat;
	int dir;
        int ret;
	
        if (in_interrupt() == 0) {
		printk("not in interrupt()\n");
        }
        
        if (unlikely(skb == NULL)) {
                return NF_ACCEPT;
        }
        
        iph = ip_hdr(skb);
        if (iph == NULL) 
		return NF_ACCEPT;
        
        if (iph->protocol != IPPROTO_TCP)
                return NF_ACCEPT;

	skb_set_transport_header(skb, iph->ihl * 4);
        th = tcp_hdr(skb);
        if (th == NULL) 
                return NF_ACCEPT;
	
        dir = tp_get_nat(&nat, iph->saddr, iph->daddr, th->source, th->dest, NULL);
	switch (dir) {

	case TP_PROXY2CLIENT:
		ret = tp_nat_proxy2client(skb, iph, th, &nat);
		break;
	case TP_PROXY2SERVER:
		ret = tp_nat_proxy2server(skb, iph, th, &nat);
		break;
	default:
		ret = NF_ACCEPT;
		break;
	}

	return ret;
}

static int 
tp_add_policy(tp_policy_t *tp)
{
	return 0;
}


static int 
tp_del_policy(tp_policy_t *tp)
{
	return 0;
}


static int __init
tproxy_init(void)
{
	int ret;
	
	ret = nf_register_hook(&tp_pre_routing_ops);
        if (ret) {
                printk(KERN_ERR "register pre routing hook failed\n");
                return 1;
        }

	ret = nf_register_hook(&tp_post_routing_ops);
        if (ret) {
                printk(KERN_ERR "register post routing hook failed\n");
                return 1;
        }
        
	printk("tproxy module init...\n");
	return 0;
}

static void __exit
tproxy_exit(void)
{
	spin_lock_init(&tp_policy_lock);

	nf_unregister_hook(&tp_pre_routing_ops);

	nf_unregister_hook(&tp_post_routing_ops);

	printk("tproxy module exit...\n");
}

/* the module parameter */
module_param(tp_policy_max, int, S_IRUSR);
module_param(tp_hash_size, int, S_IRUSR);
MODULE_PARM_DESC(tp_policy_max, " the max policy in tproxy");
MODULE_PARM_DESC(tp_hash_size, " the max hash size in tproxy");

module_init(tproxy_init);
module_exit(tproxy_exit);

