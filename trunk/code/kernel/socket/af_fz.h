/**
 *	@file	fz_sock.h
 *
 *	@brief	a simple BSD socket family implement in kernel.
 *
 *	@author	Forrest.zhang
 *
 */


#ifndef LINUX_FZ_SOCK_H
#define LINUX_FZ_SOCK_H

#ifdef __KERNEL__
#include <linux/netdevice.h>
#else
#include <net/if.h>
#endif /* __KERNEL__ */

/* we used AF_ROSE as socket family, it's used rarely */
#define AF_FZ		AF_ROSE	
#define PF_FZ		AF_FZ

struct sockaddr_fz {
	int		family;
	unsigned long	pid;
	unsigned long	group;
};

/**
 *	Message header for AF_FZ 
 */
struct fz_message {
	unsigned long	type;
	unsigned long	len;
	unsigned char	data[0];
};

#ifdef __KERNEL__

#include <net/sock.h>

struct fz_sock {
	/* struct sock must be the first member of sock */
	struct sock	sk;
	
	/* for hash table */
	struct sock	*next;

	/* sock information */
	u32		pid;
	u32		group;
};

#endif /* __KERNEL__ */

#endif	/* LINUX_FZ_SOCK_H */
