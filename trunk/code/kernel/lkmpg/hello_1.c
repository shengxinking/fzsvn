/*
 *  hello-1.c 
 *  the first caveman programmer chiseled the first
 *  program on the walls of the first cave computer, it was a
 *  program to paint the string "Hello, world" in Antelope 
 *  pictures
 */

#include <linux/module.h>
#include <linux/kernel.h>

int init_module(void)
{
    printk("Hello, world\n");

    return 0;
}

void cleanup_module(void)
{
    printk("Goodbye, world\n");
}

