/*
 *  test GPL licence and non-GPL licence EXPORT_SYMBOL
 *
 *  write by Forrest.zhang
 */

#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL ");

static void test_gpl(void)
{
    printk("test_gpl\n");
}



static int __init testgpl_init(void)
{
    printk("Initiate testgpl OK\n");
    return 0;
}

static void __exit testgpl_exit(void)
{
    printk("Destroy testgpl OK\n");
}

module_init(testgpl_init);
module_exit(testgpl_exit);

EXPORT_SYMBOL_GPL(test_gpl);

