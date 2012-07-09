#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/config.h>
#include <linux/ip.h>
#include <linux/netfilter_ipv4.h>
#include <net/tcp.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h> 

static unsigned int sample(unsigned int hooknum,struct sk_buff **skb,
				const struct net_device *in,
				const struct net_device *out,int (*okfn)(struct sk_buff *))
{

	
	return NF_ACCEPT;			

}

static unsigned int outprocess(unsigned int hooknum,struct sk_buff **skb,
				const struct net_device *in,
				const struct net_device *out,int (*okfn)(struct sk_buff *))
{
	
	return NF_ACCEPT;
}

static unsigned int inprocess(unsigned int hooknum,struct sk_buff **skb,
                                const struct net_device *in,
                                const struct net_device *out,int (*okfn)(struct sk_buff *))
{
	return NF_ACCEPT;
}



static struct nf_hook_ops iplimitfilter
={ {NULL,NULL} ,sample,NULL,PF_INET,NF_IP_FORWARD,132};

static struct nf_hook_ops outfilter
={ {NULL,NULL} ,outprocess,NULL,PF_INET,NF_IP_LOCAL_OUT,132};

static struct nf_hook_ops infilter
={ {NULL,NULL} ,inprocess,NULL,PF_INET,NF_IP_LOCAL_IN,132};



static void filter_exit(void)
{
	
	nf_unregister_hook(&iplimitfilter);
	nf_unregister_hook(&outfilter);
	nf_unregister_hook(&infilter);

}

static int filter_init(void)
{
	
	nf_register_hook(&iplimitfilter);
	nf_register_hook(&outfilter);
	nf_register_hook(&infilter);


	struct socket *sock;
	int ps = 0;
        unsigned char buffer[128] = {0};
        struct msghdr msg;
        struct kvec vec;
        struct sockaddr_in addr,addr1;
	int size = sizeof(struct sockaddr_in);

        memset(buffer,0x0,128);
        memset(&addr, 0x00, sizeof(addr));

        addr.sin_family = AF_INET;
        addr.sin_port = htons(239);
        addr.sin_addr.s_addr = INADDR_ANY;

        vec.iov_base=buffer;
        vec.iov_len=128;

        memset(&msg, 0x00, sizeof(msg));
        //msg.msg_name=addr;
        //msg.msg_namelen=sizeof(addr);
        msg.msg_flags = MSG_DONTWAIT|MSG_NOSIGNAL;



        if (sock_create_kern(PF_INET, SOCK_STREAM, IPPROTO_TCP, &sock) < 0) {
                printk("create sock failed\n");
                return 0;
        }

        if (sock->ops->bind(sock,
                               (struct sockaddr*)&addr,
                               sizeof(struct sockaddr_in)) < 0) {
                printk(" bind failed\n");
                return 0;
        }
	int dd = sock->ops->listen(sock,10);
	if(dd == -1)
	{	
		printk("listen failed\n");
		return 0;
	}
	while(1)
	{	
		int rst = 0;
		rst = sock->ops->accept(sock,
                               	 (struct sockaddr*)(&addr1),&size);
		if(rst < 0)
			continue;
		else
		{	
			printk("new connection %d\n",rst);	
			return 0;
		}
				 
	}
	return 0;
}

module_init(filter_init);
module_exit(filter_exit); 
MODULE_LICENSE("GPL");


