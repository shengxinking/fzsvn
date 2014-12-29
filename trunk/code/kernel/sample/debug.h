/*
 *  defined two debug macro for debug kernel module
 *
 *  write by Forrest.zhang
 */

#ifndef __DEBUG_H__

#define ERR(fmt, args...)          printk(KERN_ERR "[ERROR]: " fmt, ##args)

#if defined(_DEBUG_)
#   define DBG(fmt, args...)       printk(KERN_DEBUG "[INFO]: " fmt, ##args)
#else
#   define DBG(fmt, args...)
#endif


#endif /* end of __KDEBUG_H__ */
