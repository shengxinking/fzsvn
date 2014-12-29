/**
 *	@file	netutil.h
 *
 *	@brief	some convert/print/check function for network.
 */
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

#include "gcc_common.h"
#include "dbg_common.h"
#include "ip_addr.h"

static u_int8_t _s_mask_partial[9] = {
	0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff
};

/**
 *	Apply netmask @cidr on IPv4 address @ip to 
 *	get network address
 *
 * 	Return 0 if success, -1 on error.
 */
static int 
_ip4_apply_mask(struct in_addr *ip, int cidr)
{
	int whole;
	int partial;
	int start;
	int i;
	u_int8_t *ptr;

	if (unlikely(cidr < 1 || cidr > 31))
		ERR_RET(-1, "invalid argument\n");

	whole = cidr / 8;
	partial = cidr % 8;

	// Need to remove extraneous bits
	start = whole;
	ptr = (u_int8_t *)&(ip->s_addr);
	
	if (partial) {
		ptr[whole + 1] &= _s_mask_partial[partial];
		++start;
	}

	for (i = start; i < 16; ++i)
		ptr[i] = 0;

	return 0;
}

/**
 *	Apply netmask @cidr on IPv6 address @ip to 
 *	get network address
 *
 * 	Return 0 if success, -1 on error.
 */
static int 
_ip6_apply_mask(struct in6_addr *ip, int cidr)
{
	int whole;
	int partial;
	int start;
	int i;
	
	if (unlikely(cidr < 1 || cidr > 128)) 
		ERR_RET(-1, "invalid agrument\n");

	whole = cidr / 8;
	partial = cidr % 8;

	/* Need to remove extraneous bits */
	start = whole;
	if (partial) {
		ip->s6_addr[whole + 1] &= _s_mask_partial[partial];
		++start;
	}

	for (i = start; i < 16; ++i) {
		ip->s6_addr[i] = 0;
	}

	return 0;
}

/**
 *	Get CIDR according string @str.
 *	@str format is:
 *	xxx.xxx.xxx.xxx		IPv4 netmask.
 *	xx:xx:xx:xx:xx:xx:xx:xx	IPv6 netmask.
 *	NNN			CIDR format netmask.
 *
 *	Return >= 0 if success, -1 on error.
 */
static int 
_ip_get_cidr(const char *str)
{
	ip_addr_t ip;

	if (!str)
		ERR_RET(-1, "invalid argument\n");

	/* x.x.x.x format(1.1), only in IPv4 */
	if (strchr(str, '.')) {
		if (ip_addr_from_str(&ip, str) == 0 && 
		    ip.family == AF_INET)
			return ip4_mask_to_cidr(ip._addr4.s_addr);
	}
	/* CIDR format(1.2/2.1), in some IPv4 and all IPv6 */
	else {
		return atoi(str);
	}

	return -1;
}

u_int32_t 
ip4_cidr_to_mask(int cidr)
{
	if (unlikely(cidr <= 0 || cidr > 32))
		ERR_RET(0, "invalid argument\n");

	return htonl(~((1 << (32 - cidr)) - 1));
}

int 
ip4_mask_to_cidr(u_int32_t mask)
{
	int i;

	mask = ntohl(mask);
	for (i = 0; i < 32; i++)
		if (mask & (1 << i)) break;
		
	return (32 - i);
}

int  
ip4_from_str(u_int32_t *ip, const char *str)
{
	if (unlikely(!ip || !str))
		ERR_RET(-1, "invalid argument\n");

	if (inet_pton(AF_INET, str, ip) == 1) 
		return 0;
	else
		return -1;
}

const char *
ip4_to_str(u_int32_t ip, char *buf, size_t len)
{
	if (unlikely(!buf || len < INET_ADDRSTRLEN)) 
		ERR_RET(NULL, "invalid argument\n");

	return inet_ntop(AF_INET, &ip, buf, len);
}

int
ip4_in_range(u_int32_t ip, u_int32_t begin, u_int32_t end)
{
	ip = ntohl(ip);
	begin = ntohl(begin);
	end = ntohl(end);

	return (ip >= begin && ip <= end);
}

