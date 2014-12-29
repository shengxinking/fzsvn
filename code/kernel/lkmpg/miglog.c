/*
 *   miglog character device driver, it's used to store FBL logs.
 *   when a FBL program want log a message, it call fgt_text to send
 *   log to fbllog device, and daemon miglogd can wakeup and get the 
 *   message.
 *
 *   Port from 2.4 to 2.6 by Forrest.zhang in Fortinet Inc.
 *
 *   Copyright:  GPL v2
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>

#define MIGLOG_DEVNUM     155      /* miglog device minor number */
#define MIGLOG_DEVNAM     "miglog" /* miglog device name */


static int miglog_open(struct inode *inodep, struct file *filep)
{
    return 0;
}

static int miglog_release(struct inode *inodep, struct file* filep)
{
    return 0;
}

static ssize_t miglog_read(
    struct file     *filep,
    char __user     *buf,
    size_t           len,
    loff_t          *offset)
{
    return 0;
}

static ssize_t miglog_write(
    struct file       *filep,
    const char __user *buf,
    size_t             len,
    loff_t            *offset)
{
    return 0;
}

static struct file_operations miglog_ops = {
    .open    = miglog_open,
    .release = miglog_release,
    .read    = miglog_read,
    .write   = miglog_write,
};

static struct miscdevice miglog_dev = {
    .minor      = MIGLOG_DEVNUM,
    .name       = MIGLOG_DEVNAM,
    .fops       = &miglog_ops,
};

static int __init miglog_init(void)
{
    if (misc_register(&miglog_dev) < 0) {
	printk("register miglog device error\n");
	return -EFAULT;
    }

    printk("miglog initiation done.\n");

    return 0;
}

static void __exit miglog_exit(void)
{
    /* register miglog as misc device */
    if (misc_deregister(&miglog_dev) < 0) {
	printk("unregister miglog device error\n");
    }
}

module_init(miglog_init);
module_exit(miglog_exit);
