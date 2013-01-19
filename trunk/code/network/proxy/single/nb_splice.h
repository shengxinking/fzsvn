#ifndef _NB_SPLICE_H
#define _NB_SPLICE_H

#include <sys/types.h>
#include <linux/ioctl.h>

#define NB_SPLICE_MAJOR_NUM 142
#define NB_SPLICE_DEV	"/dev/nbsplice"

#define IOCTL_DO_SPLICE _IOR(NB_SPLICE_MAJOR_NUM, 11, char *)

typedef struct nb_splice {
	int		infd;	/* input socket */
	int		outfd;	/* output socket */
	size_t		max;	/* max number of byte need splice */
	unsigned int	flags;	/* splice flags */
	unsigned int	len;	/* splice bytes, returned value */
	unsigned short	is_open;/* 1 means can't close */
} nb_splice_t;

extern int 
nb_splice_init(void);

extern int 
nb_splice(int nbfd, int infd, int outfd, size_t max, u_int32_t flags);

#endif

