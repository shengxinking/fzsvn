/**
 *	@file	fz_sock.c
 *
 *	@brief	A simple BSD style socket implement in kernel.
 *	
 *	@author	Forrest.zhang
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/net.h>

#include "af_fz.h"

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Forrest Zhang");


/* the max message in socket recv queue */
#define _FZ_SOCK_RECV_QLEN		64

/* List of all packet sockets. */
static struct admin_sock *_fz_sklist = NULL;             /* store all opened socket in it */
static rwlock_t           _fz_lock = RW_LOCK_UNLOCKED;   
atomic_t                  _fz_socks_nr = ATOMIC_INIT(0); /* how many socket */

static struct proto _fz_sock_proto = {
	.name     = "FZ",
	.owner    = THIS_MODULE,
	.obj_size = sizeof(struct fz_sock),
};

static struct proto_ops _fz_sock_ops;

/*
 *  destruct a socket
 */
static void _fz_sock_destruct(struct sock *sk)
{

}

/*
 *  create a socket (user call socket), and add it to list admin_list
 */
static int _fz_sock_create(
//	struct net    *net,
	struct socket *sock, 
	int           protocol)
{
	return 0;
}


/*
 *	send a packet to sock in admin_sklist according rule. Parameter @user is mean which send packet
 *	1 means userspace send packet, 0 means kernel space send packet. the packet is send to sock which
 *	have same group and same pid.
 */
static int _fz_sock_send(
	struct sock             *sk,
	struct sk_buff          *skb,
	struct sockaddr_fz	*daddr, 
	int                      user)
{
	return 0;
}

/**
 *	Send packet function of admin socket
 *
 *	Return send bytes if success, < 0 if error.
 */
static int _fz_sock_sendmsg(
	struct kiocb   *iocb,
	struct socket  *sock, 
	struct msghdr  *msg, 
	size_t          len)
{
	return(len);
}

/*
 *	pull a packet from our receive queue and hand it to the user.
 *	If necessary we block.
 *
 *	Return recv bytes if success, or else return < 0
 */
static int _fz_sock_recvmsg(
	struct kiocb   *iocb,
	struct socket  *sock,
	struct msghdr  *msg,
	size_t          len,
	int             flags)
{
	return 0;
}


/*
 *	Bind  a address to sock
 *
 *	Return 0 if success, other on error.
 */
static int _fz_sock_bind(
	struct socket    *sock, 
	struct sockaddr  *uaddr, 
	int               addr_len)
{
	return 0;
}


/*
 *	Get local socket address
 *
 *	Return 0 if success, other on error.
 */
static int _fz_sock_getname(
	struct socket     *sock, 
	struct sockaddr   *uaddr,
	int               *uaddr_len, 
	int                peer)
{
	return 0;
}

/**
 *	Setsockopt on admin socket, not supported now
 */
static int _fz_sock_setsockopt(
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
static int _fz_sock_getsockopt(
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
static int _fz_sock_release(struct socket *sock)
{
	return 0;
}

static struct proto_ops _fz_sock_ops = {
	.family     = PF_PACKET,
	.owner      = THIS_MODULE,
	.release    = _fz_sock_release,
	.bind       = _fz_sock_bind,
	.connect    = sock_no_connect,
	.socketpair = sock_no_socketpair,
	.accept     = sock_no_accept,
	.getname    = _fz_sock_getname, 
	.poll       = datagram_poll,
	.ioctl      = sock_no_ioctl,
	.listen     = sock_no_listen,
	.shutdown   = sock_no_shutdown,
	.setsockopt = sock_no_setsockopt,
	.getsockopt = sock_no_getsockopt,
	.sendmsg    = _fz_sock_sendmsg,
	.recvmsg    = _fz_sock_recvmsg,
	.mmap       = sock_no_mmap,
	.sendpage   = sock_no_sendpage,
};

static struct net_proto_family _fz_family_ops = {
	.family = PF_FZ,
	.create = _fz_sock_create,
	.owner  = THIS_MODULE,
};


static int __init _fz_sock_init(void)
{
	sock_register(&_fz_sock_ops);
	printk("admin socket init done\n");

	return 0;
}


static void __exit _fz_sock_exit(void)
{
	sock_unregister(PF_FZ);
	return;
}


module_init(_fz_sock_init);
module_exit(_fz_sock_exit);
