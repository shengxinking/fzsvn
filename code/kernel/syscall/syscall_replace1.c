/*
 *	@file	syscall.c
 *
 *	@brief	Add a dynamic syscall to kernel.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2008-12-07
 */


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/unistd.h>

#include <linux/syscalls.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR  ("Forrest Zhang");

extern void *sys_call_table[];

asmlinkage long (*origin_sys_getcpu)(unsigned __user *cpu, unsigned __user *node, struct getcpu_cache __user *cache);

asmlinkage long sys_getcpu_replace(unsigned __user *cpu, unsigned __user *node, struct getcpu_cache __user *cache)
{
	printk(KERN_INFO "replace sys_getcpu\n");
	return origin_sys_getcpu(cpu, node, cache);
}


static int __init syscall_replace_init(void)
{
	printk(KERN_INFO "sys_getcpu address is 0x%lx\n", (unsigned long)(sys_call_table[__NR_getcpu]));

	origin_sys_getcpu = (void *)sys_call_table[__NR_getcpu];
	sys_call_table[__NR_getcpu] = (unsigned long *)&sys_getcpu_replace;

	return 0;
}


static void __exit syscall_replace_exit(void)
{
	printk("restore sys_call_table\n");
	sys_call_table[__NR_getcpu] = (unsigned long *)origin_sys_getcpu;

	printk(KERN_INFO "sys_getcpu address is 0x%lx\n", (unsigned long)(sys_call_table[__NR_getcpu]));
}

module_init(syscall_replace_init);
module_exit(syscall_replace_exit);

