/*
 *  a simple character device driver in 2.6 kernel
 *
 *  write by Forrest.zhang
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>

#define SCULL_MAJOR               168
#define SCULL_MINOR               0
#define SCULL_NAME                "scull"
#define SCULL_DEVNUM              4
#define SCULL_QUANTUM             1024

/* scull device for /dev/scullX */
struct scull_dev {
    char           data[SCULL_QUANTUM];     /* store data in here */
    size_t         size;                    /* how many byte in data */
    struct cdev    cdev;                    /* cdev struct for scull */
    dev_t          devno;                   /* device number */
};
static struct scull_dev scull[SCULL_DEVNUM]; 

/* declare file_operations */
static struct file_operations scull_fops;


/*
 *  initiation scull_dev
 */
static int __init scull_dev_init(struct scull_dev *dev, dev_t devno)
{
    if (dev) {
	memset(dev, 0, sizeof(struct scull_dev));

	dev->devno = devno;

	cdev_init(&dev->cdev, &scull_fops);
	dev->cdev.owner = THIS_MODULE;
	if (cdev_add(&(dev->cdev), dev->devno, 1)) {
	    printk(KERN_ERR "cdev_add failor\n");
	    return -1;
	}
    }

    return 0;
}

/*
 *  clear all data in scull_dev
 */
static void scull_dev_clear(struct scull_dev *dev)
{
    if (dev) {
	memset(dev->data, 0, SCULL_QUANTUM);
	dev->size = 0;
    }
}

/*
 *  release scull_dev
 */
static void __exit scull_dev_free(struct scull_dev *dev)
{
    if (dev) {
	scull_dev_clear(dev);
	cdev_del(&dev->cdev);
    }
}


static int scull_open(
    struct inode *node,
    struct file  *file)
{
    struct scull_dev      *dev = NULL;
    
    dev = container_of(node->i_cdev, struct scull_dev, cdev);
    file->private_data = dev;

    if ( (file->f_flags & O_ACCMODE) == O_WRONLY)
	scull_dev_clear(dev);

    printk(KERN_ALERT "[%s]: open %d-%d, it contain %d bytes\n", 
	__FUNCTION__, MAJOR(dev->devno), MINOR(dev->devno), dev->size);

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
    struct file       *file,
    char __user       *buf,
    size_t            len,
    loff_t            *offset)
{
    ssize_t           nreads = 0;
    struct scull_dev  *dev = file->private_data;
    
    if (!dev) {
	nreads = -EFAULT;
	goto out;
    }

    if ( (long)*offset >= dev->size)
	goto out;
    
    if ( (long)*offset + len >= dev->size)
	nreads = dev->size - (long)*offset;
    else
	nreads = len;

    if (copy_to_user(buf, dev->data + (long)*offset, nreads)) {
	printk(KERN_ERR "copy bytes to user space failor\n");
	goto out;
    }

    *offset += nreads;

  out:
    return nreads;
}

static ssize_t scull_write(
    struct file       *file,
    const char __user *buf,
    size_t            len,
    loff_t            *offset)
{
    ssize_t           nwrites = 0;
    struct scull_dev  *dev = file->private_data;

    if (!dev) {
	nwrites = -EFAULT;
	goto out;
    }

    if ( (long)*offset >= SCULL_QUANTUM) {
	nwrites = -ENOMEM;
	goto out;
    }

    if ( (long)*offset + len >= SCULL_QUANTUM)
	nwrites = SCULL_QUANTUM - (long)*offset;
    else
	nwrites = len;

    if (copy_from_user(dev->data + (long)*offset, buf, nwrites)) {
	printk(KERN_ERR "copy bytes from user space failor\n");
	nwrites = -EFAULT;
	goto out;
    }

    *offset += nwrites;
    dev->size = ( (long)*offset) > dev->size ? ( (long)*offset) : dev->size;
    printk(KERN_ALERT "kernel receive %d bytes\n", nwrites);

  out:
    return nwrites;
}

static struct file_operations scull_fops = {
    .owner   = THIS_MODULE,
    .read    = scull_read,
    .write   = scull_write,
    .open    = scull_open,
    .release = scull_release,
};

static int __init scull_init(void)
{
    int         i = 0;

    /* register character device */
    if (register_chrdev_region(MKDEV(SCULL_MAJOR, SCULL_MINOR), SCULL_DEVNUM, SCULL_NAME)) {
	printk(KERN_ERR "scull register failor\n");
	return -1;
    }

    /* clear scull device */
    for (i = 0; i < SCULL_DEVNUM; i++)
	scull_dev_init(&scull[i], MKDEV(SCULL_MAJOR, SCULL_MINOR + i));

    printk(KERN_ALERT "scull initiation success\n");

    return 0;
}

static void __exit scull_exit(void)
{
    int         i = 0;

    /* release all memory in scull */
    for (i = 0; i < SCULL_DEVNUM; i++)
	scull_dev_free(&scull[i]);

    /* unregister character device */
    unregister_chrdev_region(MKDEV(SCULL_MAJOR, SCULL_MINOR), SCULL_DEVNUM);

    printk(KERN_ALERT "scull destroy success\n");
}

module_init(scull_init);
module_exit(scull_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Forrest.zhang");
