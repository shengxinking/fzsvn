/*
 *  this is a kernel thread can send and receive datagram packet
 *  I register a char device, and use ioctl control thread.
 *
 *  write by Forrest.zhang
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <net/ip.h>
#define _KDEBUG_                1
#include "kdebug.h"
#include "ksync.h"

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Forrest.zhang");

#define MSG_LEN                    128

static unsigned long  sync_mode    = SYNC_STOP;
static unsigned long  sync_address = 0;
static unsigned short sync_port    = 0;
static unsigned long  sync_delay   = 256;

/*
 *  send a udp packet every sync_delay second
 */
static void sync_send(void)
{
    struct socket      *sock;
    struct sockaddr_in addr;
    struct msghdr      msg;
    struct kvec        vec;
    char               buf[MSG_LEN] = {0};
    int                i;
   
    /* create socket */
    if (sock_create_kern(PF_INET, SOCK_DGRAM, 0, &sock) < 0) {
	ERR("create kernel socket error\n");
	goto out;
    }

    for (i = 0; i < sync_delay; i++) {	

	/* error parameter */
	if (sync_address == 0 || 
	    sync_port == 0  ||
	    sync_delay <= 0) {
	    ERR("parameter error\n");
	    goto out_free;
	}
	
	/* mode is change */
	if (sync_mode != SYNC_MASTER) {
	    DBG("change mode from MASTER to %lu\n", sync_mode);
	    goto out_free;
	}

	/* set peer address */
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = sync_address;
	addr.sin_port        = sync_port;
	
	/* set msg */
	memset(&msg, 0, sizeof(msg));
	msg.msg_name = &addr;
	msg.msg_namelen = sizeof(addr);
	msg.msg_flags = MSG_DONTWAIT |MSG_NOSIGNAL;

	/* set vec */
	memset(buf, 0, MSG_LEN);
	snprintf(buf, MSG_LEN, "[%ld]: jiffies is %lu", (long)current->pid, jiffies);
	vec.iov_base = buf;
	vec.iov_len  = MSG_LEN;

	/* send message */
	kernel_sendmsg(sock, &msg, &vec, 1, MSG_LEN);

	DBG("send a message: %s\n", buf);
    }

  out_free:
    sock_release(sock);

  out:
    return;
}

/*
 *  receive udp packet
 */
static void sync_recv(void)
{
    struct socket      *sock;
    struct sockaddr_in addr;
    struct msghdr      msg;
    struct kvec        vec;
    char               buf[MSG_LEN] = {0};
    ssize_t            len;

    /* create socket */
    if (sock_create_kern(PF_INET, SOCK_DGRAM, 0, &sock) < 0) {
	ERR("create kernel socket error\n");
	goto out;
    }

    /* error parameter */
    if (sync_address == 0 || 
	sync_port == 0  ||
	sync_delay <= 0) {
	ERR("parameter error\n");
	goto out_free;
    }

    /* set local address */
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = sync_address;
    addr.sin_port        = sync_port;

    if (sock->ops->bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
	ERR("bind socket error\n");
	goto out_free;
    }

    while (1) {	

	/* error parameter */
	if (sync_address == 0 || 
	    sync_port == 0  ||
	    sync_delay <= 0) {
	    ERR("parameter error\n");
	    goto out_free;
	}
	
	/* mode is change */
	if (sync_mode != SYNC_SLAVE) {
	    DBG("change mode from SLAVE to %lu\n", sync_mode);
	    goto out_free;
	}

	/* set msg */
	memset(&msg, 0, sizeof(msg));

	/* set vec */
	memset(buf, 0, MSG_LEN);
	vec.iov_base = buf;
	vec.iov_len  = MSG_LEN;

	len = kernel_recvmsg(sock, &msg, &vec, 1, MSG_LEN, MSG_DONTWAIT);

	if (len > 0)
	    DBG("recv: %s\n", buf);

	
    }

  out_free:
    sock_release(sock);

  out:
    return;
}

