/**
 *	@file	ipaddr.h
 *
 *	@brief	IP address for v4/v6, include some APIs to handle
 *		it.
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_IPADDR_H
#define FZ_IPADDR_H

#include <netinet/in.h>

#define	IP_STR_LEN		64	/* the ip_addr_t max string length */

/* IP address for IPv4/IPv6 */
typedef struct ip_addr {
	int	family;			/* AF_INET, AF_INET6 */
	union {
		struct in_addr	v4;	/* IPv4 address */
		struct in6_addr	v6;	/* IPv6 address */
	} _addr;
} ip_addr_t;

/* IP address/netmask for IPv4/IPv6 */
typedef struct ip_mask {
	int	family;			/* AF_INET, AF_INET6 */
	union {
		struct in_addr	v4;	/* IPv4 address */
		struct in6_addr	v6;	/* IPv6 address */
	} _addr;
	u_int32_t cidr;			/* CIDR netmask */
} ip_mask_t;

/* IP address/port for IPv4/IPv6 */
typedef struct ip_port {
	int		family;		/* AF_INET, AF_INET6 */
	union {
		struct in_addr	v4;	/* IPv4 address */
		struct in6_addr	v6;	/* IPv6 address */
	} _addr;
	u_int16_t	port;		/* TCP port */
} ip_port_t;

#define _addr4		_addr.v4
#define _addr6		_addr.v6

#define	IP_ADDR_SET_V4(ip, val) 			\
	{(ip)->family=AF_INET;(ip)->_addr4.s_addr=val;}

#define IP_ADDR_IS_V4MAPPED(ip)				\
	( (ip)->family == AF_INET6 &&			\
	  ( IN6_IS_ADDR_V4MAPPED(&(ip)->_addr6) ||	\
	    IN6_IS_ADDR_V4COMPAT(&(ip)->_addr6) ) )

#define IP_MASK_CIDR_IS_VALID(mask)			\
	((mask)->cidr >= 0 &&				\
	 (((mask)->family == AF_INET && (mask)->cidr < 33) ||	\
	  ((mask)->family == AF_INET6 && (mask)->cidr < 129)))

#define	IP_PORT_SET_V4(ip, v4, p)			\
	{(ip)->family=AF_INET;(ip)->_addr4.s_addr=v4;(ip)->port=p;}

/**
 *	Covert IPv4 netmask @mask to cidr number
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
ip4_mask_to_cidr(u_int32_t mask);

/**
 *	Convert IPv4 CIDR netmask @cidr to netmask.
 *
 *	Return netmask if success.
 */
extern u_int32_t 
ip4_cidr_to_mask(int cidr);

/**
 *	Check IPv4 address @ip is between IPv4 address
 *	@begin:@end, it's @ip >= @begin || @ip <= @end.
 *
 *	Return 1 if in range, 0 if not.
 */
int
ip4_in_range(u_int32_t ip, u_int32_t begin, u_int32_t end);

/**
 *	Check IPv4 address @ip is in IPv4 range @ip_or_range
 *	@ip_or_range have following format:
 *
 *	Single host (1.1): x.x.x.x
 *	Begin-End(2.1): x.x.x.x-x.x.x.x
 *	Begin-End(2.2): -x.x.x.x, same as 0.0.0.0-x.x.x.x
 *	Begin-End(2.3): x.x.x.x-, same as x.x.x.x-0.0.0.0
 *	Subnet(3.1): xxx.xxx.xxx.0/24 or xxx.xxx.0.0/16
 *	Subnet(3.2): xxx.xxx.0.0/255.255.255.255
 *     
 *	Return 1 if matched, 0 if not matched or error.
 */
int 
ip4_match_range(u_int32_t ip, const char *ip_or_range);

/**
 *	Convert ip_addr_t object @ip to string format. 
 *
 *	Return pointer to buf if success, NULL on error. 
 */
extern char * 
ip_addr_to_str(const ip_addr_t *ip, char *buf, size_t len);

/**
 *	Convert string @str to ip_addr_t object @ip.
 *	The string must be IPv4/IPv6 IP address string:
 *	IPv4 (1.1): xxx.xxx.xxx.xxx.
 *	Ipv6 (2.1): xxxx:xxxx:xxxx:xxxx:xxxx:... (16 bytes)
 *	IPv6 (2.2): xxxx::xxxx (skipped continue zero)
 *	IPv6 (2.3): ::ffff:xxx.xxx.xxx.xxx (IPv4 mapped)
 *	Ipv6 (2.4): ::xxx.xxx.xxx.xxx (IPv4 compatible)
 *
 *	Return 0 if success, -1 on error.
 */
extern int
ip_addr_from_str(ip_addr_t *ip, const char *buf);

/**
 *	Compare two ip_addr_t object have same address.
 *
 *	Return 0 if same, 1 if @ip1 > @ip2, -1 if @ip1 < @ip2
 */
extern int 
ip_addr_compare(const ip_addr_t *ip1, const ip_addr_t *ip2);

/**
 *	Convert a IPv4 address @v4 to IPv6 address @v6.
 *	The @v6 is IPv4 mapped address, it can use IPv4 program
 *	communicate with IPv6 program.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
ip_addr_v4_to_v6(const ip_addr_t *v4, ip_addr_t *v6);

/**
 *	Convert a IPv6 address @v6 to IPv4 address @v4.
 *	The @v6 must be IPv4 mapped address or IPv4 
 *	compatible address.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
ip_addr_v6_to_v4(const ip_addr_t *v6, ip_addr_t *v4);

/**
 *	Apply netmask on IP address @ip. It's used to
 *	Get the network address of IP address.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
ip_addr_apply_mask(ip_addr_t *ip, int cidr);

/**
 *	Convert a ip_mask_t object @ip to string saved
 *	in @buf, the @buf length must >= INET6_ADDR_LEN
 *
 *	Return pointer to buf if success, NULL on error.
 */
