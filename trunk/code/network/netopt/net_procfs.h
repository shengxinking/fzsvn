/**
 *	@file	net_procfs.h
 *
 *	@brief	Get/Set network option by /proc filesystem.
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_NET_PROCFS_H
#define FZ_NET_PROCFS_H

/**
 *	Get error message when netp_xxxx failure.
 *
 *	Return string of error message.
 */
extern const char * 
netp_get_error();

/**
 *	Get default socket receive buffer size and return it.
 *
 * 	Return >= 0 if success, -1 on error.
 */
extern int 
netp_core_get_rmem_default(void);

/**
 *	Set default socket receive buffer size as @val.
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
netp_core_set_rmem_default(int val);

/**
 *	Get max socket receive buffer size and return it.
 *
 * 	Return >= 0 if success, -1 on error.
 */
extern int 
netp_core_get_rmem_max(void);

/**
 *	Set max socket receive buffer size as @val.
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
netp_core_set_rmem_max(int val);

/**
 *	Get default socket send buffer size and return it.
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
netp_core_get_wmem_default(void);

/**
 *	Set default socket send buffer size as @val
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
netp_core_set_wmem_default(int val);

/**
 *	Get max socket send buffer size and return it.
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
netp_core_get_wmem_max(void);

/**
 *	Set max socket send buffer size as @val
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
netp_core_set_wmem_max(int val);

/**
 *	Get socket max connection value and return it.
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
netp_core_get_somaxconn(void);

/**
 *	Set socket max connection value as @val
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
netp_core_set_somaxconn(int val);

/**
 *	Get global RFS table size.
 *
 *	Return >=0 if success, -1 on error.
 */
extern int 
netp_core_get_rps(void);

/**
 *	Set global RFS table size as @flowcnt.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
netp_core_set_rps(int val);

/**
 * 	Get IPv4 ip_forward value and return it. 
 *
 *	proc file is /proc/sys/net/ipv4/ip_forward.
 *
 * 	Return >=0 if success, -1 on error.
 */
extern int 
netp_ip4_get_forward(void);

/**
 * 	Set IPv4 ip_forward value as @val. 
 *
 *	proc file is /proc/sys/net/ipv4/ip_forward.
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
netp_ip4_set_forward(int val);

/**
 * 	Get IPv4 local port range saved into @buf
 *	
 * 	proc file is /proc/sys/net/ipv4/ip_local_port_range
 *
 *	Return 0 if success, -1 on error.
 */
extern char *  
netp_ip4_get_local_port_range(char *buf, size_t len);

/**
 *	Set the IPv4 local port range as @range(eg "1000 30000")
 *
 * 	proc file is /proc/sys/net/ipv4/ip_local_port_range
 *
 * 	return 0 if success, -1 on error.
 */
extern int 
netp_ip4_set_local_port_range(const char *range);

/**
 * 	Get default promote secondaries value and return it. 
 *	The @ifname can be "default"/"all"/"interface name"
 *
 *	proc file is /proc/sys/net/conf/@ifname/promote_secondaries
 *
 * 	Return >=0 if success, -1 on error.
 */
extern int 
netp_ip4_get_promote_secondaries(const char *ifname);

/**
 * 	Set IPv4 interface @ifname promote secondaries value as @val. 
 *	The @ifname can be "default"/"all"/"interface name"
 *
 *	proc file is /proc/sys/net/conf/@ifname/promote_secondaries
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
netp_ip4_set_promote_secondaries(const char *ifname, int val);

/**
 * 	Get TCPv4 memory usage saved in @buf
 *
 *	proc file is /proc/sys/net/ipv4/tcp_mem
 * 	
 *	Return 0 if success, -1 on error.
 */
extern char * 
netp_tcp4_get_mem(char *buf, size_t len);

/**
 * 	Set the TCPv4 memory usage as @range(eg, "4096 8192 16384")
 *	
 *	proc file is /proc/sys/net/ipv4/tcp_mem
 * 	
 * 	Return 0 if success, -1 on error.
 */
extern int 
netp_tcp4_set_mem(const char *ranges);

/**
 *	Get TCPv4 receive buffer size and save it into @buf.
 *
 *	proc file is /proc/sys/net/ipv4/tcp_rmem
 *
 *	Return >=0 if success, -1 on error.
 */
extern char *  
netp_tcp4_get_rmem(char *buf, size_t len);

