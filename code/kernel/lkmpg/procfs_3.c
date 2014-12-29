/*
 *  the proc filesystem using standard file_operations and inode_operations
 *
 *  write by Forrest.zhang
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

#define  PROC_NAME         "procbuff2k"
#define  PROC_BUF_MAX_SIZ  2048

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Forrest.zhang");
MODULE_DESCRIPTION("test proc filesystem");

static char    proc_buf[PROC_BUF_MAX_SIZ];
static int     proc_siz = 0;

static ssize_t 
procfs_read(struct file   *filep, 
	    char  __user  *buf, 
	    size_t        count, 
	    loff_t        *offset)
{
    ssize_t         nreads = 0;

    nreads = count;
    if (nreads > proc_siz - *offset)
	nreads = proc_siz - *offset;
    
    if (copy_to_user(buf, proc_buf + *offset, nreads))
	return -EFAULT;
    *offset += nreads;

    return nreads;
}

static ssize_t
procfs_write(struct file       *filep,
	     const char __user *buf,
	     size_t            count,
	     loff_t            *offset)
{
    ssize_t          nwrites = 0;

    nwrites = count;
    if (*offset + nwrites >= PROC_BUF_MAX_SIZ)
	nwrites = PROC_BUF_MAX_SIZ - *offset - 1;

    if (copy_from_user(proc_buf + *offset, buf, nwrites))
	return -EFAULT;

    *offset += nwrites;
    proc_siz += nwrites;

    return nwrites;
}

static int
procfs_open(struct inode* inodep,
	    struct file*  filep)
{
    static int  open_times = 1;

    printk("kernel: open %d times\n", open_times++);

    return 0;
}

static int
procfs_release(struct inode* inodep,
	       struct file*  filep)
{
    static int  release_times = 1;
    printk("kernel: release %d times\n", release_times++);

    return 0;
}


static struct file_operations fops = {
    .read = procfs_read,
    .write = procfs_write,
    .open = procfs_open,
    .release = procfs_release,
};


static int
procfs_permission(struct inode*     inodep,
		  int               op,
		  struct nameidata* nd)
{
    if ( (op == 4) || (op == 2 && current->euid == 0) )
	return 0;

    return -EACCES;
}

static struct inode_operations iops = {
    .permission = procfs_permission,
};

static int __init procfs_init(void)
{
    struct proc_dir_entry*      procfs;

    procfs = create_proc_entry(PROC_NAME, 0644, NULL);
    if (!procfs) {
	printk("kernel: create proc entry %s error\n", PROC_NAME);
	return -EINVAL;
    }

    procfs->owner = THIS_MODULE;
    procfs->proc_iops = &iops;
    procfs->proc_fops = &fops;
    procfs->mode = S_IFREG | S_IRUGO | S_IWUSR;
    procfs->uid = 0;
    procfs->gid = 0;
    procfs->size = 80;

    return 0;
}

static void __exit procfs_exit(void)
{
    remove_proc_entry(PROC_NAME, &proc_root);
}

module_init(procfs_init);
module_exit(procfs_exit);
	

