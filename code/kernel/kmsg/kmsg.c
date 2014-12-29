/*
 *	kmsg character device driver, it's used to send data between kernel
 *	and user space.
 * 
 *	the kernel and user-space can using kmsg to comunicate
 *	
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <asm/semaphore.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>

#define MIGLOG_DEVNUM     155		/* miglog device minor number */
#define MIGLOG_DEVNAM     "kmsg"	/* miglog device name */
#define MIGLOG_MAXLEN     2048		/* max log size */
#define MIGLOG_MAXLOG     8192		/* max log in queue */

/* the miglog message struct, log message follow it, like nlmsghdr */
struct kmsghdr{
	struct migloghdr *next;		/* next migloghdr */
	size_t		length;		/* log message length, */
	char		data[0];	/* the real data */
} kmsghdr_t;


/* the macro that handle kmsghdr struct, study from nlmsghdr's macro */
#define KMSG_ALIGNTO		4
#define KMSG_ALIGN(len)		(((len) + KMSG_ALIGNTO-1) & ~(KMSG_ALIGNTO-1))
#define KMSG_LENGTH(len)	(len + KMSG_ALIGN(sizeof(kmsghdr_t)))
#define KMSG_DATA(m)		((void*)(((char*)m) + sizeof(kmsghdr_t)))

/* the queue struct of migloghdr, add in tail and delete in head */
struct kmsg_queue {
	spinlock_t	lock;		/* lock for read and write */
	kmsghdr_t	*head;		/* head of migloghdr list */
	kmsghdr_t	*tail;		/* tail of migloghdr list */
	wait_queue_head_t outq;		/* wait queue */
	atomic_t	size;		/* size of queue */
} kmsg_queue_t;


/* the queues contain the message: kernel->user and user->kernel */
static kmsg_queue_t	kern2user;
static kmsg_queue_t	user2kern;

/***********************************************************************
 *  kmsg operator functions                                     *
 **********************************************************************/

/*
 *  init a kmsg_queue
 */
static void kmsg_queue_init(struct kmsg_queue *mq)
{
	if (mq) {
		memset(mq, 0, sizeof(kmsg_queue));
		mq->head = NULL;
		mq->tail = NULL;
		init_waitqueue_head(&mq->outq);
		spin_lock_init(&mq->lock);
		atomic_set(&mq->size, 0);
	}
}

/*
 *  add a kmsg into kmsg_queue's tail
 */
static void kmsg_queue_add(kmsg_queue_t *mq, kmsg_t *msg)
{
	if (mq && msg) {
		if (mq->tail)
			mq->tail->next = msg;
		
		msg->next = NULL;
		mq->tail = msg;
		
		if (!mq->head)
			mq->head = msg;
		
		atomic_inc(&mq->size);
	}
}

/*
 *  remove a migloghdr from miglog_queue's head and return it.
 */
static kmsghdr_t *kmsg_queue_del(kmsg_queue_t *mq)
{
	kmsghdr_t    *msg = NULL;
	
	if (mq) {
		msg = mq->head;
		mq->head = mq->head ? mq->head->next : NULL;
	
		if (!mq->head)
			mq->tail = NULL;
		
		if (msg)
			atomic_dec(&mq->size);
	}
	
	return msg;
}


/*
 *  clear a miglog_queue and release all resources alloced in it.
 */
static void kmsg_queue_clear(kmsg_queue_t *mq)
{
	kmsghdr_t  *msg = NULL;

	if (mq) {
		while(atomic_read(&mq->size) > 0) {
			msg = kmsg_queue_del(mq);
			kfree(msg);
		}
	}
}


/***********************************************************************
 *  miglog device operator functions                                   *
 **********************************************************************/

static int miglog_open(struct inode *inodep, struct file *filep)
{
    filep->private_data = &miglog_queue;

    return 0;
}

static int miglog_release(struct inode *inodep, struct file* filep)
{
    filep->private_data = NULL;

    return 0;
}

