/*
 *  a simple charater driver in linux 2.6
 *
 *  write by Forrest.zhang
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/fs.h>

#define SCULL_MAJOR                  168
#define SCULL_DEVNUM                 4
#define SCULL_MINOR                  0
#define SCULL_NAME                   "scull"
#define SCULL_QUANTUM                128
#define _DEBUG_                      1

#ifdef _DEBUG_
#    define DBG(fmt, args...)        printk(KERN_WARNING "scull: " fmt, ## args)
#else
#    define DBG(fmt, args...)
#endif

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Forrest.zhang");

struct scull_dev {
    char                data[SCULL_QUANTUM];
    size_t              size;
    dev_t               devno;
    struct cdev         cdev;
};

static struct scull_dev       scull[SCULL_DEVNUM];
static struct file_operations scull_fops;

static int __init scull_dev_init(struct scull_dev *dev, int major, int minor)
{
    if (dev) {
	memset(dev, 0, sizeof(struct scull_dev));

	dev->devno = MKDEV(major, minor);

	cdev_init(&dev->cdev, &scull_fops);
	dev->cdev.owner = THIS_MODULE;
	if (cdev_add(&dev->cdev, dev->devno, 1))
	    return -1;
    }

    return 0;
}

static void __exit scull_dev_free(struct scull_dev *dev)
{
    if (dev) {
	cdev_del(&dev->cdev);
    }
}

static void scull_dev_clear(struct scull_dev *dev)
{
    if (dev) {
	memset(dev->data, 0, SCULL_QUANTUM);
	dev->size = 0;
    }	
}

static int scull_open(
    struct inode *inode,
    struct file  *file)
{
    struct scull_dev    *dev = NULL;
    struct cdev         *cdev = NULL;

    cdev = inode->i_cdev;
    dev = container_of(cdev, struct scull_dev, cdev);
    
    file->private_data = dev;

    if ( (file->f_mode & O_ACCMODE) == O_WRONLY)
	scull_dev_clear(dev);

    printk(KERN_ALERT "you open %d-%d device\n", MAJOR(dev->devno), MINOR(dev->devno));

    return 0;
}

static int scull_release(
    struct inode *node,
    struct file  *file)
{
    file->private_data = NULL;
    return 0;
}

static ssize_t scull_read(
    struct file         *file,
    char __user         *buf,
    size_t              len,
    loff_t              *offset)
{
    ssize_t             nread = 0;      /* read bytes */
    struct scull_dev    *dev = file->private_data;

    /* no scull_dev */
    if (dev == NULL) {
	nread = -ENOMEM;
	goto out;
    }
    
    /* no data in scull_dev */
    if (dev->size == 0)
	goto out;

    /* offset is out of range */
    if (*offset >= dev->size)
	goto out;

    /* calc the bytes can be read */
    if ((long)*offset + len >= SCULL_QUANTUM)
	nread = SCULL_QUANTUM - (long)*offset;
    else
	nread = len;

    if (copy_to_user(buf, dev->data + *offset, nread)) {
	printk(KERN_ERR "copy data to user space error\n");
	nread = -EFAULT;
	goto out;
    }

  out:
    return nread;
}

static ssize_t scull_write(
    struct file         *file,
    const char __user   *buf,
    size_t              len,
    loff_t              *offset)
{
    return -EBUSY;
}

static struct file_operations scull_fops = {
    .owner   = THIS_MODULE,
    .open    = scull_open,
    .release = scull_release,
    .read    = scull_read,
    .write   = scull_write,
};

static int __init scull_init(void)
{
    int      err = 0;
    int      i = 0;
    dev_t    first = MKDEV(SCULL_MAJOR, SCULL_MINOR);

    if ((err = register_chrdev_region(first, SCULL_DEVNUM, SCULL_NAME))) {
	printk(KERN_ERR "register %s error\n", SCULL_NAME);
	goto out;
    }

    for (i = 0; i < SCULL_DEVNUM; i++)
	scull_dev_init(&(scull[i]), SCULL_MAJOR, SCULL_MINOR + i);

    DBG("register %s success\n", SCULL_NAME);

  out:
    return err;
}

static void __exit scull_exit(void)
{
    int         i = 0;

    for (i = 0; i < SCULL_DEVNUM; i++)
	scull_dev_free(&scull[i]);

    unregister_chrdev_region(MKDEV(SCULL_MAJOR, SCULL_MINOR), SCULL_DEVNUM);

    DBG("unregister %s success\n", SCULL_NAME);
}

module_init(scull_init);
module_exit(scull_exit);


