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
#define SCULL_QUANTUM             16

/* data item */
struct scull_data {
    char              buf[SCULL_QUANTUM];
    struct scull_data *next;
}

/* scull device for /dev/scullX */
struct scull_dev {
    struct scull_data *data;                   /* store data in here */
    atomic_t          nitems;                  /* how many scull_data in data */
    size_t            size;                    /* how many byte in data */
    struct cdev       cdev;                    /* cdev struct for scull */
    dev_t             devno;                   /* device number */
};
static struct scull_dev scull[SCULL_DEVNUM]; 

/* declare file_operations */
static struct file_operations scull_fops;

/*
 *  free scull_data list
 */
static void scull_data_clear(struct scull_data *data)
{
    struct scull_data    *next = NULL;
    struct scull_data    *ptr = NULL;

    ptr = data;
    while (ptr) {
	next = ptr->next;
	free(ptr);
	ptr = next;
    }
}

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

	atomic_set(&dev->nitems, 0)
    }

    return 0;
}

/*
 *  clear all data in scull_dev
 */
static void scull_dev_clear(struct scull_dev *dev)
{
    struct scull_data   *ptr = NULL;

    if (dev) {
	dev->size = 0;
	atomic_set(&dev->nitems, 0)

	/* free data */
	scull_data_clear(dev->data);
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
    struct scull_data *data = dev->data;
    int               nblk = 0;
    int               n = 0;
    int               m = 0;
    int               i;
    
    if (!dev) {
	nreads = -EFAULT;
	goto out;
    }

    /* out of range */
    if ( (long)*offset >= dev->size)
	goto out;
    
    /* how many bytes */
    if ( (long)*offset + len >= dev->size)
	nreads = dev->size - (long)*offset;
    else
	nreads = len;

    nblk = (long)(*offset) / SCULL_QUANTUM;
    n = (long)(*offset) % SCULL_QUANTUM;
    
    /* move to start block */
    for (i = 0; i < nblk; i++)
	data = data->next;

    /* copy first block */
    if (n + nreads <= SCULL_QUANTUM) {
	if (copy_to_user(buf, data + n, nreads)) {
	    printk(KERN_ERR "copy bytes to user space failor\n");
	    nreads = -EFAULT;
	    goto out;
	}
	m += nreads;
    }
    else {
	if (copy_to_user(buf, data + n, SCULL_QUANTUM - n)) {
	    printk(KERN_ERR "copy bytes to user space failor\n");
	    goto out;
	}
	m += SCULL_QUANTUM - n;
    }

    /* left data */
    while (m < nreads) {
	data = data->next;

	/* only this block */
	if (nreads - m <= SCULL_QUANTUM) {
	    if (copy_to_user(buf, data, nreads - m)) {
		printk(KERN_ERR "copy bytes to user space failor\n");
		nreads = -EFAULT
		goto out;
	    }
	    m += nreads - m;
	}
	/* have more block */
	else {
	    if (copy_to_user(buf, data, SCULL_QUANTUM)) {
		printk(KERN_ERR "copy bytes to user space failor\n");
		nreads = -EFAULT;
		goto out;
	    }
	    m += SCULL_QUANTUM;
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
    struct scull_data *data = dev->data;
    int               nblk = 0;
    int               n = 0;
    int               m = 0;
    int               i = 0;

    if (!dev) {
	nwrites = -EFAULT;
	goto out;
    }

    if ( (long)*offset > size) {
	nwrites = -EINVAL;
	goto out;
    }

    if (len == 0)
	goto out;

    /* calc block and start */
    nblk = (long)*offset / SCULL_QUANTUM;
    n = (long)*offset % SCULL_QUANTUM;

    /* reach previous block of block that need write */
    for (i = 0; i < nblk - 1; i++)
	data = data->next;

    /* we have no room, alloced */
    if (!data->next) {
	data->next = kmalloc(sizeof(struct scull_data), GFP_KERNEL);
	memset(data->next, 0, sizeof(struct scull_data));
    }
    data = data->next;
	
    

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
