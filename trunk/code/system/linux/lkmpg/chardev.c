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

#define CHARDEV_NAME                 "chardev"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Forrest zhang");
MODULE_DESCRIPTION("a simple character device module, not refer a phisical device");

static int   chardev_major = 0;
static int   use_count = 0;
static int   in_usage = 0;
static char  buf[512] = {0};
static char* ptr = 0;

static int chardev_open(struct inode* nd, struct file* fp)
{
    if (in_usage) {
	printk("kernel: %s in usage\n", CHARDEV_NAME);
	return -EBUSY;
    }

    ++in_usage;
    ++use_count;
    sprintf(buf, "kernel: %s opened %d times\n", CHARDEV_NAME, use_count);
    ptr = buf;

    return 0;
}

static int chardev_release(struct inode* nd, struct file* fp)
{
    --in_usage;
    ptr = 0;

    return 0;
}

static ssize_t chardev_read(struct file* fp, 
			    char* __user buf, 
			    size_t len, 
			    loff_t* offset)
{
    ssize_t       nreads = 0;

    if (*ptr == 0)
	return 0;

    while (len > 0 && *ptr) {
	put_user(*(ptr++), buf++);
	--len;
	++nreads;
    }

    return nreads;
}

static ssize_t chardev_write(struct file* fp, 
			     const char* __user buf, 
			     size_t len, 
			     loff_t* offset)
{
    printk("kernel: %s unsupport write\n", CHARDEV_NAME);
    return -EINVAL;
}

static struct file_operations fops = {
    open:    chardev_open,
    release: chardev_release,
    read:    chardev_read,
    write:   chardev_write
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
