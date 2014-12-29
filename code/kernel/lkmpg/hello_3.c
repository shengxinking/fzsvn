/*
 *
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

static int hello_data __initdata = 3;

static int __init hello_init(void)
{
    printk("Hello, world. init data is %d\n", hello_data);
    
    return 0;
}

static void __exit hello_exit(void)
{
    printk("Goodbye, worlds");
}

module_init(hello_init);
module_exit(hello_exit);



