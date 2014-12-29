/*
 *  map a char device to user space
 *
 *  write by Forrest.zhang
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Forrest.zhang");

#define MMAP_NAME                "chardev"
#define MMAP_MAJOR               168
#define MMAP_MINOR               0

#define _DEBUG_                  1

#if defined(_DEBUG_)
#   define DBG(fmt, args...)     printk(KERN_DEBUG "mmap_dbg: " fmt, ## args) 
#else
#   define DBG(fmt, args...)
#endif

#define ERR(fmt, args...)        printk(KERN_ERR "mmap_err: " fmt, ## args)

static mmap_open()
{
}

static mmap_release()
{
}

static mmap_read()
{
}

static mmap_write()
{
}

static mmap_mmap()
{
}


static int __init mmap_init()
{
}

static void __exit mmap_exit()
{
}


