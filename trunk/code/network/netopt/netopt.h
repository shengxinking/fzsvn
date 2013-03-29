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
netp_get_ip4_forward(void);

/**
 *	Set the IPv4 local port range as @low @high
 *
 * 	proc file is /proc/sys/net/ipv4/ip_local_port_range
 *
 * 	return 0 if success, -1 on error.
 */
extern int 
netp_set_ip4_local_port_range(int low, int high);

/**
 * 	Get IP local port range saved into @low @high
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
netp_get_ip4_local_port_range(int &low, int &high);

/**
 * 	Set IPv4 forward value as @val. 
 *
 *	proc file is /proc/sys/net/ipv4/ip_forward.
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
netp_set_ip4_def_promote_secondaries(int val);

/**
 * 	Get IPv4 forward value and return it. 
 *
 * 	Return >=0 if success, -1 on error.
 */
extern int 
netp_get_ip4_def_promote_secondaries(void);

/**
 * 	Set IPv4 forward value as @val. 
 *
 *	proc file is /proc/sys/net/ipv4/ip_forward.
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
netp_set_ip4_all_promote_secondaries(int val);

/**
 * 	Get IPv4 forward value and return it. 
 *
 * 	Return >=0 if success, -1 on error.
 */
extern int 
netp_get_ip4_all_promote_secondaries(void);

/**
 *	Set socket send buffer size as @val
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
netp_set_tcp4_timestamps(int val);

/**
 *	Get socket recv buffer size and return it.
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
netp_get_tcp4_timestamps(void);

/**
 *	Set socket send buffer size as @val
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
netp_set_tcp4_max_syn_backlog(int val);
/**
 *	Get socket recv buffer size and return it.
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
netp_get_tcp4_max_syn_backlog(void);

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
netp_get_tcp4_tw_reuse(void);

/**
 *	Set tcp4 timewait socket recyle as @val.
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
netp_set_tcp4_tw_recycle(int val);

/**
 *	Get tcp4 timewait recycle and return it.
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
netp_get_tcp4_tw_recycle(void);

/**
 *	Set tcp4 timewait max buckets as @val
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
netp_set_tcp4_tw_max_buckets(int val);

/**
 *	Get tcp4 timewait max buckets and return it.
 *
 *	Return >=0 if success, -1 on error.
 */
extern int 
netp_get_tcp4_tw_max_buckets(void);

/**
 *	Set tcp4 timewait max buckets as @val
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
netp_set_tcp4_fin_timeout(int val);

/**
 *	Get tcp4 timewait max buckets and return it.
 *
 *	Return >=0 if success, -1 on error.
 */
extern int 
netp_get_tcp4_fin_timeout(void);

/**
 *	Set tcp4 timewait max buckets as @val
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
netp_set_tcp4_rmem(int val);

/**
 *	Get tcp4 timewait max buckets and return it.
 *
 *	Return >=0 if success, -1 on error.
 */
extern int 
netp_get_tcp4_rmem(void);

/**
 *	Set tcp4 timewait max buckets as @val
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
netp_set_tcp4_wmem(int val);

/**
 *	Get tcp4 timewait max buckets and return it.
 *
 *	Return >=0 if success, -1 on error.
 */
extern int 
netp_get_tcp4_wmem(void);

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
 *	Set socket recv buffer size as @val.
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
netp_set_rmem_max(int val);

/**
 *	Get socket recv buffer size and return it.
 *
 * 	Return >= 0 if success, -1 on error.
 */
extern int 
netp_get_rmem_max(void);

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
netp_get_wmem_default(void);

/**
 *	Set socket send buffer size as @val
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
netp_set_wmem_max(int val);

/**
 *	Get socket recv buffer size and return it.
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
netp_get_wmem_max(void);

/**
 *	Set socket send buffer size as @val
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
netp_set_somaxconn(int val);

/**
 *	Get socket recv buffer size and return it.
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
netp_get_somaxconn(void);

/**
 *	Set IPv6 interface @ifname accept_dad value @val
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
netp_set_ip6_accept_dad(const char *ifname, int val);

/**
 *	Set IPv6 interface @ifname accept_dad value @val
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
netp_set_ip6_def_accept_dad(const char *ifname, int val);


#endif /* end of FZ_NETOPT_H  */


