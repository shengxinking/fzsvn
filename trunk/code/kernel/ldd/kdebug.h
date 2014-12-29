/*
 *  debug information output MACRO
 *
 *  write by Forrest.zhang
 */

#ifndef __KDEBUG_H__
#define __KDEBUG_H__

#include <linux/kernel.h>

#if defined(__KDEBUG__)
#   define DBG(fmt, args...)    printk(KERN_DEBUG "info: " fmt, ## args)
#else
#   define DBG(fmt, args...)
#endif

#define ERR(fmt, args...)       printk(KERN_ERR "err: " fmt, ## args)

#endif /* end of __KDEBUG_H__ */



