/*
 *  this code implement a simple char device in kernel 2.6
 *
 *  write by Forrest.zhang for keep hand feeling of write C code
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <asm/ioctl.h>

#define IOCTL_SET_MSG                0xeeff01
#define IOCTL_GET_MSG                0xeeff02
#define IOCTL_CLR_MSG                0xeeff03

#define CHARDEV_NAME                 "chardev"
#define CHARDEV_BUF_SIZ              512


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Forrest zhang");
MODULE_DESCRIPTION("a simple character device module, not refer a phisical device");

static int   chardev_major = 0;
static char  dev_buf[CHARDEV_BUF_SIZ] = {0};
static int   dev_siz = 0;

static int 
chardev_open(struct inode* nd, struct file* fp)
{
    static int usage = 1;

    printk("kernel: %s opened %d times\n", CHARDEV_NAME, usage++);

    return 0;
}

static int 
chardev_release(struct inode* nd, struct file* fp)
{
    static int usage = 1;

    printk("kernel: %s release %d times\n", CHARDEV_NAME, usage++);
    
    return 0;
}

static ssize_t 
chardev_read(struct file* fp, 
	     char* __user buf, 
	     size_t len, 
	     loff_t* offset)
{
    int           off = 0;

    if (offset)
	off = *offset;

    // most dev_size chars
    if (len + off >= dev_siz)
	len = dev_siz - off;

    // out of ranger
    if (len <= 0)
	return 0;

    // copy data
    if (copy_to_user(buf, dev_buf + off, len))
	return -EFAULT;

    if(offset)
	*offset += len;

    return len;
}

static ssize_t 
chardev_write(struct file* fp, 
	      const char* __user buf, 
	      size_t len, 
	      loff_t* offset)
{
    int           off = 0;
    
    if (offset)
	off = *offset;

    if (len + off > CHARDEV_BUF_SIZ)
	len = CHARDEV_BUF_SIZ - off;

    if (len <= 0)
	return -ENOSPC;

    if (copy_from_user(dev_buf + off, buf, len))
	return -EFAULT;

    if (offset)
	*offset += len;

    dev_siz = off + len;

    return len;
}

static int 
chardev_ioctl(struct inode* inodep,
	      struct file*  filep,
	      unsigned int  cmd,
	      unsigned long param)
{
    char*     temp = 0;
    int       len;
    char      ch;
    int       i;

    switch (cmd) {
    case IOCTL_SET_MSG:
	temp = (char*)param;

	get_user(ch, temp);
	for (i = 0;  ch && i < CHARDEV_BUF_SIZ; i++, temp++)
	    get_user(ch, temp);

	temp = (char*)param;
	len = chardev_write(filep, temp, i, 0);

	dev_buf[dev_siz - 1] = 0;
	
	return len;

    case IOCTL_GET_MSG:
	temp = (char*)param;
	
	if (copy_to_user(temp, dev_buf, dev_siz))
	    return -EFAULT;

	return 0;
    case IOCTL_CLR_MSG:
	temp = (char*)param;
	
	if (copy_to_user(temp, dev_buf, dev_siz))
	    return -EFAULT;

	dev_siz = 0;

	return 0;
    default:
	printk("kernel: unsupport command: %d\n", cmd);
	return -EINVAL;
    }
}

static struct file_operations fops = {
    open:    chardev_open,
    release: chardev_release,
    read:    chardev_read,
    write:   chardev_write,
    ioctl:   chardev_ioctl,
};

static int __init chardev_init(void)
{
    chardev_major = register_chrdev(0, CHARDEV_NAME, &fops);

    if (chardev_major < 0) {
	printk("kernel: register %s error\n", CHARDEV_NAME);
	return chardev_major;
    }
    
    printk("kernel: register %s major number %d\n", CHARDEV_NAME, chardev_major);

    return 0;
}

static void __exit chardev_exit(void)
{
    int   ret;
    ret = unregister_chrdev(chardev_major, CHARDEV_NAME);

    if (ret < 0)
	printk("kernel: unregister %s error\n", CHARDEV_NAME);
}

module_init(chardev_init);
module_exit(chardev_exit);
