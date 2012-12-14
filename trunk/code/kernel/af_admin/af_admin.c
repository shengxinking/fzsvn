/*
 *  An implementation of management domain socket.
 *  Hongwei Li, Fortinet INC.
*/

#include <linux/module.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/fcntl.h>
#include <linux/socket.h>
#include <linux/in.h>
#include <linux/inet.h>
#include <linux/netdevice.h>
#include <linux/if_packet.h>
#include <net/ip.h>
#include <net/protocol.h>
#include <linux/skbuff.h>
#include <net/sock.h>
#include <linux/errno.h>
#include <linux/timer.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/ioctls.h>
#include <linux/proc_fs.h>
#include <linux/poll.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/if_bridge.h>
#include "af_admin.h"


#define ADMIN_SOCK_RECV_QLEN		64

/* List of all packet sockets. */
static struct admin_sock *admin_sklist = NULL;             /* store all opened socket in it */
static rwlock_t           admin_lock = RW_LOCK_UNLOCKED;   
atomic_t                  admin_socks_nr = ATOMIC_INIT(0); /* how many socket */

static struct proto admin_proto = {
	.name     = "ADMIN",
	.owner    = THIS_MODULE,
	.obj_size = sizeof(struct admin_sock),
};

static struct proto_ops admin_ops;

/*
 *  destruct a socket
 */
static void admin_sock_destruct(struct sock *sk)
{
	BUG_TRAP(atomic_read(&sk->sk_rmem_alloc) == 0);
	BUG_TRAP(atomic_read(&sk->sk_wmem_alloc) == 0);

	/* free alloced memory */
	if (sk->sk_protinfo){
		kfree(sk->sk_protinfo);
	}
	atomic_dec(&admin_socks_nr);
}

/*
 *  create a socket (user call socket), and add it to list admin_list
 */
static int admin_create(
	struct net    *net,
	struct socket *sock, 
	int           protocol)
{
	struct sock         *sk;
	struct admin_sock   *ask;
	int                  err = -ENOBUFS;

#if 0
	/* no privilege to create */
	if (!capable(CAP_NET_RAW))
		return -EPERM;
#endif	

	/* only support SOCK_DGRAM */
	if (sock->type != SOCK_DGRAM) 
		return -ESOCKTNOSUPPORT;
    
	sock->state = SS_UNCONNECTED;
    
	/* 
	 * alloc a sock struct, but it's size is admin_sock size, can 
	 * safty convert to admin_sock struct, so can add it to admin_list 
	 */
	sk = sk_alloc(net, PF_PACKET, GFP_KERNEL, &admin_proto);
	if (sk == NULL)
		goto out;

	sock->ops = &admin_ops;
	sock_init_data(sock, sk);

	/* alloc sock proto info */
	sk->sk_protinfo = kmalloc(sizeof(struct admin_opt), GFP_KERNEL);
	if (sk->sk_protinfo == NULL)
		goto out_free;
	memset(sk->sk_protinfo, 0, sizeof(struct admin_opt));

	sk->sk_family = PF_ADMIN;
	sk->sk_protocol = protocol;   
	sk->sk_destruct = admin_sock_destruct;
	atomic_inc(&admin_socks_nr);

	/* convert admin_sock to common sock structure */
	ask = (struct admin_sock*)sk;

	/* add admin_sock to admin_sklist */
	write_lock_bh(&admin_lock);

	ask->next = admin_sklist;
	admin_sklist = ask;
	sk_node_init(&admin_sklist->sk.sk_node);

	write_unlock_bh(&admin_lock);
    
	return 0;

out_free:
	sk_free(sk);
out:
	return err;
}


/*
 *	send a packet to sock in admin_sklist according rule. Parameter @user is mean which send packet
 *	1 means userspace send packet, 0 means kernel space send packet. the packet is send to sock which
 *	have same group and same pid.
 */
