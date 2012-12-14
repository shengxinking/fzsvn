/*
 *  the syscall replace module
 *
 */

#if 0

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <asm/uaccess.h>
#include <linux/unistd.h>
#include <linux/sched.h>

extern void* sys_call_table[];
static  int uid = 0;
asmlinkage int (*orig_call)(char*, int, int);

module_param(uid, int, 0644);

asmlinkage int my_syscall(char* filename, int flag, int mode)
{
    int          i = 0;
    char         ch;

    if (uid == current->uid) {
	printk("kernel: my_syscall \n");
	do {
	    get_user(ch, filename + i);
	    printk("%c", ch);
	} while (ch != 0);
	printk("\n");
    }

    return orig_call(filename, flag, mode);
}

static int __init my_init(void)
{
    printk("kernel: replace open with my_open\n");

    orig_call = sys_call_table[__NR_open];
    sys_call_table[__NR_open] = my_syscall;

    return 0;
}

static void __exit my_exit(void)
{
    printk("kernel: replace my_open with open\n");
    
    if (sys_call_table[__NR_open] != my_syscall) {
	printk("kernel: others changed syscall open.\n");
	printk("kernel: the kernel maybe in unstable status\n");
    }

    sys_call_table[__NR_open] = orig_call;
}

module_init(my_init);
module_exit(my_exit);

#endif

#ifndef __KERNEL__
#define __KERNEL__
#endif

#ifndef MODULE
#define MODULE
#endif

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/unistd.h>
#include <linux/linkage.h>
#include <linux/syscalls.h>
#include <asm/current.h>

//extern void* sys_call_table[];
/*
   * After version 2.4.18, the symbol sys_call_table is not exported,
   * but you can get the address from the file System.map, Just like
   * this `grep sys_call_table /boot/System.map'.
   */
void** sys_call_table = (void **)0xc03835c0;
long (*orig_mkdir)(const char *path, int mode);

asmlinkage long hacked_mkdir(const char *path, int mode)
{
            struct task_struct *ts;

	            ts = current;
		            printk("sys_mkdir(%s, %d) is called by (uid = %d, pid = %d)\n",
				                            path, mode, ts->uid, ts->pid);
			            return orig_mkdir(path, mode);
}

static int hacked_init(void)
{
            orig_mkdir=sys_call_table[__NR_mkdir];
	            sys_call_table[__NR_mkdir]=hacked_mkdir;
		            return 0;
}

static void hacked_exit(void)
{
            sys_call_table[__NR_mkdir]=orig_mkdir; /*set mkdir syscall to the origal
						                                                                                             one*/
}

module_init(hacked_init);
module_exit(hacked_exit);

MODULE_LICENSE("GPL2");
MODULE_AUTHOR("xiaosuo@gmail.com"); 
