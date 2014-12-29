/*
 *
 *
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("hello_5, test module parameter");
MODULE_AUTHOR("Forrest.zhang");

#define    my_array_size       4

static char  my_char = 'a';
static short my_short = -1;
static int   my_int = -1;
static long  my_long = -1;
static char* my_string = "s";
static char  my_char_arr[my_array_size] = {'a', 'a', 'a', 'a'};
static short my_short_arr[my_array_size] = {-1, -1, -1, -1};
static int   my_int_arr[my_array_size] = {-1, -1, -1, -1};
static long   my_long_arr[my_array_size] = {-1, -1, -1, -1};
static char* my_string_arr[my_array_size] = {"s", "s", "s", "s"};

MODULE_PARM_DESC(my_char, "a byte parameter");
MODULE_PARM_DESC(my_short, "a short parameter");
MODULE_PARM_DESC(my_int, "a integer parameter");
MODULE_PARM_DESC(my_long, "a long integer parameter");
MODULE_PARM_DESC(my_string, "a string parameter");
MODULE_PARM_DESC(my_char_arr, "a char array parameter");
MODULE_PARM_DESC(my_short_arr, "a short array parameter");
MODULE_PARM_DESC(my_int_arr, "a integer array parameter");
MODULE_PARM_DESC(my_long_arr, "a long integer array parameter");
MODULE_PARM_DESC(my_string_arr, "a string array parameter");

MODULE_PARM(my_char, "b");
MODULE_PARM(my_short, "h");
MODULE_PARM(my_int, "i");
MODULE_PARM(my_long, "l");
MODULE_PARM(my_string, "s");
MODULE_PARM(my_char_arr, "0-1b");
MODULE_PARM(my_short_arr, "1-4h");
MODULE_PARM(my_int_arr, "3-4i");
MODULE_PARM(my_long_arr, "2-4l");
MODULE_PARM(my_string_arr, "3-3s");

static int __init hello_init(void)
{
    int          i;

    printk("Hello, world\n");
    printk("module parameters:\n");
    printk("my_char: %c\n", my_char);
    printk("my_short: %d\n", my_short);
    printk("my_int: %d\n", my_int);
    printk("my_long: %ld\n", my_long);
    printk("my_string: %s\n", my_string);

    printk("my_char_arr: ");
    for (i = 0; i < my_array_size; i++)
	printk("%c   ", my_char_arr[i]);
    printk("\n");

    printk("my_short_arr: ");
    for (i = 0; i < my_array_size; i++)
	printk("%d   ", my_short_arr[i]);
    printk("\n");

    printk("my_int_arr: ");
    for (i = 0; i < my_array_size; i++)
	printk("%d   ", my_int_arr[i]);
    printk("\n");

    printk("my_long_arr: ");
    for (i = 0; i < my_array_size; i++)
	printk("%ld   ", my_long_arr[i]);
    printk("\n");

    printk("my_string_arr: ");
    for (i = 0; i < my_array_size; i++)
	printk("%s   ", my_string_arr[i]);
    printk("\n");

    return 0;
}

static void __exit hello_exit(void)
{
    printk("Goodbye, world\n");
}

module_init(hello_init);
module_exit(hello_exit);
