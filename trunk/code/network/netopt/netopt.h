/**
 *	@file	netopt.h
 *
 *	@brief	network global option APIs.
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_NETOPT_H
#define FZ_NETOPT_H

/**
 * 	Set IPv4 forward value as @val. 
 *
 *	proc file is /proc/sys/net/ipv4/ip_forward.
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
netp_set_ip4_forward(int val);

/**
 * 	Get IPv4 forward value and return it. 
 *
 * 	Return >=0 if success, -1 on error.
 */
extern int 
netp_get_ip4_forward();

/**
 *	Set the IPv4 local port range as @low @high
 *
 * 	proc file is /proc/sys/net/ipv4/ip_local_port_range
 *
 * 	return 0 if success, -1 on error.
 */
extern int 
netp_set_ip4_local_port_range(int family, int low, int high);

/**
 * 	Get IP local port range saved into @low @high
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
netp_get_ip4_local_port_range(int family, int &low, int &high);

/**
 * 	Set the tcp4 memory usage as @low @media @high
 *	
 *	proc file is /proc/sys/net/ipv4/tcp_mem
 * 	
 * 	Return 0 if success, -1 on error.
 */
extern int 
netp_set_tcp4_mem(int low, int media, int high);

/**
 * 	Get tcp4 memory usage saved in @low @media @high
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
netp_get_tcp4_mem(int &low, int &media, int &high);

/**
 * 	Set tcp4 timewait socket resue as @val.
 *
 *	Return 0 if success -1 on error.
 */
extern int 
netp_set_tcp4_tw_reuse(int val);

/**
 *	Get tcp4 timewait socket resue and return it.
 *
 * 	Return >=0 if success, -1 on error.
 */
extern int 
netp_get_tcp4_tw_reuse(int val);

/**
 *	Set tcp4 timewait socket recyle as @val.
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
netp_set_tcp4_tw_recycle(int family, int val);

/**
 *	Get tcp4 timewait recycle and return it.
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
netp_get_tcp4_tw_recycle(int family, int val);

/**
 *	Set tcp4 timewait max buckets as @val
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
netp_set_tcp4_tw_max_buckets(int family, int val);

/**
 *	Get tcp4 timewait max buckets and return it.
 *
 *	Return >=0 if success, -1 on error.
 */
extern int 
netp_get_tcp4_tw_max_buckets(int family, int val);

/**
 *	Set socket recv buffer size as @val.
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
netp_set_rmem_default(int val);

/**
 *	Get socket recv buffer size and return it.
 *
 * 	Return >= 0 if success, -1 on error.
 */
extern int 
netp_get_rmem_default(void);

/**
 *	Set socket send buffer size as @val
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
netp_set_wmem_default(int val);

/**
 *	Get socket recv buffer size and return it.
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
netp_get_wmem_default();

/**
 *	Set IPv6 interface @ifname accept_dad value @val
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
netp_set_ip6_accept_dad(const char *ifname, int val);




#endif /* end of FZ_NETOPT_H  */