static int thread_main(void* data)
{
    int   retval = 0;

    /* master: send udp packet */
    if (sync_mode == SYNC_MASTER) {
	DBG("run in master mode, address is %lu, port is %u, delay is %lu\n",
	    sync_address, ntohs(sync_port), sync_delay);
	sync_send();
    }

    /* slave: receive udp packet */
    if (sync_mode == SYNC_SLAVE) {
	DBG("run in slave mode, address is %lu, port is %u\n",
	    sync_address, ntohs(sync_port));
	sync_recv();
    }

    /* exit from thread, the mode is STOP */
    sync_mode = SYNC_STOP;

    return retval;
}

static int thread_first(void* data)
{
    int         pid;
    int         retval = 0;
    
  repeat:
    
    if ( (pid = kernel_thread(thread_main, data, 0)) < 0) {
	ERR("create second kernel thread error\n");
	goto repeat;
    }

    DBG("create second kernel thread %d success\n", pid);

    return retval;
}

static void run_thread(void)
{
    int         pid;
    
  repeat:

    if ( (pid = kernel_thread(thread_first, NULL, 0)) < 0) {
	ERR("create first kernel thread error\n");
	goto repeat;
    }

    DBG("create first kernel thread %d success\n", pid);
}

static int sync_open(
    struct inode     *inode,
    struct file      *file)
{
    if (!try_module_get(THIS_MODULE))
	return ENODEV;

    DBG("%s success\n", __FUNCTION__);
    
    return 0;
}

static int sync_release(
    struct inode     *inode,
    struct file      *file)
{
    module_put(THIS_MODULE);
    
    DBG("%s success\n", __FUNCTION__);

    return 0;
}

static int sync_ioctl(
    struct inode     *inode,
    struct file      *file,
    unsigned int     cmd,
    unsigned long    param)
{
    int retval = 0;

    switch (cmd) {
    case IO_SET_MODE:
	
	DBG("set status %lu\n", param);
	
	/* param error */
	if (param != SYNC_STOP && 
	    param != SYNC_MASTER &&
	    param != SYNC_SLAVE) {
	    ERR("error parameter when using ioctl IO_SET_MODE\n");
	    retval = -EINVAL;
	    break;
	}
	
	/* 
	 * start thread if origin mode is SYNC_STOP and ioctl 
         * set SYNC_MASTER or SYNC_SLAVE
	 */
	if (sync_mode == SYNC_STOP && param != SYNC_STOP)
	    run_thread();
	
	sync_mode = param & SYNC_MODE;

	break;
	
    case IO_GET_MODE:

	DBG("get status %lu\n", sync_mode);

	retval = sync_mode;

	break;

    case IO_SET_ADDRESS:

	DBG("set address %lu\n", param);

	sync_address = param;

	break;

    case IO_GET_ADDRESS:

	DBG("get address %lu\n", sync_address);

	retval = sync_address;

	break;

    case IO_SET_PORT:
	DBG("set port %lu\n", param);
	sync_port = htons(param);
	break;

    case IO_GET_PORT:
	DBG("get prot %u\n", sync_port);

	retval = ntohs(sync_port);

	break;

    case IO_SET_DELAY:
	DBG("set delay %lu\n", param);

	sync_delay = param;

	break;

    case IO_GET_DELAY:
	DBG("get delay %lud\n", sync_delay);

	retval = sync_delay;

	break;
    default:
	ERR("error command %d\n", cmd);

	retval = -EINVAL;

	break;
    }

    return retval;
}

static struct file_operations  sync_fops = {
    .open    = sync_open,
    .release = sync_release,
    .ioctl   = sync_ioctl,
};

static int __init sync_init(void)
{
    if (register_chrdev(SYNC_MAJOR, SYNC_NAME, &sync_fops)) {
	ERR("register character device error\n");
	return -1;
    }

    DBG("%s success\n", __FUNCTION__);
    
    return 0;
}

static void __exit sync_exit(void)
{
    unregister_chrdev(SYNC_MAJOR, SYNC_NAME);

    DBG("%s success\n", __FUNCTION__);
}

module_init(sync_init);
module_exit(sync_exit);