int
ip4_match_range(u_int32_t ip, const char *ip_or_range)
{
	int res = 0;
	int cidr = 0;
	char * ptr;
	u_int32_t value = 0;
	u_int32_t begin = 0;
	u_int32_t end = 0;
	u_int32_t mask = 0;
	char range_copy[IP_STR_LEN];

	if (unlikely(!ip_or_range))
		ERR_RET(0, "invalid argument\n");

	/* save ip_or_range string */
	strncpy(range_copy, ip_or_range, sizeof(range_copy) - 1);
	range_copy[sizeof(range_copy) - 1] = 0;

	/* It's IP range string */
	if ((ptr = strchr(range_copy, '-')) == NULL) {
		/**
		 * Format (1) or (3) see comments before this 
		 * function.
		 */
		if ((ptr = strchr(range_copy, '/')) == NULL) {
			/**
			 * Format (1): Single host only.
			 */
			if (ip4_from_str(&value, range_copy) == 0 && 
			    value == ip) 
			{
				res = 1;
			}

			return res;
		} 
		
		/**
		 * Format (3): subnet ip_or_range e.g. 
		 * 172.16.0.0/16 or 172.16.0.0/255.255.0.0
		 */
		cidr = atoi(ptr + 1);
		*ptr = '\0';
		ip4_from_str(&value, range_copy);
		*ptr = '/';

		if (strchr(ptr + 1, '.')) {
			/**
			 * Format (3.1): Subnet Mask. e.g. 
			 * 172.16.0.0/255.255.0.0
			 * Convert to <IP>/cidr format to 
			 * be consistent with (3.2)
			 */
			ip4_from_str(&mask, ptr + 1);
			cidr = ip4_mask_to_cidr(mask);
		}

		/**
		 * Format (3.2): <IP>/cidr format: e.g., 
		 * 172.16.0.0/16
		 */
		if (cidr <= 0 || cidr >= 32) {
			/** 
			 * Treat <IP> with invalide cidr as a 
			 * single host 
			 */
			if (value == ip) {
				res = 1;
			}
		} else {
			/**
			 * Determine it's a single host or a range.
			 * If the host part of address are not 0, 
			 * treat <IP>/cidr
			 * as a single IP address.
			 * This is a rule of distiguish networking 
			 * segment address
			 * to single host IP adress if presented 
			 * in format (3.1) or in (3.2)
			 */
			value = ntohl(value);
			if ((value << cidr) >> cidr) {
				/* Single host IP */
				if (value == ntohl(ip)) {
					res = 1;
				}
			} else {
				/* networking address: a range */
				cidr = 32 - cidr;
				begin = (value >> cidr) << cidr;
				end = value;
				begin = htonl(begin);
				end = htonl(end);
				while (cidr) {
					end |= (1 << (--cidr));
				}

				if (ip4_in_range(ip, begin, end)) {
					res = 1;
				}
			}
		}
	} else {
		/**
		 * Format (2). See comments before this function.
		 * It's an ip range,e.g.172.16.79.10-172.16.79.1
		 */
		*ptr = '\0';
		if (ptr == range_copy) {
			/* if no start ip, start ip is 0.0.0.0 */
			begin = 0;
		} else {
			ip4_from_str(&begin, range_copy);
		}

		if (*(ptr + 1) == '\0') {
			/* no end ip, end ip is 255.255.255.255 */
			end = 0xffffffff;
		} else {
			ip4_from_str(&end, ptr + 1);
		}
		*ptr = '-';

		if (ip4_in_range(ip, begin, end)) {
			res = 1;
		}
	}

	return res;
}

int 
ip4_is_zero(u_int32_t ip)
{
	return (ip == 0);
}

int
ip4_is_loopback(u_int32_t ip)
{               
	return ((ip & 0x7f000000) == 0x7f000000 ? 1 : 0);
}                                                                                          

int             
ip4_is_multicast(u_int32_t ip)
{       
	return ip4_match_range(ip, "224.0.0.0-239.255.255.255");
}

int
ip4_is_linklocal(u_int32_t ip)
{               
	return ((ip & 0xfe800000) == 0xfe80000 ? 1 : 0);
}

int
ip4_is_sitelocal(u_int32_t ip)
{
	if (ip4_match_range(ip, "10.0.0.0-10.255.255.255"))
		return 1;

	if (ip4_match_range(ip, "172.16.0.0-172.31.255.255"))
		return 1;

	if (ip4_match_range(ip, "192.168.0.0-192.168.255.255"))
		return 1;

	return 0;
}

int
ip6_from_str(void *ip6, const char *ip6str)
{       
	int ret;

	if (unlikely(!ip6 || !ip6str))
		ERR_RET(-1, "invalid argument\n");

	ret = inet_pton(AF_INET6, ip6str, ip6);

	return ret == 1 ? 0 : -1;
}

char *
ip6_to_str(const void *addr, char *buf, size_t len)
{
	if (unlikely(!addr || !buf || len < INET6_ADDRSTRLEN))
		return NULL;

	return (char *)inet_ntop(AF_INET6, (void *)addr, buf, len);
}