static int admin_send_packet(
	struct sock             *sk,
	struct sk_buff          *skb,
	struct sockaddr_admin   *daddr, 
	int                      user)
{
	struct admin_sock  *next = NULL;
	struct sk_buff     *skb1;
    
	/* get read lock */
	if (user) 
		read_lock_bh(&admin_lock);
	else 
		read_lock(&admin_lock);

	/* send it to group */
	for ( next = admin_sklist; next; next = next->next) {

		/* same socket, skipped */
		if (sk == &(next->sk))
			continue;

		/* not enough receive queue in sock, skipped */
		if (next->sk.sk_receive_queue.qlen > ADMIN_SOCK_RECV_QLEN) 
			continue;

		/* not in same group, skipped */
		if (daddr->group != ( (struct admin_opt*)(next->sk.sk_protinfo) )->group)
			continue;

		/* not same pid if pid > 0, skipped */
		if (daddr->pid && 
		   (daddr->pid != ((struct admin_opt*)(next->sk.sk_protinfo))->pid))
			continue;

		/* clone a skb, add it to current sock receive queue */
		skb1 = skb_clone(skb, GFP_ATOMIC);
		if (skb1 == NULL)
			break;

		skb_queue_tail(&((next->sk).sk_receive_queue), skb1);

		/* wake up socket wait on recv */
		next->sk.sk_data_ready(&(next->sk), skb1->len);
	}

	/* free old skb */
	kfree_skb(skb);
    
	if(user) 
		read_unlock_bh(&admin_lock);
	else 
		read_unlock(&admin_lock);

	return 0;
}

/**
 *	Send packet function of admin socket
 *
 *	Return send bytes if success, < 0 if error.
 */
static int admin_sendmsg(
	struct kiocb   *iocb,
	struct socket  *sock, 
	struct msghdr  *msg, 
	size_t          len)
{
	struct sock            *sk = sock->sk;
	struct sockaddr_admin  *daddr = (struct sockaddr_admin *)msg->msg_name;
	struct sockaddr_admin   saddr;
	struct sk_buff         *skb;
	int                     err = -EINVAL;

	/* no destination address */
	if(daddr == NULL) 
		goto out;
    
	/* destination address length not enough */
	if (msg->msg_namelen < sizeof(struct sockaddr_admin))
		goto out;

	/* alloc sk_buff for send message */
	skb = sock_alloc_send_skb(sk, len, msg->msg_flags & MSG_DONTWAIT, &err);
	if (skb == NULL)	
		goto out;
    
	/* copy send data from msg to skb */
	err = memcpy_fromiovec(skb_put(skb, len), msg->msg_iov, len);
	if (err)
		goto out_free;

	/* set sender's address */
	saddr.family = AF_ADMIN;
	saddr.pid = ((struct admin_opt *)sk->sk_protinfo)->pid;
	saddr.group = ((struct admin_opt *)sk->sk_protinfo)->group;
	memcpy(skb->cb, &saddr, sizeof(saddr));

	/* copy admin_send_packet to send packet */
	skb_orphan(skb);
	admin_send_packet(sk, skb, daddr, 1);

	return(len);

out_free:
	kfree_skb(skb);
out:
	return err;
}

/*
 *	pull a packet from our receive queue and hand it to the user.
 *	If necessary we block.
 *
 *	Return recv bytes if success, or else return < 0
 */
static int admin_recvmsg(
	struct kiocb   *iocb,
	struct socket  *sock,
	struct msghdr  *msg,
	size_t          len,
	int             flags)
{
	struct sock         *sk = sock->sk;
	struct sk_buff      *skb = NULL;
	int                  copied;
	int                  err = -EINVAL;

	/* not support MSG_PEEK, MSG_DONTWAIT, MSG_TRUNC */
	if (flags & ~(MSG_PEEK | MSG_DONTWAIT | MSG_TRUNC) )
		goto out;

	/* address length not enough */
	if (msg->msg_namelen < sizeof(struct sockaddr_admin) )
		goto out;

	msg->msg_namelen = sizeof(struct sockaddr_admin);

	/* get a packet from recv queue */
	skb = skb_recv_datagram(sk, flags, flags & MSG_DONTWAIT, &err);
	if (skb == NULL)
		goto out;
	copied = skb->len;

	/* user provide not enough room, truncate it */
	if (copied > len) {
		copied = len;
		msg->msg_flags |= MSG_TRUNC;
	}

	/* copy data from skb to msg */
	err = skb_copy_datagram_iovec(skb, 0, msg->msg_iov, copied);
	if (err)
		goto out_free;

	/* copy sender's address to msg */
	if (msg->msg_name)
		memcpy(msg->msg_name, skb->cb, msg->msg_namelen);
	
	err = (flags & MSG_TRUNC) ? skb->len : copied;

out_free:
	kfree_skb(skb);

out:
	return err;
}


/*
 *	Bind  a address to sock
 *
 *	Return 0 if success, other on error.
 */
