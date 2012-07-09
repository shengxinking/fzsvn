/*
 *  test GPL licence and non-GPL licence EXPORT_SYMBOL
 *
 *  write by Forrest.zhang
 */

#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("nonGPL");

extern void test_gpl(void);

static void test_nongpl(void)
{
    printk("test_nongpl\n");
    test_gpl();
}



static int __init testnongpl_init(void)
{
    printk("Initiate testnongpl OK\n");
    test_nongpl();
    return 0;
}

static void __exit testnongpl_exit(void)
{
    printk("Destroy testnongpl OK\n");
}

module_init(testnongpl_init);
module_exit(testnongpl_exit);


