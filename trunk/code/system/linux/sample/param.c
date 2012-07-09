/*
 *  test module parameter of linux-2.6
 *
 *  write by forrest
 */


#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

MODULE_LICENSE("GPL");

static int   ntimes = 1;
static char* whos[5] = {"world", 0, 0, 0, 0};
static int   nwhos = 4;

module_param(ntimes, int, S_IRUGO);
module_param_array(whos, charp, &nwhos, S_IRUGO);

static int param_init(void)
{
    char**   ptr = whos;
    int      i;

    for (i = 0; i < ntimes; ++i) {
	printk("hello");
	while (*ptr) {
	    printk(", %s", *ptr);
	    ++ptr;
	}
	printk("\n");
	ptr = whos;
    }

    return 0;
}

static void param_exit(void)
{
    char**   ptr = whos;
    int      i;

    for (i = 0; i < ntimes; ++i) {
	printk("bye");
	while (*ptr) {
	    printk(", %s", *ptr);
	    ++ptr;
	}
	printk("\n");
	ptr = whos;
    }

}

module_init(param_init);
module_exit(param_exit);
