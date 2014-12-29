/*
 *  a kernel thread program
 *
 *  write by Forrest.zhang
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/jiffies.h>

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Forrest.zhang");

#define THREAD_NAME       "[Forrest]"
#define _DEBUG_           1

#define ERR(fmt, args...)          printk(KERN_ERR "kthread: " fmt, ##args)

#if defined(_DEBUG_)
#   define DBG(fmt, args...)       printk(KERN_DEBUG "kthread: " fmt, ##args)
#else
#   define DBG(fmt, args...)
#endif

static int kthread_sec = 60;

static int kthread_main(void *data)
{
    int              n = *((int *)data);
    unsigned long    expire = jiffies + 60 * HZ;

    DBG("in [%d][%s] %d second\n", current->pid, __FUNCTION__, n);

    memcpy(current->comm, THREAD_NAME, 10);
    
    while (time_before(jiffies, expire))
	;

    DBG("command name is %s\n", current->comm);

    return 0;
}

static int kthread_first(void *data)
{
    pid_t        pid;
 
    DBG("in %s, run %d second\n", __FUNCTION__, *((int *)data));

    if ( (pid = kernel_thread(kthread_main, data, 0)) < 0) {
	ERR("create second thread error\n");
	return -1;
    }

    return 0;
}

static int __init kthread_init(void)
{
    pid_t          pid;

    DBG("init start, run %d second\n", kthread_sec);

    if ( (pid = kernel_thread(kthread_first, &kthread_sec, 0)) < 0) {
	ERR("create thread error\n");
	return -1;
    }

    DBG("init success\n");

    return 0;
}

static void __exit kthread_exit(void)
{
    DBG("exit success\n");
}

module_init(kthread_init);
module_exit(kthread_exit);
