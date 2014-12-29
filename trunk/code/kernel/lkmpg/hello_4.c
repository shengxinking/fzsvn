/*
 *
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("hello_4, test module attribute");
MODULE_AUTHOR("Forrest.zhang");

static int __init hello_init(void)
{
    printk("Hello, world\n");
    return 0;
}

static void __exit hello_exit(void)
{
    printk("Goodbye, world\n");
}

module_init(hello_init);
module_exit(hello_exit);