/**
 *	Set TCPv4 receive buffer size as @range 
 *	(eg: 4096 87380 6291456)
 *
 *	proc file is /proc/sys/net/ipv4/tcp_rmem
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
netp_tcp4_set_rmem(const char *range);

/**
 *	Get TCPv4 send buffer size and save it into @buf.
 *
 *	proc file is /proc/sys/net/ipv4/tcp_wmem
 *
 *	Return >=0 if success, -1 on error.
 */
extern char *  
netp_tcp4_get_wmem(char *buf, size_t len);

/**
 *	Set TCPv4 send buffer size as @range 
 *	(eg: 4096 87380 6291456)
 *
 *	proc file is /proc/sys/net/ipv4/tcp_wmem
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
netp_tcp4_set_wmem(const char *range);

/**
 *	Get TCPv4 timewait socket reuse value and return it.
 *
 *	proc file is /proc/sys/net/ipv4/tcp_tw_reuse
 *
 * 	Return >=0 if success, -1 on error.
 */
extern int 
netp_tcp4_get_tw_reuse(void);

/**
 * 	Set TCPv4 timewait socket reuse value as @val.
 *
 *	proc file is /proc/sys/net/ipv4/tcp_tw_reuse
 *
 *	Return 0 if success -1 on error.
 */
extern int 
netp_tcp4_set_tw_reuse(int val);

/**
 *	Get TCPv4 timewait recycle and return it.
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
netp_tcp4_get_tw_recycle(void);

/**
 *	Set TCPv4 timewait socket recyle value as @val.
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
netp_tcp4_set_tw_recycle(int val);

/**
 *	Get TCPv4 timewait max buckets value and return it.
 *
 *	Return >=0 if success, -1 on error.
 */
extern int 
netp_tcp4_get_max_tw_buckets(void);

/**
 *	Set TCPv4 timewait max buckets value as @val
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
netp_tcp4_set_max_tw_buckets(int val);

/**
 *	Get TCPv4 fin timeout value and return it.
 *
 *	Return >=0 if success, -1 on error.
 */
extern int 
netp_tcp4_get_fin_timeout(void);

/**
 *	Set TCPv4 fin timeout value as @val
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
netp_tcp4_set_fin_timeout(int val);

/**
 *	Get TCPv4 timestamps value and return it.
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
netp_tcp4_get_timestamps(void);

/**
 *	Set TCPv4 timestamps value as @val
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
netp_tcp4_set_timestamps(int val);

/**
 *	Get TCPv4 max syn backlog value and return it.
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
netp_tcp4_get_max_syn_backlog(void);

/**
 *	Set TCPv4 max syn backlog value as @val
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
netp_tcp4_set_max_syn_backlog(int val);

/**
 *	Get IPv6 interface @ifname accept_dad value and return it.
 *	The @ifname can be "default"/"all"/"interface name"
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
netp_ip6_get_accept_dad(const char *ifname);

/**
 *	Set IPv6 interface @ifname accept_dad value @val
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
netp_ip6_set_accept_dad(const char *ifname, int val);

/**
 *	Get the IPv6 interface @ifname disable value.
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
netp_ip6_get_disable(const char *ifname);

/**
 *	Set the IPv6 interface @ifname disable value as @val, 
 *	if @val no zero, disable IPv6 or else enable IPv6.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
netp_ip6_set_disable(const char *ifname, int val);

/**
 *	Get the interface @ifname IPv6 forwarding value.
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
netp_ip6_get_forwarding(const char *ifname);

/**
 *	Set the interface @ifname IPv6 forwarding value as @val.
 *	
 *	Return 0 if success, -1 on error.
 */
extern int 
netp_ip6_set_forwarding(const char *ifname, int val);

/**
 *	Get the interface @ifname IPv6 mtu.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
netp_ip6_get_mtu(const char *ifname);

/**
 *	Set the interface @ifname IPv6 mtu as @mtu.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
netp_ip6_set_mtu(const char *ifname, int mtu);

/**
 *	Get IRQ @irq CPU bind mask.
 *
 *	Return >0 if success, 0 on error.
 */
extern u_int32_t  
netp_irq_get_cpu(int irq);

/**
 *	Bind IRQ @irq to CPU mask @mask.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
netp_irq_set_cpu(int irq, u_int32_t mask);

/**
 *	Get the device name of give IRQ @irq, the 
 *	device name stored in @buf, @buf len is @len.
 *
 *	Return 0 if success, -1 on error.
 */
extern char * 
netp_irq_get_name(int irq, char *buf, size_t len);


#endif /* end of FZ_NET_PROCFS_H  */