int
ip6_is_netaddr(const void *ip6, int cidr)
{
	struct in6_addr netaddr;

	if (unlikely(!ip6 || cidr < 0 || cidr > 128))
		ERR_RET(0, "invalid argument\n");

	if (cidr == 128)
		return 1;

	memcpy(&netaddr, ip6, sizeof(netaddr));
	if (_ip6_apply_mask(&netaddr, cidr))
		return 0;

	if (memcmp(ip6, &netaddr, 16) == 0)
		return 1;

	return 0;
}

int
ip6_is_zero(const struct in6_addr *ip6)
{       
	if (unlikely(!ip6))
		ERR_RET(0, "invalid argument\n");

	if (ip6->s6_addr32[0] == 0 &&
	    ip6->s6_addr32[1] == 0 &&
	    ip6->s6_addr32[2] == 0 &&
	    ip6->s6_addr32[3] == 0)
		return 1;
	else
		return 0;
}

int
ip_addr_str_is_valid(const char *str)
{
	int i;
	ip_addr_t ip;

	if (unlikely(!str)) 
		ERR_RET(0, "invalid argument\n");

	/** 
	 * inet_aton() will not complain about spaces 
	 * so check here
	 */
	for (i = 0; str[i]; i++)
		if (isspace(str[i]))
			return 0;

	/* IPv6 address: xx:xx:xx:xx:xx:xx:xx:xx... */
	if (strchr(str, ':'))
		ip.family = AF_INET6;	
	/* IPv4 address: xxx.xxx.xxx.xxx */
	else 
		ip.family = AF_INET;
	
	if (inet_pton(ip.family, str, &ip._addr))
		return 1;

	return 0;
}

int
ip_addr_from_str(ip_addr_t *ip, const char *str)
{

	if (unlikely(!ip || !str)) 
		ERR_RET(-1, "invalid argument\n");

#if 0
	/* check begin white space */ 
	int i;
	for (i = 0; str[i]; i++)
		if (isspace(str[i]))
			ERR_RET(-1, "string begin with space char\n");
#endif

	/* IPv6 address: xx:xx:xx:xx:xx.... or [xx:xx:xx....] */
	if (strchr(str, ':')) 
		ip->family = AF_INET6;
	/* IPv4 address: xxx.xxx.xxx.xxx */
	else
		ip->family = AF_INET;

	return inet_pton(ip->family, str, &ip->_addr) > 0 ? 0 :-1;
}

char *
ip_addr_to_str(const ip_addr_t *ip, char *buf, size_t len)
{
	/* check argument */
	if (unlikely(!ip || !buf || len < IP_STR_LEN)) 
		ERR_RET(NULL, "invalid arguments\n");

	return (char *)inet_ntop(ip->family, &ip->_addr, buf, len);
}

int 
ip_addr_apply_mask(ip_addr_t *ip, int cidr)
{
	if (unlikely(!ip)) 
		ERR_RET(-1, "invalid argument\n");

	if (ip->family == AF_INET) 
		return _ip4_apply_mask(&(ip->_addr.v4), cidr);
	else if (ip->family == AF_INET6) 
		return _ip6_apply_mask(&(ip->_addr.v6), cidr);

	return -1;
}

int 
ip_addr_from_socket(ip_addr_t *ip, int fd, int get_remote)
{
	struct sockaddr addr;
	struct sockaddr_in *v4;
	struct sockaddr_in6 *v6;
	socklen_t len;
	int ret;
	
	if (unlikely(!ip || fd < 0)) 
		ERR_RET(-1, "invalid argument\n");
	
	len = sizeof(addr);
	if (get_remote)
		ret = getpeername(fd, &addr, &len);
	else
		ret = getsockname(fd, &addr, &len);
	if (ret < 0)
		return -1;

	if (addr.sa_family == AF_INET) {
		ip->family = AF_INET;
		v4 = (struct sockaddr_in *)&addr;
		ip->_addr4.s_addr = v4->sin_addr.s_addr;
    		return 0;
	} 
	else if (addr.sa_family == AF_INET6 ) {
		ip->family = AF_INET6;
		v6 = (struct sockaddr_in6 *)&addr;
		ip->_addr6 = v6->sin6_addr;
		return 0;
	}
	
	return -1;
}

