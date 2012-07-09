/*
 *  test module of linux-2.6
 *
 *  write by forrest
 */


#include <linux/init.h>
#include <linux/module.h>

MODULE_LICENSE("GPL");

int test_count;

EXPORT_SYMBOL(test_count);

static int test_init(void)
{
    printk("test module init\n");
    ++test_count;
    return 0;
}

static void test_exit(void)
{
    printk("test module exit\n");
}

module_init(test_init);
module_exit(test_exit);