extern char * 
ip_mask_to_str(const ip_mask_t *mask, char *buf, size_t len);

/**
 *	Convert a string @str to ip_mask_t object @mask.
 *	The IP mask string format:
 *	IPv4 tradition (1.1): x.x.x.x/x.x.x.x
 *	IPv4 cidr (1.2): x.x.x.x/x
 *	IPv6 full(2.1): xxxx:xxxx:xxxx:.../x
 *	IPv6 abbrev(2.2): xxx::xxx/x
 *
 *	Returns:
 *	-1: input error
 *	 0: parse succeed
 *	 1: not format of <subnet>/<mask>
 *	 2: <subnet> not a IP address
 *	 3: <mask> is not a integer and in range of [0, 32]
 */
extern int 
ip_mask_from_str(ip_mask_t *mask, const char *buf);

/**
 *	Compare ip_mask_t object @mask1 @mask2 is same IP
 *	or not.
 *
 *	Return 0 if same, 1 means @mask1 > @mask2 -1 means
 *	@mask1 < @mask2.
 */
extern int 
ip_mask_compare(const ip_mask_t *mask1, const ip_mask_t *mask2);

/**
 *	Convert ip_port_t object to string and save it in
 *	@buf, @buf length is @len.
 *
 *	Return 0 if success, -1 on error.
 */
extern char * 
ip_port_to_str(const ip_port_t *ip, char *buf, size_t len);

/**
 *	Convert a string @str to ip_port_t object @ip.
 *	IPv4 format(1): x.x.x.x:x
 *	IPv6 format(2): [x:x:x:x:x:x:x...]:x
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
ip_port_from_str(ip_port_t *ip, const char *str);

/**
 * 	Compare ip_port_t object @ip1 and @ip2.
 *
 *	Return 0 if them is equal, 1 if @ip1 > @ip2, 
 *	-1 if @ip1 < @ip2
 */
extern int 
ip_port_compare(const ip_port_t *ip1, const ip_port_t *ip2);

/********************************************************* 
 *	Socket util functions.
 ********************************************************/
/**
 * 	Common socket address.
 */
typedef struct sk_addr {
	union {
		struct sockaddr		sa;	/* common */
		struct sockaddr_in	v4;	/* IPv4 */
		struct sockaddr_in6	v6;	/* IPv6 */
	} _addr;
} sk_addr_t;

#define	_addrsa		_addr.sa
#define	_addrv4		_addr.v4
#define _addrv6		_addr.v6


/**
 *	Create TCP server socket accoring IP port address @ip. 	
 *
 * 	Return socket fd is success, -1 on error.
 */
extern int
sk_tcp_server(ip_port_t *ip);

/**
 * 	Create a TCP client socket and connect to IP port address
 * 	@ip.
 *
 * 	Return socket fd is success, -1 on error.
 */
extern int 
sk_tcp_client(ip_port_t *ip);

/**
 * 	Create a TCP client socket and connect to IP port address
 * 	@ip, it use NONBLOCK socket before connect. If connection
 *	is finished, set @wait = 0, or set @wait = 1 and need
 *	check connection is finished later.
 *
 * 	Return socket fd is success, -1 on error.
 */
extern int 
sk_tcp_client_nb(ip_port_t *ip, int *wait);

/**
 * 	Connect socket @fd to server IP port address @ip.
 *	
 *	Return 0 if success, 1 need check later, -1 on error.
 */
extern int 
sk_tcp_connect(int fd, ip_addr_t *ip, u_int16_t port);

/**
 * 	Check Socket @fd connection is success or failed.
 *
 *	Return 0 if success, -1 if error.
 */
extern int 
sk_is_connected(int fd);

/**
 *	Accept an client and save it's IP port address in @ip.
 *
 *	Return client socket fd if accept success, -1 on error.
 */
extern int 
sk_accept(int fd, ip_port_t *ip);

/**
 * 	Set socket @fd to NONBLOCK mode if @nbio != 0, clear
 * 	socket @fd to BLOCKED mode if @nbio is 0.
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
sk_set_nonblock(int fd, int nbio);

/**
 * 	Set TCP socket @fd keepalive if @keepalive != 0, clear
 * 	@fd keepalive if @keepalive is 0.
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
sk_set_keepalive(int fd);

/**
 * 	Recv data from socket @fd, the data saved in @buf, it
 * 	the @buf size is @len.
 *
 * 	If peer close connect, the @closed is set 1.
 *
 * 	Return recevied bytes number if success, -1 on error.
 */
extern int 
sk_recv(int fd, void *buf, size_t len, int *closed);

/**
 * 	Send data in @buf to socket @fd, the @buf length is
 * 	@len.
 *
 * 	Return send bytes number is success, -1 on error.
 */
extern int 
sk_send(int fd, const void *buf, size_t len);

/**
 *	Resolve the domain name and save it into IP. 
 *	If family is AF_INET, only return IPv4 address.
 *	If family is AF_INET6, only return IPv6 address.
 *	If family is 0, return a valid address if exist.
 *
 *	Return 0 if sucess, -1 on error.
 */
extern int 
sk_gethostbyname(const char *domain, int family, ip_addr_t *ip);

#endif /* end of FZ_IP_ADDR_H  */