static ssize_t miglog_read(
    struct file     *filep,
    char __user     *buf,
    size_t           len,
    loff_t          *offset)
{
    ssize_t                  nreads = 0;
    struct migloghdr*        msg = NULL;
    struct migloghdr_queue   *mq = filep->private_data;

    /* no queue, error */
    if (!mq) {
	nreads = -EFAULT;
	goto out;
    }

    /* no message in queue, return 0 */
    if (atomic_read(&mq->size) == 0)
	goto out;

    /* get message from queue */
    spin_lock(&mq->lock);
    msg = miglog_queue_del(mq);
    spin_unlock(&mq->lock);

    /* no message in queue */
    if (!msg)
	goto out;

    /* 
     * need read how many bytes? 
     * NOTE, the first 4 bytes is the length of log, it'll return to use space
     */
    if (len < msg->length + sizeof(int))
	nreads = len;
    else
	nreads = msg->length + sizeof(int);

    /* copy data to user space, first 4 byte is length of data */
    if (copy_to_user(buf, MIGLOG_DATA(msg) - sizeof(int), nreads)) {
	nreads = -EFAULT;
	goto out;
    }

    kfree(msg);

  out:
    return nreads;
}

static ssize_t miglog_write(
    struct file       *filep,
    const char __user *buf,
    size_t             len,
    loff_t            *offset)
{
    size_t                  nwrites = len;
    struct migloghdr        *msg = NULL;
    struct migloghdr_queue  *mq = filep->private_data;

    /* queue is NULL, error */
    if (!mq) {
	nwrites = -EFAULT;
	goto out;
    }

    /* not exceed MIGLOG_MAXLOG logs */
    if (atomic_read(&mq->size) >= MIGLOG_MAXLOG) {
	nwrites = -ENOSPC;
	goto out;
    }

    /* how many bytes we need write, don't exceed MIGLOG_MAXLEN */
    if (len > MIGLOG_MAXLEN)
	nwrites = MIGLOG_MAXLEN;
    else
	nwrites = len;

    /* alloc buf to store log */
    msg = kmalloc(MIGLOG_LENGTH(nwrites), GFP_KERNEL);
    if (msg == NULL) {
	nwrites = -ENOMEM;
	goto out;
    }

    /* init migloghdr */
    memset(msg, 0, MIGLOG_LENGTH(nwrites));
    msg->length = nwrites;
	
    if (copy_from_user(MIGLOG_DATA(msg), buf, nwrites)) {
	kfree(msg);
	nwrites = -EFAULT;
	goto out;
    }

    /* add log to queue */
    spin_lock(&mq->lock);
    miglog_queue_add(mq, msg);
    spin_unlock(&mq->lock);

    /* wake up read poll */
    if (waitqueue_active(&mq->outq))
        wake_up_interruptible(&mq->outq);

  out:
    return nwrites;
}

static unsigned int miglog_poll(struct file *filep, poll_table *wait)
{
    unsigned int               mask = 0;
    struct migloghdr_queue     *mq = filep->private_data;

    if (!mq)
	return -EFAULT;

    /* add to read wait queue */
    poll_wait(filep, &mq->outq, wait);

    /* have data in it, can read */
    if (atomic_read(&mq->size) > 0)
	mask = POLLIN | POLLRDNORM;

    return mask;
}

static struct file_operations miglog_fops = {
    .open    = miglog_open,
    .release = miglog_release,
    .read    = miglog_read,
    .write   = miglog_write,
    .poll    = miglog_poll,
};

static struct miscdevice miglog_dev = {
    .minor      = MIGLOG_DEVNUM,
    .name       = MIGLOG_DEVNAM,
    .fops       = &miglog_fops,
};

static int __init miglog_init(void)
{
    /* register miglog */
    if (misc_register(&miglog_dev) < 0) {
	printk(KERN_ERR "register miglog device error\n");
	return -EFAULT;
    }

    /* init queue */
    miglog_queue_init(&miglog_queue);

    printk("miglog initiation done.\n");

    return 0;
}

static void __exit miglog_exit(void)
{
    /* free all unused data */
    miglog_queue_clear(&miglog_queue);

    /* register miglog as misc device */
    if (misc_deregister(&miglog_dev) < 0) {
	printk(KERN_ERR "unregister miglog device error\n");
    }
}

module_init(miglog_init);
module_exit(miglog_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Forrest.zhang");
