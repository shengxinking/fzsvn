/*
 *  char device support atomic variable
 *
 *  write by Forrest.zhang
 */

#define __KDEBUG__                     1

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/rwsem.h>
#include "../kdebug.h"

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Forrest.zhang");

#define SCULL_NAME                "scull"
#define SCULL_BUFSIZ              1024
#define SCULL_MAJOR               168

struct scull_data {
    char                 buff[SCULL_BUFSIZ];
    atomic_t             size;
    struct rw_semaphore  rwsem;
};

static struct scull_data     scull_data = {
    .buff   = {0},
    .size   = ATOMIC_INIT(0),
};

static int scull_open(struct inode *inode, struct file *file)
{
    if (try_module_get(THIS_MODULE) < 0) {
	ERR("modules not exist\n");
	return -1;
    }

    file->private_data = &scull_data;
    
    if ( (file->f_flags & O_ACCMODE) == O_WRONLY) {
	DBG("open O_WRONLY\n");
	memset(scull_data.buff, 0, sizeof(scull_data.buff));
	atomic_set(&scull_data.size, 0);
    }

    if ( (file->f_flags & O_ACCMODE) == O_APPEND) {
	DBG("open O_APPEND\n");
	file->f_pos = atomic_read(&scull_data.size);
    }

    DBG("open success\n");

    return 0;
}

static int scull_release(struct inode *inode, struct file *file)
{
    module_put(THIS_MODULE);

    file->private_data = NULL;

    DBG("release success\n");

    return 0;
}

static ssize_t scull_read(
    struct file         *file,
    char __user         *buff,
    size_t              len,
    loff_t              *offset)
{
    int                 nreads = 0;
    struct scull_data   *data = file->private_data;
    
    if (atomic_read(&data->size) <= 0)
	goto out;

    if ((long)*offset >= atomic_read(&data->size))
	goto out;

    down_read(&data->rwsem);

    if ((long)*offset + len > atomic_read(&data->size))
	nreads = atomic_read(&data->size) - (long)*offset;
    else
	nreads = len;

    if (copy_to_user(buff, data->buff + (long)*offset, nreads)) {
	nreads = -EFAULT;
	goto out_free;
    }

    *offset += nreads;
    
  out_free:
    up_read(&data->rwsem);

  out:
    return nreads;
}


static ssize_t scull_write(
    struct file         *file,
    const char __user   *buff,
    size_t              len,
    loff_t              *offset)
{
    int                 nwrites = 0;
    struct scull_data   *data = file->private_data;
    
    if ((long)*offset >= SCULL_BUFSIZ)
	goto out;

    if ((long)*offset + len > SCULL_BUFSIZ)
	nwrites = SCULL_BUFSIZ - (long)*offset;
    else
	nwrites = len;

    down_write(&data->rwsem);

    if (copy_from_user(data->buff + (long)*offset, buff, nwrites)) {
	nwrites = -EFAULT;
	goto out_free;
    }

    *offset += nwrites;
    if ((long)*offset > atomic_read(&data->size))
	atomic_set(&data->size, (int)*offset);

  out_free:
    up_write(&data->rwsem);

  out:
    return nwrites;
}

static struct file_operations scull_fops = {
    .open    = scull_open,
    .release = scull_release,
    .read    = scull_read,
    .write   = scull_write,
};

static int __init scull_init(void)
{
    if (register_chrdev(SCULL_MAJOR, SCULL_NAME, &scull_fops)) {
	ERR("can't register %s on %d\n", SCULL_NAME, SCULL_MAJOR);
	return -1;
    }
    
    init_rwsem(&scull_data.rwsem);

    DBG("init success\n");

    return 0;
}

static void __exit scull_exit(void)
{
    unregister_chrdev(SCULL_MAJOR, SCULL_NAME);

    DBG("exit success\n");
}

module_init(scull_init);
module_exit(scull_exit);


    