static int admin_bind(
	struct socket    *sock, 
	struct sockaddr  *uaddr, 
	int               addr_len)
{
	struct sockaddr_admin *addr = (struct sockaddr_admin*)uaddr;
	struct admin_sock     *sk = (struct admin_sock*)sock->sk; 
	struct admin_sock     *sk2;
	int                    err = -EEXIST;

	/* verify protocol */
	if(addr->family != AF_ADMIN) 
		return -EINVAL;
    
	lock_sock(&sk->sk);

	/* find if a old bind exist */
	read_lock_bh(&admin_lock);
	for(sk2 = admin_sklist; sk2; sk2 = sk2->next) {
		/* same socket */
		if(sk2 == sk) 
			continue;
	
		/* bind exist */
		if(((struct admin_opt *)sk2->sk.sk_protinfo)->pid == addr->pid &&
		   ((struct admin_opt *)sk2->sk.sk_protinfo)->group & addr->group) 
			break;
	}
	read_unlock_bh(&admin_lock);
    
	/* add addr->pid and addr->group to sk */
	if(sk2 == NULL) {
		((struct admin_opt *)sk->sk.sk_protinfo)->pid = addr->pid;
		((struct admin_opt *)sk->sk.sk_protinfo)->group = addr->group;
		err = 0;
	}
    
	release_sock(&sk->sk);

	return err;
}


/*
 *	Get local socket address
 *
 *	Return 0 if success, other on error.
 */
static int admin_getname(
	struct socket     *sock, 
	struct sockaddr   *uaddr,
	int               *uaddr_len, 
	int                peer)
{
	struct sock              *sk = sock->sk;
	struct sockaddr_admin    *addr = (struct sockaddr_admin*)uaddr;

	/* can't get peer address */
	if (peer)
		return -EOPNOTSUPP;

	if(*uaddr_len < sizeof(*addr)) 
		return -ENOMEM;

	addr->family = AF_PACKET;
	addr->pid = ((struct admin_opt *)sk->sk_protinfo)->pid;
	addr->group = ((struct admin_opt *)sk->sk_protinfo)->group;
	*uaddr_len = sizeof(*addr);
    
	return 0;
}

/**
 *	Setsockopt on admin socket, not supported now
 */
static int admin_setsockopt(
	struct socket   *sock,
	int              level, 
	int              optname, 
	char            *optval, 
	int              optlen)
{
	return -ENOPROTOOPT;
}

/*
 *	Getsockopt on admin socket, not supported now
 *
 */
static int admin_getsockopt(
	struct socket   *sock, 
	int              level, 
	int              optname,
	char            *optval, 
	int             *optlen)
{
	return -ENOPROTOOPT;
}

/*
 *	Release a socket (user call close)
 *
 *	Return 0 if success, other on error.
 */
static int admin_release(struct socket *sock)
{
	struct sock *sk         = sock->sk;
	struct admin_sock *next = NULL;
	struct admin_sock *prev = NULL;

	if (!sk)
		return 0;
    
	write_lock_bh(&admin_lock);
	for (next = admin_sklist; next; next = next->next) {
		if (&next->sk == sk) {
			if (prev)
				prev->next = next->next;
			else
				admin_sklist = next->next;
			break;
		}
		prev = next;
	}
	write_unlock_bh(&admin_lock);

	sock_orphan(sk);
	sock->sk = NULL;
    
	/* purge queues */
	skb_queue_purge(&next->sk.sk_receive_queue);
    
	sock_put(sk);

	return 0;
}

static struct proto_ops admin_ops = {
	.family     = PF_PACKET,
	.owner      = THIS_MODULE,
	.release    = admin_release,
	.bind       = admin_bind,
	.connect    = sock_no_connect,
	.socketpair = sock_no_socketpair,
	.accept     = sock_no_accept,
	.getname    = admin_getname, 
	.poll       = datagram_poll,
	.ioctl      = sock_no_ioctl,
	.listen     = sock_no_listen,
	.shutdown   = sock_no_shutdown,
	.setsockopt = admin_setsockopt,
	.getsockopt = admin_getsockopt,
	.sendmsg    = admin_sendmsg,
	.recvmsg    = admin_recvmsg,
	.mmap       = sock_no_mmap,
	.sendpage   = sock_no_sendpage,
};

static struct net_proto_family admin_family_ops = {
	.family = PF_ADMIN,
	.create = admin_create,
	.owner  = THIS_MODULE,
};

static void __exit admin_exit(void)
{
	sock_unregister(PF_ADMIN);
	return;
}

static int __init admin_init(void)
{
	sock_register(&admin_family_ops);
	printk("admin socket init done\n");

	return 0;
}

module_init(admin_init);
module_exit(admin_exit);
