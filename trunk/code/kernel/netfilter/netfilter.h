/*
 * @file	netfilter.h
 * @brief	some APIs using in netfilter application
 *
 * @author	Forrest.zhang
 */

#ifndef FZ_NETFILTER_H
#define FZ_NETFILTER_H

#include <sys/socket.h>
#include <netinet/in.h>

#include "declare.h"

#define SO_ORIGINAL_DST 80

BEGIN_DECLS

extern int nf_origin_dst(int fd, struct sockaddr_in *addr);


END_DECLS

#endif /* end of FZ_NETFILTER_H */

