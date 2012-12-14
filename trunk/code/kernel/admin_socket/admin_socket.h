
#ifndef _LINUX_ADMIN_H
#define _LINUX_ADMIN_H

#ifdef __KERNEL__
#include <linux/netdevice.h>
#else
#include <net/if.h>
#endif

#define AF_ADMIN		AF_ROSE	/* system admin socket.. */
#define PF_ADMIN		AF_ADMIN

struct sockaddr_admin
{
	int family;
	unsigned long pid;
	unsigned long group;
};

/* Event happen in the kernel, but application want to know,
IKE, HA, alertmail can listen kernel event to detect
their interested event */

#define AF_ADMIN_GROUP_IKE		(1<<0)
#define AF_ADMIN_GROUP_HA		(1<<1)
/*
	this group can be used to monitor the virtual system
	create/destory.
*/
#define AF_ADMIN_GROUP_VF		(1<<2)
/*to monitor dev enter/leave MANUAL/DHCP/PPPOE mode 
 monitor virtual device create or destory
*/
#define AF_ADMIN_GROUP_NETDEVICE	(1<<3) 
#define AF_ADMIN_GROUP_DHCPSERVER	(1<<4) 
#define AF_ADMIN_GROUP_ALERTMAIL	(1<<5) 
#define AF_ADMIN_GROUP_SYNC_IKE	        (1<<6)
#define AF_ADMIN_GROUP_TRAP_EVENT       (1<<7)

#define AF_ADMIN_GROUP_SCAN         (1<<8)
#define AF_ADMIN_GROUP_HTTP         (1<<9)
#define AF_ADMIN_GROUP_IKE_CMDB     (1<<10)
#define AF_ADMIN_GROUP_IPS          (1<<11)

/*     this group is for monitoring sys global option changes
 *
 */
#define AF_ADMIN_GROUP_SYS_GLOBAL   (1<<12)
#define AF_ADMIN_GROUP_CMDB_SVR     (1<<13)
#define AF_ADMIN_GROUP_HATALK       (1<<14)

//for modem daemon
#define AF_ADMIN_GROUP_SYS_MODEM     (1<<14)

#define AF_ADMIN_GROUP_PPTP_L2TP_CMDB     (1<<15)
#define AF_ADMIN_GROUP_PINGGEN_CMDB     (1<<16)
#define AF_ADMIN_GROUP_FSAE		(1<<17)
#define AF_ADMIN_GROUP_ZEBOS         (1<<18)

#define AF_ADMIN_GROUP_HACMD	    (1<<19)	 
struct af_message{
	unsigned long type;
	unsigned long len;
	unsigned char data[0];
};
#define AF_MSG_T_NONE		0
#define AF_MSG_T_IKE		1
#define AF_MSG_T_HA		2
#define AF_MSG_T_HA_SYNC_P1	3
#define AF_MSG_T_IF		4
#define AF_MSG_T_NIDS		5
#define AF_MSG_T_HATALK		6
#define AF_MSG_T_FSAE		7
#define AF_MSG_T_ZEBOS		8

#define EVENT_TYPE_IF_OPER 2  /* operational, got carrier */
#define EVENT_TYPE_IF_UP   1  /* operational, no carrier  */
#define EVENT_TYPE_IF_DOWN 0  /* not operational */

struct interface_event {
	int type;             /* EVENT_TYPE_IF_xxx */
	char name[IFNAMSIZ];
	int state_change;     /* EVENT_IF_xxx  (see linux/if_events.h) */
};

#ifdef __KERNEL__
extern int kernel_send_admin_packet(struct sk_buff *skb, u32 group);

struct admin_opt
{
	u32 pid;
	u32 group;
};

struct admin_sock {
	/* struct sock must be the first member of pppox_sock */
	struct sock             sk;
	struct admin_sock       *next;    /* for hash table */
	union {
		struct admin_opt admin;
	} proto;
};
#endif

#endif	/* _LINUX_ADMIN_H */
