/**
 *	@file	bonding_util.c
 *
 *	@brief	Bonding interface APIs for 802.3ad ethernet aggregation.
 *	
 *	@author	Forrest.zhang
 *
 *	@date	2013-10-29
 */

#ifndef FZ_BONDING_UTIL
#define FZ_BONDING_UTIL

#include <net/if.h>

#define	BONDING_MAX_SLAVE	10

/**
 * 	the follow macro is copy from <linux/if_bonding.h>, 
 * 	we can't include it in userspace program(compile failed), 
 * 	so copied it. 
 */
#define BOND_MODE_ROUNDROBIN    0
#define BOND_MODE_ACTIVEBACKUP  1
#define BOND_MODE_XOR           2
#define BOND_MODE_BROADCAST     3
#define BOND_MODE_8023AD        4
#define BOND_MODE_TLB           5
#define BOND_MODE_ALB           6 /* TLB + RLB (receive load balancing) */

/* hashing types */
#define BOND_XMIT_POLICY_LAYER2         0 /* layer 2 (MAC only), default */
#define BOND_XMIT_POLICY_LAYER34        1 /* layer 3+4 (IP ^ (TCP || UDP)) */
#define BOND_XMIT_POLICY_LAYER23        2 /* layer 2+3 (IP ^ MAC) */

/* end of copy from <linux/if_bonding.h> */


typedef struct bonding_arg {
	int		mode;		/* aggregate mode */
	int		xmit_hash_policy;/* Loadbalance policy */
	int		lacp_rate;	/* LACP rate */ 
	int		miimon;		/* MII monitor rate */
	char		slaves[BONDING_MAX_SLAVE][IFNAMSIZ];/* slaves */
	int		nslave;		/* number of slaves */
} bonding_arg_t;

/**
 *	Check bonding device @name is exist or not.
 *
 *	Return 1 if exist, 0 not exist, -1 on error.
 */
extern int 
bonding_is_exist(const char *name);

/**
 *	Add a new bonding device @ifname. the bonding 
 *	device parameter store in @arg.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
bonding_add(const char *ifname, const bonding_arg_t *arg);

/**
 *	Modify bonding device @ifname using @arg
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
bonding_modify(const char *name, const bonding_arg_t *arg);

/**
 *	Delete bonding device @ifname.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
bonding_del(const char *name);

/**
 *	Get bonding device @ifname's parameter and
 *	stored in @arg.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
bonding_get_arg(const char *name, bonding_arg_t *arg);

/**
 *	Get the bonding device number in system.
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
bonding_get_number(void);

/**
 *	Get all bonding device in system. the 
 *	all bonding devices name stored in @buf. 
 *	The @buf length is @len. If the @buf space
 *	is not enough store all names, return NULL.
 *
 *	Return all device names if success, NULL on error. 
 */
extern char ** 
bonding_get_all(char *buf, size_t len);

/**
 *	Delete all bonding device in system.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
bonding_del_all(void);

/**
 *	Get the error message when bonding_xxxx() failed.
 *
 * 	Return string of error message.
 */
extern const char * 
bonding_get_error(void);

#endif /* end of FZ_BONDING_UTIL  */