int 
ip_addr_from_dns(ip_addr_t *ip, const char *name)
{
	int err;
	int ret;
	size_t len;
	char buf[4096];
	struct hostent he;
	struct hostent *phe = NULL;

	if (unlikely(!ip || !name)) 
		ERR_RET(-1, "invalid argument\n");
	
	len = sizeof(buf);
	ret = gethostbyname_r(name, &he, buf, len, &phe, &err); 
	if (ret || phe == NULL) 
		return -1;
	
	/* convert address string to ip_addr_t @ip */
	return ip_addr_from_str(ip, phe->h_addr_list[0]);
}

int
ip_addr_v4_to_v6(const ip_addr_t *v4, ip_addr_t *v6)
{
	u_int8_t *v;
	u_int32_t ipv4;

	if (unlikely(!v4 || !v6))
		ERR_RET(-1, "invalid argument\n");

	if (unlikely(v4->family != AF_INET))
		ERR_RET(-1, "v4 is not IPv4 address\n");

	v6->family = AF_INET6;
	ipv4 = v4->_addr4.s_addr;
	v = (u_int8_t *)(&ipv4);
	struct in6_addr addr = {{{0,0,0,0,0,0,0,0,0,0,0xFF,0xFF,\
				  v[0], v[1], v[2], v[3]}}};
	memcpy(&v6->_addr, &addr, sizeof(addr));

	return 0;
}

int
ip_addr_v6_to_v4(const ip_addr_t *v6, ip_addr_t *v4)
{
	u_int32_t *addr;

	if (unlikely(!v4 || !v6)) 
		ERR_RET(-1, "invalid arguments\n");

	if (IP_ADDR_IS_V4MAPPED(v6)) {
		v4->family = AF_INET;
		addr = &v4->_addr4.s_addr;
		*addr = v6->_addr6.s6_addr32[3];
		return 0;
	}

	return -1;
}

int
ip_addr_compare(const ip_addr_t *ip1, const ip_addr_t *ip2)
{
	u_int32_t lv, rv;
	ip_addr_t v6;
	size_t len;

	if (unlikely(ip1 && !ip2))
		return 1;

	if (unlikely(!ip1 && ip2))
		return -1;

	if (unlikely(ip1 == ip2))
		return 0;

	
	len = sizeof(struct in6_addr);

	/* same type */
	if (ip1->family == ip2->family) {
		/* IPv4 address compare */
		if (ip1->family == AF_INET) {
			lv = ntohl(ip1->_addr4.s_addr);
			rv = ntohl(ip2->_addr4.s_addr);
			
			if (lv == rv)
				return 0;
			else if (lv < rv)
				return -1;
			else
				return 1;
		}
		
		/* IPv6 address compare */
		return memcmp(&ip1->_addr6, &ip2->_addr6, len);
	}

     	/* convert IPv4 to IPv6 and compare */
	if (ip1->family == AF_INET) {
		if (ip_addr_v4_to_v6(ip2, &v6))
			return -1;
		return memcmp(&v6._addr6, &ip2->_addr6, len);
	}
	else if (ip2->family == AF_INET) {
		if (ip_addr_v4_to_v6(ip2, &v6))
			return -1;
		return memcmp(&ip1->_addr6, &v6._addr6, len);
	}

	return -1;
}

int
ip_mask_from_str(ip_mask_t *mask, const char *str)
{
	int ret = 0;
	const char *sep;
	int len;
	char ip[IP_STR_LEN] = {'\0'};
	
	if (unlikely(!str || !mask)) 
		ERR_RET(-1, "invalid argument\n");

	/* Find the slash first */
	sep = strchr(str, '/');

	/* find netmask */
	if (sep && *(sep + 1)) {
		mask->cidr = _ip_get_cidr(sep + 1);
		len = sep - str;
		if (len > INET6_ADDRSTRLEN) {
			return -1;
		}

		strncpy(ip, str, len);
		if (ip_addr_from_str((ip_addr_t *)mask, ip))
			ret = -1;
		else {
			if (!IP_MASK_CIDR_IS_VALID(mask))
				ret = -1;
		}
	}
	else {
		ret = -1;
	}

	return ret;
}

char *
ip_mask_to_str(const ip_mask_t *mask, char *buf, size_t len)
{
	char ip[IP_STR_LEN] = {'\0'};

	if (unlikely(!mask || !buf || len < IP_STR_LEN)) 
		ERR_RET(NULL, "invalid argument\n");

	/* get address string */
	if (!ip_addr_to_str((ip_addr_t *)mask, ip, IP_STR_LEN))
		return NULL;
	
	/* check CIDR */
	if (!IP_MASK_CIDR_IS_VALID(mask))
		return NULL;

	snprintf(buf, len, "%s/%d", ip, mask->cidr);
	return buf;
}

