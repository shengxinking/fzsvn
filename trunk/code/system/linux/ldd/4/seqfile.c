/*
 *  a simple seqfile module in kernel 2.6
 *
 *  write by Forrest.zhang
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>

#define SEQ_NUM         4
#define SEQ_SIZ         12
#define PROC_MODE       0666
#define PROC_NAME       "seqfile"
#define _DEBUG_         1

#if defined(_DEBUG_)
#   define DBG(fmt, args...)          printk(KERN_ALERT "seq: " fmt, ##args)
#else
#   define DBG(fmt, args...)
#endif

static char  seq[SEQ_NUM][SEQ_SIZ + 1] = {"hello", "jerry", "tom", "forrest"};

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Forrest.zhang");

static void *seq_start(struct seq_file *file, loff_t *pos)
{
    if (*pos >= SEQ_NUM)
	return NULL;
    
    return seq[*pos];
}

static void *seq_next(struct seq_file *file, void* v, loff_t *pos)
{
    (*pos)++;
    if (*pos >= SEQ_NUM)
	return NULL;
    
    return seq[*pos];
}

static void seq_stop(struct seq_file *file, void *v)
{
}

static int seq_show(struct seq_file *file, void* v)
{
    char    *ptr = v;
    seq_printf(file, "%s\n", ptr);
    
    return 0;
}

static struct seq_operations seq_ops = {
    .start = seq_start,
    .next  = seq_next,
    .stop  = seq_stop,
    .show  = seq_show,
};

static int seq_proc_open(struct inode *inode, struct file *file)
{
    return seq_open(file, &seq_ops);
}

static struct file_operations proc_fops = {
    .owner   = THIS_MODULE,
    .open    = seq_proc_open,
    .release = seq_release,
    .read    = seq_read,
    .llseek   = seq_lseek,
};

static int __init proc_init(void)
{
    struct proc_dir_entry    *entry = NULL;

    /* create entry in /proc */
    entry = create_proc_entry(PROC_NAME, PROC_MODE, NULL);
    if (!entry) {
	printk(KERN_ERR "create proc entry %s error\n", PROC_NAME);
	return -1;
    }

    entry->proc_fops = &proc_fops;

    DBG("init success\n");

    return 0;
}

static void __exit proc_exit(void)
{
    remove_proc_entry(PROC_NAME, NULL);
}

module_init(proc_init);
module_exit(proc_exit);
