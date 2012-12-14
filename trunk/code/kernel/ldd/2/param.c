/*
 *  module parameter test
 *
 *  write by Forrest.zhang
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/moduleparam.h>

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Forrest.zhang");

static int   i = 0;
static int   iarr[4] = {0};
static char* s = "";
static char* sarr[4] = {"", "", "", ""};
static int   iarr_size = 2;
static int   sarr_size = 3;

MODULE_PARM_DESC(i, "integer value");
MODULE_PARM_DESC(s, "charater string");

module_param(i, int, S_IRUGO);
module_param_array(iarr, int, &iarr_size, S_IRUGO);
module_param(s, charp, S_IRUSR);
module_param_array(sarr, charp, &sarr_size, S_IRUSR);

static int __init param_init(void)
{
    printk(KERN_ALERT "you input i = %d\n", i);
    printk(KERN_ALERT "you input s = %s\n", s);
    printk(KERN_ALERT "you input %d interge, iarr = [%d, %d, %d, %d]\n", 
	   iarr_size, iarr[0], iarr[1], iarr[2], iarr[3]);
    printk(KERN_ALERT "you input %d string, sarr = [%s, %s, %s, %s]\n", 
	   sarr_size, sarr[0], sarr[1], sarr[2], sarr[3]);

    return 0;
}

static void __exit param_exit(void)
{
    printk(KERN_ALERT "Bye param\n");
}

module_init(param_init);
module_exit(param_exit);