int
ip_mask_compare(const ip_mask_t *mask1, const ip_mask_t *mask2)
{
	int ret;
	ip_addr_t *ip1;
	ip_addr_t *ip2;

	if (unlikely(mask1 && !mask2))
		return 1;

	if (unlikely(!mask1 && mask2))
		return -1;

	if (unlikely(mask1 == mask2))
		return 0;

	ip1 = (ip_addr_t *)mask1;
	ip2 = (ip_addr_t *)mask2;
	ret = ip_addr_compare(ip1, ip2);
	if (ret)
		return ret;

	return mask1->cidr - mask2->cidr;
}

int
ip_mask_is_wildcard(const ip_mask_t *mask)
{
	if (unlikely(!mask)) 
		ERR_RET(0, "invalid argument\n");

	if (mask->cidr != 0)
		return 0;

	if (mask->family == AF_INET) {
		return mask->_addr4.s_addr == 0;
	}
	else if (mask->family == AF_INET6) {
		return IN6_IS_ADDR_UNSPECIFIED(&mask->_addr6);
	}
		
	return 0;
}

int
ip_mask_match(const ip_mask_t *mask, const ip_addr_t *ip)
{
	int whole;
	int partial;
	u_int8_t partial_mask;

	if (unlikely(!mask || !ip)) 
		ERR_RET(0, "invalid argument\n");
	
	if (mask->family != ip->family)
		return 0;

	if (mask->cidr == 0)
		return 1;

	whole = mask->cidr / 8;
	partial = mask->cidr % 8;
	
	if (memcmp(&mask->_addr6, &ip->_addr6, whole))
		return 0;

	partial_mask = _s_mask_partial[partial];
	return (mask->_addr6.s6_addr[whole] & partial_mask) ==
		(ip->_addr6.s6_addr[whole] & partial_mask);
}

int 
ip_port_from_str(ip_port_t *ip, const char *str)
{
	int port;
	char *ptr1;
	char *ptr2;
	char buf[IP_STR_LEN] = {0};

	if (unlikely(!ip || !str))
		ERR_RET(-1, "invalid argument\n");

	strncpy(buf, str, sizeof(buf) - 1);

	/* IPv6 format: [x:x:x:x...]:x */
	if (buf[0] == '[') {
		/* find ']' */
		ptr1 = buf + 1;
		ptr1 = strchr(buf, ']');
		if (!ptr1)
			return -1;

		/* get IP address */
		*ptr1 = 0;
		if (ip_addr_from_str((ip_addr_t *)ip, buf + 1))
			return -1;
		
		/* skip "]:" */
		ptr1 += 2;
		ptr2 = ptr1;
		while (*ptr2) {
			if (isspace(*ptr2))
				break;
			
			if (!isdigit(*ptr2))
				return -1;
			ptr2++;
		}

		port = atoi(ptr1);
		if (port < 0 || port > 65535)
			return -1;
		ip->port = htons(port);
	}
	/* IPv4 format: x.x.x.x:x */
	else {
		ptr1 = strchr(buf, ':');
		if (!ptr1)
			return -1;
		*ptr1 = 0;
		if (ip_addr_from_str((ip_addr_t *)ip, buf))
			return -1;

		ptr1++;
		ptr2 = ptr1;
		while (*ptr2) {
			if (!isspace(*ptr2))
				break;
			if (!isdigit(*ptr2))
				return -1;
			ptr2++;
		}

		port = atoi(ptr1);
		if (port < 0 || port > 65535)
			return -1;

		ip->port = htons(port);
	}

	return 0;
}

char * 
ip_port_to_str(const ip_port_t *ip, char *buf, size_t len)
{
	char str[IP_STR_LEN];
	u_int16_t port;

	if (unlikely(!ip || !buf || len < IP_STR_LEN))
		ERR_RET(NULL, "invalid argument\n");

	if (!ip_addr_to_str((ip_addr_t *)ip, str, sizeof(str)))
		return NULL;
	
	port = ntohs(ip->port);
	snprintf(buf, len - 1, "[%s]:%u", str, port);
	buf[len - 1] = 0;

	return buf;
}

int 
ip_port_compare(const ip_port_t *ip1, const ip_port_t *ip2)
{
	int ret;

	if (unlikely(ip1 && !ip2))
		return 1;

	if (unlikely(!ip1 && ip2))
		return -1;

	if (unlikely(ip1 == ip2))
		return 0;

	ret = ip_addr_compare((ip_addr_t *)ip1, (ip_addr_t *)ip2);
	if (ret)
		return ret;

	return (ntohs(ip1->port) - ntohs(ip2->port));
}

