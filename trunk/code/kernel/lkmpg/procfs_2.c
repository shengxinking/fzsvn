/*
 *
 *
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/init.h>
#include <asm/uaccess.h>

#define PROCFS_NAME       "procbuffer"
#define PROCFS_BUFSIZ     1024

static char   procfs_buf[PROCFS_BUFSIZ] = {0};
static int    procfs_siz = 0;

static int procfs_read(char*   buf, 
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
	memcpy(buf, procfs_buf, procfs_siz);
	ret = procfs_siz;
    }

    *eof = 1;

    return ret;
}

static int procfs_write(struct file*       fp,
			const char* __user buf,
			unsigned long      count,
			void*              data)
{
    procfs_siz = count;
    if (procfs_siz > PROCFS_BUFSIZ)
	procfs_siz = PROCFS_BUFSIZ;

    if (copy_from_user(procfs_buf, buf, procfs_siz))
	return -EFAULT;

    return procfs_siz; 
}


static int __init procfs_init(void)
{
    struct proc_dir_entry*     procfs_entry = 0;

    procfs_entry = create_proc_entry(PROCFS_NAME, 0644, NULL);
    if (procfs_entry == NULL) {
	printk("kernel: register procfs %s error\n", PROCFS_NAME);
	remove_proc_entry(PROCFS_NAME, NULL);
	return -ENOMEM;
    }

    procfs_entry->read_proc = procfs_read;
    procfs_entry->write_proc = procfs_write;
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
