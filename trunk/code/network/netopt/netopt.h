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
 *	Get interface @ifname vendor ID and return it.
 *
 *	Return >=0 if success, -1 on error.
 */
int 
nets_get_vendor_id(const char *ifname);

/**
 *	Get interface @ifname device ID and return it.
 *
 *	Return >=0 if success, -1 on error.
 */
int 
nets_get_device_id(const char *ifname);

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
 *	Set the IPv4 local port range as @range(eg "1000 30000")
 *
 * 	proc file is /proc/sys/net/ipv4/ip_local_port_range
 *
 * 	return 0 if success, -1 on error.
 */
extern int 
netp_set_ip4_local_port_range(const char *range);

/**
 * 	Get IPv4 local port range saved into @buf
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
netp_get_ip4_local_port_range(char *buf, size_t len);

/**
 * 	Set IPv4 interface @ifname promote secondaries value as @val. 
 *	The @ifname can be "default"/"all"/"interface name"
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
netp_set_ip4_promote_secondaries(const char *ifname, int val);

/**
 * 	Get default promote secondaries value and return it. 
 *
 * 	Return >=0 if success, -1 on error.
 */
extern int 
netp_get_ip4_promote_secondaries(const char *ifname);


/**
 * 	Set the TCPv4 memory usage as @range(eg, "4096 8192 16384")
 *	
 *	proc file is /proc/sys/net/ipv4/tcp_mem
 * 	
 * 	Return 0 if success, -1 on error.
 */
extern int 
netp_set_tcp4_mem(const char *ranges);

/**
 * 	Get TCPv4 memory usage saved in @buf
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
netp_get_tcp4_mem(char *buf, size_t len);

/**
 *	Set TCPv4 receive buffer size as @range(eg: 4096 87380 6291456)
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
netp_set_tcp4_rmem(const char *range);

/**
 *	Get TCPv4 receive buffer size and save it into @buf.
 *
 *	Return >=0 if success, -1 on error.
 */
extern int 
netp_get_tcp4_rmem(char *buf, size_t len);

/**
 *	Set TCPv4 send buffer size as @range(eg: 4096 87380 6291456)
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
netp_set_tcp4_wmem(const char *range);

/**
 *	Get TCPv4 send buffer size and save it into @buf.
 *
 *	Return >=0 if success, -1 on error.
 */
extern int 
netp_get_tcp4_wmem(char *buf, size_t len);

/**
 * 	Set TCPv4 timewait socket reuse value as @val.
 *
 *	Return 0 if success -1 on error.
 */
extern int 
netp_set_tcp4_tw_reuse(int val);

/**
 *	Get TCPv4 timewait socket reuse value and return it.
 *
 * 	Return >=0 if success, -1 on error.
 */
extern int 
netp_get_tcp4_tw_reuse(void);

/**
 *	Set TCPv4 timewait socket recyle value as @val.
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
netp_set_tcp4_tw_recycle(int val);

/**
 *	Get TCPv4 timewait recycle and return it.
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
netp_get_tcp4_tw_recycle(void);

/**
 *	Set TCPv4 timewait max buckets value as @val
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
netp_set_tcp4_tw_max_buckets(int val);

/**
 *	Get TCPv4 timewait max buckets value and return it.
 *
 *	Return >=0 if success, -1 on error.
 */
extern int 
netp_get_tcp4_tw_max_buckets(void);

/**
 *	Set TCPv4 fin timeout value as @val
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
netp_set_tcp4_fin_timeout(int val);

/**
 *	Get TCPv4 fin timeout value and return it.
 *
 *	Return >=0 if success, -1 on error.
 */
extern int 
netp_get_tcp4_fin_timeout(void);

/**
 *	Set TCPv4 timestamps value as @val
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
netp_set_tcp4_timestamps(int val);

/**
 *	Get TCPv4 timestamps value and return it.
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
netp_get_tcp4_timestamps(void);

/**
 *	Set TCPv4 max syn backlog value as @val
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
netp_set_tcp4_max_syn_backlog(int val);

/**
 *	Get TCPv4 max syn backlog value and return it.
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
netp_get_tcp4_max_syn_backlog(void);


/**
 *	Set default socket receive buffer size as @val.
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
netp_set_rmem_default(int val);

/**
 *	Get default socket receive buffer size and return it.
 *
 * 	Return >= 0 if success, -1 on error.
 */
extern int 
netp_get_rmem_default(void);

/**
 *	Set max socket receive buffer size as @val.
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
netp_set_rmem_max(int val);

/**
 *	Get max socket receive buffer size and return it.
 *
 * 	Return >= 0 if success, -1 on error.
 */
extern int 
netp_get_rmem_max(void);

/**
 *	Set default socket send buffer size as @val
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
netp_set_wmem_default(int val);

/**
 *	Get default socket send buffer size and return it.
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
netp_get_wmem_default(void);

/**
 *	Set max socket send buffer size as @val
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
netp_set_wmem_max(int val);

/**
 *	Get max socket send buffer size and return it.
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
netp_get_wmem_max(void);

/**
 *	Set socket max connection value as @val
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
netp_set_somaxconn(int val);

/**
 *	Get socket max connection value and return it.
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
 *	Get IPv6 interface @ifname accept_dad value and return it.
 *	The @ifname can be "default"/"all"/"interface name"
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
netp_get_ip6_accept_dad(const char *ifname);

/**
 *	Get interface irqs and stored in @irqs, the nirq 
 *	is @irqs object number
 *
 *	Return > 0 irq count if success, -1 on error.
 */
extern int 
netp_get_irqs(const char *ifname, int *irqs, size_t nirq);

#endif /* end of FZ_NETOPT_H  */


