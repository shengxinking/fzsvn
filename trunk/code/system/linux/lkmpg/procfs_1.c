/*
 *
 *
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/init.h>

#define PROCFS_NAME       "helloworld"

int procfs_read(char*   buf, 
		char**  start,
		off_t   offset,
		int     count,
		int*    eof,
		void*   data
		)
{
    static int     ref_count = 0;
    int    ret;

    printk("kernel: read %s, need read %d, offset %d\n", PROCFS_NAME, count, (int)offset);

    if (offset > 0)
	ret = 0;
    else {
	++ref_count;
	ret = sprintf(buf, "kernel: you read %d times\n", ref_count);
    }

    *eof = 1;

    return ret;
}

static int __init procfs_init(void)
{
    struct proc_dir_entry* procfs_entry = NULL;

    procfs_entry = create_proc_entry(PROCFS_NAME, 0644, NULL);
    if (procfs_entry == NULL) {
	printk("kernel: register procfs %s error\n", PROCFS_NAME);
	remove_proc_entry(PROCFS_NAME, NULL);
	return -ENOMEM;
    }

    procfs_entry->read_proc = procfs_read;
    procfs_entry->owner = THIS_MODULE;
    procfs_entry->mode = S_IRUGO;
    procfs_entry->size = 512;
    procfs_entry->uid = 0;
    procfs_entry->gid = 0;

    return 0;
}

static void __exit procfs_exit(void)
{
    remove_proc_entry(PROCFS_NAME, NULL);
}

module_init(procfs_init);
module_exit(procfs_exit);
