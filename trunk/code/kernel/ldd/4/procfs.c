/*
 *  a simple procfs entry in kernel 2.6
 *
 *  write by Forrest.zhang
 */


#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Forrest.zhang");

#define PROC_NAME            "hello"
#define PROC_MODE            0666
#define PROC_MAXSIZ          1024
#define _DEBUG_              1

#if defined (_DEBUG_)
#   define DBG(fmt, args...)     printk(KERN_ALERT "proc: " fmt, ##args)
#else
#   define DBG(fmt, args...)
#endif

static int proc_read(
    char        *page,
    char        **start,
    off_t       offset,
    int         count,
    int         *eof,
    void        *data)
{
    int         nreads = 0;

    nreads = sprintf(page, "hello world from Forrest.zhang by [%s][%d]\n",
		     current->comm, current->pid);

    
    *eof = 1;
    
    return nreads;
}

static int proc_write(
    struct file         *file,
    const char __user   *buf,
    unsigned long       count,
    void                *date)
{
    int       nwrites = 0;
    char      wbuf[PROC_MAXSIZ + 1] = {0};

    if (count >= PROC_MAXSIZ)
	nwrites = PROC_MAXSIZ;
    else
	nwrites = count;

    if (copy_from_user(wbuf, buf, nwrites)) {
	printk(KERN_ERR "copy data from user error\n");
	nwrites = -EFAULT;
	goto out;
    }

    wbuf[PROC_MAXSIZ] = 0;
    DBG("receive %d bytes: %s\n", nwrites, wbuf);
    
  out:
    return nwrites;
}
    

static int __init proc_init(void)
{
    struct proc_dir_entry     *entry = NULL;

    /* create entry in proc fs */
    entry = create_proc_entry(PROC_NAME, PROC_MODE, NULL);
    if (!entry) {
	printk(KERN_ERR "can't create %s in procfs\n", PROC_NAME);
	return -1;
    }

    entry->read_proc = proc_read;
    entry->write_proc = proc_write;

    DBG("create proc entry %s success\n", PROC_NAME);

    return 0;
}


static void __exit proc_exit(void)
{
    /* remove proc entry */
    remove_proc_entry(PROC_NAME, NULL);
}

module_init(proc_init);
module_exit(proc_exit);
