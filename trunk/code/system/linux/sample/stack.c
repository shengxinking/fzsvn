/*
 *  test kernel stack is only two page in version 2.6
 *
 *  write by Forrest.zhang in Fortinet Inc.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>

/*
 *  a recursive function to test kernel stack's size
 */

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Forrest");

static void test_stack(unsigned int level)
{
    int     arr[1024];
    int     i;
    
    if (level <= 0)
	return;
    
    for (i = 0; i < 1024; i++)
	arr[i] = i * level;

    test_stack(level - 1);
}

static int __init stack_init(void)
{
    printk("init stack module\n");

    test_stack(4);
    
    return 0;
}

static void __exit stack_exit(void)
{
    printk("destroy stack module\n");
}

module_init(stack_init);
module_exit(stack_exit);

