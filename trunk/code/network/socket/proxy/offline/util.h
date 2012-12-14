/* $Id$ */
/*
** Copyright (C) 2002-2008 Sourcefire, Inc.
** Copyright (C) 2002 Martin Roesch <roesch@sourcefire.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License Version 2 as
** published by the Free Software Foundation.  You may not use, modify or
** distribute this program under any other version of the GNU General
** Public License.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/


#ifndef __UTIL_H__
#define __UTIL_H__

#define TIMEBUF_SIZE 26

#include "cap.h"
#ifndef WIN32
#include <sys/time.h>
#include <sys/types.h>
#endif /* !WIN32 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "debug.h"
#ifndef DEBUG
    #define FLOWASSERT(a)  
#else
    #include <assert.h>
    #define FLOWASSERT(a) assert(a)
#endif /* DEBUG */

#define ONE_MBYTE (1024 * 1024)
#define ONE_HOUR  3600

#define FULLBITS 0xFFFFFFFF

#ifndef IP_MAXPACKET
#define IP_MAXPACKET 65535
#endif

#ifndef WIN32
/* for inet_ntoa */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif /* WIN32 */


#include "sf_types.h"
#include "sfhashfcn.h"

/* specifies that a function does not return 
 * used for quieting Visual Studio warnings
 */
#ifdef _MSC_VER
#if _MSC_VER >= 1400
#define NORETURN __declspec(noreturn)
#else
#define NORETURN
#endif
#else
#define NORETURN
#endif

#define CSE_IP    0x01
#define CSE_TCP   0x02
#define CSE_UDP   0x04
#define CSE_ICMP  0x08
#define CSE_IGMP  0x10


#define SNORT_SNPRINTF_SUCCESS 0
#define SNORT_SNPRINTF_TRUNCATION 1
#define SNORT_SNPRINTF_ERROR -1

#define SNORT_STRNCPY_SUCCESS 0
#define SNORT_STRNCPY_TRUNCATION 1
#define SNORT_STRNCPY_ERROR -1

#define SNORT_STRNLEN_ERROR -1

#define SECONDS_PER_DAY  86400  /* number of seconds in a day  */
#define SECONDS_PER_HOUR  3600  /* number of seconds in a hour */
#define SECONDS_PER_MIN     60     /* number of seconds in a minute */



#define TIMERSUB(a, b, result)                                                \
  do {                                                                        \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;                             \
    (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;                          \
    if ((result)->tv_usec < 0) {                                              \
      --(result)->tv_sec;                                                     \
      (result)->tv_usec += 1000000;                                           \
    }                                                                         \
  } while (0)


extern u_long netmasks[33];

/* Self preservation memory control struct */
typedef struct _SPMemControl
{
    unsigned long memcap;
    unsigned long mem_usage;
    void *control;
    int (*sp_func)(struct _SPMemControl *);

    unsigned long fault_count;

} SPMemControl;

typedef struct _PcapPktStats
{
    UINT64 recv;
    UINT64 drop;
    u_int32_t wrap_recv;
    u_int32_t wrap_drop;

} PcapPktStats;


typedef struct _IntervalStats
{
    UINT64 recv, recv_total;
    UINT64 drop, drop_total;
    UINT64 processed, processed_total;
    UINT64 tcp, tcp_total;
    UINT64 udp, udp_total;
    UINT64 icmp, icmp_total;
    UINT64 arp, arp_total;
    UINT64 ipx, ipx_total;
    UINT64 eapol, eapol_total;
    UINT64 ipv6, ipv6_total;
    UINT64 ethloopback, ethloopback_total;
    UINT64 other, other_total;
    UINT64 frags, frags_total;
    UINT64 discards, discards_total;
    UINT64 frag_trackers, frag_trackers_total;
    UINT64 frag_rebuilt, frag_rebuilt_total;
    UINT64 frag_element, frag_element_total;
    UINT64 frag_incomp, frag_incomp_total;
    UINT64 frag_timeout, frag_timeout_total;
    UINT64 frag_mem_faults, frag_mem_faults_total;
    UINT64 tcp_str_packets, tcp_str_packets_total;
    UINT64 tcp_str_trackers, tcp_str_trackers_total;
    UINT64 tcp_str_flushes, tcp_str_flushes_total;
    UINT64 tcp_str_segs_used, tcp_str_segs_used_total;
    UINT64 tcp_str_segs_queued, tcp_str_segs_queued_total;
    UINT64 tcp_str_mem_faults, tcp_str_mem_faults_total;

#ifdef GRE
    UINT64 ip4ip4, ip4ip4_total;
    UINT64 ip4ip6, ip4ip6_total;
    UINT64 ip6ip4, ip6ip4_total;
    UINT64 ip6ip6, ip6ip6_total;

    UINT64 gre, gre_total;
    UINT64 gre_ip, gre_ip_total;
    UINT64 gre_eth, gre_eth_total;
    UINT64 gre_arp, gre_arp_total;
    UINT64 gre_ipv6, gre_ipv6_total;
    UINT64 gre_ipx, gre_ipx_total;
    UINT64 gre_loopback, gre_loopback_total;
    UINT64 gre_vlan, gre_vlan_total;
    UINT64 gre_ppp, gre_ppp_total;
#endif

#ifdef DLT_IEEE802_11
    UINT64 wifi_mgmt, wifi_mgmt_total;
    UINT64 wifi_control, wifi_control_total;
    UINT64 wifi_data, wifi_data_total;
#endif

} IntervalStats;



int DisplayBanner();
void GetTime(char *);
int gmt2local(time_t);
void ts_print(register const struct timeval *, char *);
char *copy_argv(char **);
void strip(char *);
double CalcPct(UINT64, UINT64);
void ReadPacketsFromFile();
void GenHomenet(char *);
void InitNetmasks();
void InitBinFrag();
void GoDaemon();
void SignalWaitingParent();
void CheckLogDir();
char *read_infile(char *);
void InitProtoNames();
void CleanupProtoNames();
void ErrorMessage(const char *, ...);
void LogMessage(const char *, ...);
NORETURN void FatalError(const char *, ...);
void CreatePidFile(char *);
void ClosePidFile();
void SetUidGid(void);
void SetChroot(char *, char **);
void DropStats(int);
void GenObfuscationMask(char *);
void *SPAlloc(unsigned long, struct _SPMemControl *);
int SnortSnprintf(char *, size_t, const char *, ...);
int SnortSnprintfAppend(char *, size_t, const char *, ...);
char *SnortStrdup(const char *);
int SnortStrncpy(char *, const char *, size_t);
char *SnortStrndup(const char *, size_t);
int SnortStrnlen(const char *, int);
const char *SnortStrcasestr(const char *s, const char *substr);
void *SnortAlloc(unsigned long);
void *SnortAlloc2(size_t, const char *, ...);
char *CurrentWorkingDir(void);
char *GetAbsolutePath(char *dir);
char *StripPrefixDir(char *prefix, char *dir);
void DefineAllIfaceVars();
void DefineIfaceVar(char *,u_char *, u_char *);
#ifdef TIMESTATS
void DropStatsPerTimeInterval(void);
void ResetTimeStats(void);
#endif
#define PCAP_CLOSE  // allow for rollback for now
#ifdef PCAP_CLOSE
/* cacheReturn = 0 is normal operation; 1 will cause the
 * return value to be returned on the next call with 0 */
int UpdatePcapPktStats(int cacheReturn);
#else
int UpdatePcapPktStats(void);
#endif
UINT64 GetPcapPktStatsRecv(void);
UINT64 GetPcapPktStatsDrop(void);
void TimeStats(void);
#ifndef WIN32
SF_LIST * SortDirectory(const char *);
int GetFilesUnderDir(const char *, SF_QUEUE *, const char *);
#endif

inline unsigned short in_chksum_ip(  unsigned short * w, int blen );
inline unsigned short in_chksum_tcp(  unsigned short *h, unsigned short * d, int dlen );
inline unsigned short in_chksum_tcp6(  unsigned short *h, unsigned short * d, int dlen );
inline unsigned short in_chksum_udp6(  unsigned short *h, unsigned short * d, int dlen );
inline unsigned short in_chksum_udp(  unsigned short *h, unsigned short * d, int dlen );
inline unsigned short in_chksum_icmp( unsigned short * w, int blen );

#define COPY4(x, y) \
    x[0] = y[0]; x[1] = y[1]; x[2] = y[2]; x[3] = y[3];

#define COPY16(x,y) \
    x[0] = y[0]; x[1] = y[1]; x[2] = y[2]; x[3] = y[3]; \
    x[4] = y[4]; x[5] = y[5]; x[6] = y[6]; x[7] = y[7]; \
    x[8] = y[8]; x[9] = y[9]; x[10] = y[10]; x[11] = y[11]; \
    x[12] = y[12]; x[13] = y[13]; x[14] = y[14]; x[15] = y[15];



#define SAFEMEM_ERROR 0
#define SAFEMEM_SUCCESS 1
#include "debug.h"
 #include <string.h>

#ifndef DEBUG
    #define ERRORRET return SAFEMEM_ERROR;
#else
    #define ERRORRET assert(0==1)
#endif /* DEBUG */


static INLINE int inBounds(const u_int8_t *start, const u_int8_t *end, const u_int8_t *p)
{
    if(p >= start && p < end)
    {
        return 1;
    }
    
    return 0;
}

/** 
 * A Safer Memcpy
 * 
 * @param dst where to copy to
 * @param src where to copy from
 * @param n number of bytes to copy
 * @param start start of the dest buffer
 * @param end end of the dst buffer
 * 
 * @return 0 on failure, 1 on success
 */
static INLINE int SafeMemcpy(void *dst, const void *src, size_t n, const void *start, const void *end)
{
    void *tmp;

    if(n < 1)
    {
        ERRORRET;
    }

    if (!dst || !src)
    {
        ERRORRET;
    }

    tmp = ((u_int8_t*)dst) + (n-1);
    if (tmp < dst)
    {
        ERRORRET;
    }

    if(!inBounds(start,end, dst) || !inBounds(start,end,tmp))
    {
        ERRORRET;
    }

    memcpy(dst, src, n);

    return SAFEMEM_SUCCESS;
}

/** 
 * A Safer *a = *b
 * 
 * @param start start of the dst buffer
 * @param end end of the dst buffer
 * @param dst the location to write to
 * @param src the source to read from
 * 
 * @return 0 on failure, 1 on success
 */
static INLINE int SafeWrite(u_int8_t *start, u_int8_t *end, u_int8_t *dst, u_int8_t *src)
{
    if(!inBounds(start, end, dst))
    {
        ERRORRET;
    }
     
    *dst = *src;        
    return 1;
}

static inline int SafeRead(u_int8_t *start, u_int8_t *end, u_int8_t *src, u_int8_t *read)
{
    if(!inBounds(start,end, src))
    {
        ERRORRET;
    }
    
    *read = *start;
    return 1;
}

#endif /* _BOUNDS_H */
/* $Id$ */
/*
 * Copyright (C) 2002-2008 Sourcefire, Inc.
 *
 * Author(s):  Andrew R. Baker <andrewb@sourcefire.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation.  You may not use, modify or
 * distribute this program under any other version of the GNU General
 * Public License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#ifndef __IP_ADDR_SET_H__
#define __IP_ADDR_SET_H__

#include <sys/types.h>

#ifdef SUP_IP6
#include "sf_ipvar.h"
sfip_var_t *IpAddrSetParse(char *);
#else
typedef struct _IpAddrNode
{
    u_int32_t ip_addr;   /* IP addr */
    u_int32_t netmask;   /* netmask */
    u_int8_t  addr_flags; /* flag for normal/exception processing */

    struct _IpAddrNode *next;
} IpAddrNode;

typedef struct _IpAddrSet
{
    IpAddrNode *iplist;
    IpAddrNode *neg_iplist;
} IpAddrSet;

/* flags */
#define EXCEPT_IP   0x01

void IpAddrSetPrint(char *prefix, IpAddrSet *);
void IpAddrSetDestroy(IpAddrSet *);
IpAddrSet *IpAddrSetCopy(IpAddrSet *);
IpAddrSet *IpAddrSetCreate();
IpAddrSet *IpAddrSetParse(char *);
int IpAddrSetContains(IpAddrSet *, struct in_addr);


/* XXX legacy support function */
int ParseIP(char *paddr, IpAddrSet *, int); 
#endif // SUP_IP6
#endif /* __IP_ADDR_SET_H__ */
/*
** Copyright (C) 1998-2008 Sourcefire, Inc.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License Version 2 as
** published by the Free Software Foundation.  You may not use, modify or
** distribute this program under any other version of the GNU General
** Public License.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/*
 * Adam Keeton
 * sf_ip.h
 * 11/17/06
*/

#ifndef SF_IP_H
#define SF_IP_H

#ifndef WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#else
#include <winsock2.h>
#endif
#include "debug.h" /* for INLINE definition */

/* define SFIP_ROBUST to check pointers passed into the sfip libs.
 * Robustification should not be enabled if the client code is trustworthy.
 * Namely, if pointers are checked once in the client, or are pointers to
 * data allocated on the stack, there's no need to check them again here.
 * The intention is to prevent the same stack-allocated variable from being
 * checked a dozen different times. */
#define SFIP_ROBUST

#ifdef SFIP_ROBUST

#define ARG_CHECK1(a, z) if(!a) return z;
#define ARG_CHECK2(a, b, z) if(!a || !b) return z;
#define ARG_CHECK3(a, b, c, z) if(!a || !b || !c) return z;

#elif defined(DEBUG)

#define ARG_CHECK1(a, z) assert(a);
#define ARG_CHECK2(a, b, z) assert(a); assert(b);
#define ARG_CHECK3(a, b, c, z) assert(a); assert(b); assert(c);

#else

#define ARG_CHECK1(a, z) 
#define ARG_CHECK2(a, b, z) 
#define ARG_CHECK3(a, b, c, z)

#endif

typedef struct _ip {
    int family;

    union
    {
        u_int8_t  u6_addr8[16];
        u_int16_t u6_addr16[8];
        u_int32_t u6_addr32[4];
//        UINT64    u6_addr64[2];
    } ip;
    #define ip8  ip.u6_addr8
    #define ip16 ip.u6_addr16
    #define ip32 ip.u6_addr32
//    #define ip64 ip.u6_addr64

    int bits;
} sfip_t;

typedef enum _return_values {
    SFIP_SUCCESS=0,
    SFIP_FAILURE,
    SFIP_LESSER,
    SFIP_GREATER,
    SFIP_EQUAL,
    SFIP_ARG_ERR,
    SFIP_CIDR_ERR,
    SFIP_INET_PARSE_ERR,
    SFIP_INVALID_MASK,
    SFIP_ALLOC_ERR,
    SFIP_CONTAINS,
    SFIP_NOT_CONTAINS,
    SFIP_DUPLICATE,         /* Tried to add a duplicate variable name to table */
    SFIP_LOOKUP_FAILURE,    /* Failed to lookup a variable from the table */
    SFIP_UNMATCHED_BRACKET, /* IP lists that are missing a closing bracket */
    SFIP_NOT_ANY,           /* For !any */
    SFIP_CONFLICT           /* For IP conflicts in IP lists */
} SFIP_RET;


/* IP allocations and setting ******************************************/

/* Parses "src" and stores results in "dst" */
/* If the conversion is invalid, returns SFIP_FAILURE */
SFIP_RET sfip_pton(char *src, sfip_t *dst);

/* Allocate IP address from a character array describing the IP */
sfip_t *sfip_alloc(char *ip, SFIP_RET *status);

/* Frees an sfip_t */
void sfip_free(sfip_t *ip);

/* Allocate IP address from an array of integers.  The array better be 
 * long enough for the given family! */
sfip_t *sfip_alloc_raw(void *ip, int family, SFIP_RET *status);

/* Sets existing IP, "dst", to a raw source IP (4 or 16 bytes, 
 * according to family) */
SFIP_RET sfip_set_raw(sfip_t *dst, void *src, int src_family);

/* Sets existing IP, "dst", to be source IP, "src" */
SFIP_RET sfip_set_ip(sfip_t *dst, sfip_t *src);

/* Obfuscates an IP */
void sfip_obfuscate(sfip_t *ob, sfip_t *ip);



/* Member-access *******************************************************/

/* Returns the family of "ip", either AF_INET or AF_INET6 */
/* XXX This is a performance critical function,
*  need to determine if it's safe to not check these pointers */
// ARG_CHECK1(ip, 0);
#define sfip_family(ip) ip->family

/* Returns the number of bits used for masking "ip" */
static INLINE unsigned char sfip_bits(sfip_t *ip) {
    ARG_CHECK1(ip, 0);
    return ip->bits;
}   

static INLINE void sfip_set_bits(sfip_t *p, int bits) {

    if(!p)
        return;

    if(bits < 0 || bits > 128) return;

    p->bits = bits;
}

///* Returns the raw IP address as an in6_addr */
//inline struct in6_addr sfip_to_raw(sfip_t *);



/* IP Comparisons ******************************************************/

/* Check if ip is contained within the network specified by net */ 
/* Returns SFIP_EQUAL if so */
SFIP_RET sfip_contains(sfip_t *net, sfip_t *ip);

/* Returns 1 if the IP is non-zero. 0 otherwise */
/* XXX This is a performance critical function, \
 *  need to determine if it's safe to not check these pointers */\
static INLINE int sfip_is_set(sfip_t *ip) {
//    ARG_CHECK1(ip, -1);
    return ip->ip32[0] || 
            ( (ip->family == AF_INET6) && 
              (ip->ip32[1] || 
              ip->ip32[2] || 
              ip->ip32[3]) ) ;
}

/* Return 1 if the IP is a loopback IP */
int sfip_is_loopback(sfip_t *ip);

/* Returns 1 if the IPv6 address appears mapped. 0 otherwise. */
int sfip_ismapped(sfip_t *ip);

/* Support function for sfip_compare */
static INLINE int _ip4_cmp(u_int32_t ip1, u_int32_t ip2) {
    if(ip1 < ip2) return SFIP_LESSER;
    if(ip1 > ip2) return SFIP_GREATER;
    return SFIP_EQUAL;
}

/* Support function for sfip_compare */
static INLINE int _ip6_cmp(sfip_t *ip1, sfip_t *ip2) {
    SFIP_RET ret;
    u_int32_t *p1, *p2; 

    /* XXX
     * Argument are assumed trusted!
     * This function is presently only called by sfip_compare 
     * on validated pointers.
     * XXX */

    p1 = ip1->ip32;
    p2 = ip2->ip32;

    if( (ret = _ip4_cmp(p1[0], p2[0])) != SFIP_EQUAL) return ret;
    if( (ret = _ip4_cmp(p1[1], p2[1])) != SFIP_EQUAL) return ret;
    if( (ret = _ip4_cmp(p1[2], p2[2])) != SFIP_EQUAL) return ret;
    if( (ret = _ip4_cmp(p1[3], p2[3])) != SFIP_EQUAL) return ret;

    return ret;
}

/* Compares two IPs 
 * Returns SFIP_LESSER, SFIP_EQUAL, SFIP_GREATER, if ip1 is less than, equal to, 
 * or greater than ip2 In the case of mismatched families, the IPv4 address 
 * is converted to an IPv6 representation. */
/* XXX-IPv6 Should add version of sfip_compare that just tests equality */
static INLINE SFIP_RET sfip_compare(sfip_t *ip1, sfip_t *ip2) {
    int f1,f2;

    ARG_CHECK2(ip1, ip2, SFIP_ARG_ERR);

    /* This is being done because at some points in the existing Snort code,
     * an unset IP is considered to match anything.  Thus, if either IP is not
     * set here, it's considered equal. */
    if(!sfip_is_set(ip1) || !sfip_is_set(ip2)) return SFIP_EQUAL;

    f1 = sfip_family(ip1);
    f2 = sfip_family(ip2);

    if(f1 == AF_INET && f2 == AF_INET) {
        return _ip4_cmp(*ip1->ip32, *ip2->ip32);
    } 
/* Mixed families not presently supported */
#if 0
    else if(f1 == AF_INET && f2 == AF_INET6) {
        conv = sfip_4to6(ip1);
        return _ip6_cmp(&conv, ip2);
    } else if(f1 == AF_INET6 && f2 == AF_INET) {
        conv = sfip_4to6(ip2);
        return _ip6_cmp(ip1, &conv);
    } 
    else {
        return _ip6_cmp(ip1, ip2);
    }
#endif
    else if(f1 == AF_INET6 && f2 == AF_INET6) {
        return _ip6_cmp(ip1, ip2);
    }

    return SFIP_FAILURE;
}

static INLINE int sfip_fast_lt4(sfip_t *ip1, sfip_t *ip2) {
    return *ip1->ip32 < *ip2->ip32;
}
static INLINE int sfip_fast_gt4(sfip_t *ip1, sfip_t *ip2) {
    return *ip1->ip32 > *ip2->ip32;
}
static INLINE int sfip_fast_eq4(sfip_t *ip1, sfip_t *ip2) {
    return *ip1->ip32 == *ip2->ip32;
}

static INLINE int sfip_fast_lt6(sfip_t *ip1, sfip_t *ip2) {
    u_int32_t *p1, *p2; 

    p1 = ip1->ip32;
    p2 = ip2->ip32;

    if(*p1 < *p2) return 1;
    else if(*p1 > *p2) return 0;

    if(p1[1] < p2[1]) return 1;
    else if(p1[1] > p2[1]) return 0;

    if(p1[2] < p2[2]) return 1;
    else if(p1[2] > p2[2]) return 0;

    if(p1[3] < p2[3]) return 1;
    else if(p1[3] > p2[3]) return 0;

    return 0;
}

static INLINE int sfip_fast_gt6(sfip_t *ip1, sfip_t *ip2) {
    u_int32_t *p1, *p2; 

    p1 = ip1->ip32;
    p2 = ip2->ip32;

    if(*p1 > *p2) return 1;
    else if(*p1 < *p2) return 0;

    if(p1[1] > p2[1]) return 1;
    else if(p1[1] < p2[1]) return 0;

    if(p1[2] > p2[2]) return 1;
    else if(p1[2] < p2[2]) return 0;

    if(p1[3] > p2[3]) return 1;
    else if(p1[3] < p2[3]) return 0;

    return 0;
}

static INLINE int sfip_fast_eq6(sfip_t *ip1, sfip_t *ip2) {
    u_int32_t *p1, *p2; 

    p1 = ip1->ip32;
    p2 = ip2->ip32;

    if(*p1 != *p2) return 0;
    if(p1[1] != p2[1]) return 0;
    if(p1[2] != p2[2]) return 0;
    if(p1[3] != p2[3]) return 0;

    return 1;
}

/* Checks if ip2 is equal to ip1 or contained within the CIDR ip1 */
static INLINE int sfip_fast_cont4(sfip_t *ip1, sfip_t *ip2) {
    u_int32_t shift = 32 - sfip_bits(ip1);
    u_int32_t ip = *ip2->ip32;

#ifdef WORDS_BIGENDIAN
    ip >>= shift;
    ip <<= shift;
#else
    ip <<= shift;
    ip >>= shift;
#endif

    return *ip1->ip32 == ip;
}

/* Checks if ip2 is equal to ip1 or contained within the CIDR ip1 */
static INLINE int sfip_fast_cont6(sfip_t *ip1, sfip_t *ip2) {
    u_int32_t bits = sfip_bits(ip1);
    u_int32_t ip;

    /* Divide bits by 32 to puts it in units of words to determine
     * which words are, and are not, masked */
    switch((bits / 32)) {
        /* 0 to 31 bits */
        case 0:
            ip = *ip2->ip32;
            ip <<= 32 - bits;
            ip >>= 32 - bits;
            return *ip1->ip32 == ip;

        /* 32 to 63 bits */
        case 1:
            if(!sfip_fast_eq4(ip1, ip2)) 
                return 0;

            ip = ip2->ip32[1];

            ip <<= 32 - bits;
            ip >>= 32 - bits;
            return ip1->ip32[1] == ip;
        
        /* 64 to 95 bits */
        case 2:
            if(!sfip_fast_eq4(ip1, ip2)) 
                return 0;

            if(!(ip1->ip32[1] && ip2->ip32[1]))
                return 0;

            ip = ip2->ip32[2];

            ip <<= 32 - bits;
            ip >>= 32 - bits;
            return ip1->ip32[2] == ip;


        /* 96 to 127 bits */
        case 3:
            if(!sfip_fast_eq4(ip1, ip2)) 
                return 0;

            if(!(ip1->ip32[1] && ip2->ip32[1]))
                return 0;

            if(!(ip1->ip32[2] && ip2->ip32[2]))
                return 0;

            ip = ip2->ip32[3];

            ip <<= 32 - bits;
            ip >>= 32 - bits;
            return ip1->ip32[3] == ip;

        /* 128 bits */
        case 4:
            return sfip_fast_eq6(ip1, ip2);

        /* Black magic bits */
        default:
            return 0;
    };      
}

#define sfip_equals(x,y) (sfip_compare(&x, &y) == SFIP_EQUAL)
#define sfip_not_equals !sfip_equals
#define sfip_clear(x) memset(x, 0, 16)



/* Printing ************************************************************/

/* Uses a static buffer to return a string representation of the IP */
char *sfip_to_str(sfip_t *ip);
#define sfip_ntoa(x) sfip_to_str(x)
void sfip_raw_ntop(int family, const void *ip_raw, char *buf, int bufsize);

#ifndef strndup
char *strndup(const char *s, size_t n);
#endif

#ifndef inet_pton
int inet_pton(int af, const char *src, void *dst);
#endif

#endif
/*  
** Copyright (C) 2007-2008 Sourcefire, Inc.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License Version 2 as
** published by the Free Software Foundation.  You may not use, modify or
** distribute this program under any other version of the GNU General
** Public License.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if nto, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Bosotn, MA 02111-1307, USA.
*/

#ifndef SFIPH_H
#define SFIPH_H

#ifdef SUP_IP6
void sfiph_build(Packet *p, const void *hdr, int family);
void sfiph_orig_build(Packet *p, const void *hdr, int family);
#endif

#endif
/*
** Copyright (C) 2007-2008 Sourcefire, Inc.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License Version 2 as
** published by the Free Software Foundation.  You may not use, modify or
** distribute this program under any other version of the GNU General
** Public License.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef IPV6_PORT_H
#define IPV6_PORT_H

///////////////////
/* IPv6 and IPv4 */
#ifdef SUP_IP6

#include "sf_ip.h"

typedef sfip_t snort_ip;
typedef sfip_t *snort_ip_p;

#define IpAddrNode sfip_node_t
#define IpAddrSet sfip_var_t
#define IpAddrSetContains(x,y) sfvar_ip_in(x, y)
#define IpAddrSetPrint sfvar_print

#define inet_ntoa sfip_ntoa

#define GET_SRC_IP(p) (p->iph_api.iph_ret_src(p))
#define GET_DST_IP(p) (p->iph_api.iph_ret_dst(p))

#define GET_ORIG_SRC(p) (p->iph_api.orig_iph_ret_src(p))
#define GET_ORIG_DST(p) (p->iph_api.orig_iph_ret_dst(p))

/* These are here for backwards compatibility */
#define GET_SRC_ADDR(x) GET_SRC_IP(x)
#define GET_DST_ADDR(x) GET_DST_IP(x)

#define IP_EQUALITY(x,y) (sfip_compare(x,y) == SFIP_EQUAL)
#define IP_LESSER(x,y)   (sfip_compare(x,y) == SFIP_LESSER)
#define IP_GREATER(x,y)  (sfip_compare(x,y) == SFIP_GREATER)

#define GET_IPH_TOS(p)   p->iph_api.iph_ret_tos(p)
#define GET_IPH_LEN(p)   p->iph_api.iph_ret_len(p)
#define GET_IPH_TTL(p)   p->iph_api.iph_ret_ttl(p)
#define GET_IPH_ID(p)    p->iph_api.iph_ret_id(p)
#define GET_IPH_OFF(p)   p->iph_api.iph_ret_off(p)
#define GET_IPH_VER(p)   p->iph_api.iph_ret_ver(p)
#define GET_IPH_PROTO(p) p->iph_api.iph_ret_proto(p)

#define GET_ORIG_IPH_PROTO(p)   p->iph_api.orig_iph_ret_proto(p)
#define GET_ORIG_IPH_VER(p)     p->iph_api.orig_iph_ret_ver(p)
#define GET_ORIG_IPH_LEN(p)     p->iph_api.orig_iph_ret_len(p)
#define GET_ORIG_IPH_OFF(p)     p->iph_api.orig_iph_ret_off(p)
#define GET_ORIG_IPH_PROTO(p)   p->iph_api.orig_iph_ret_proto(p)

#define IS_IP4(x) (x->family == AF_INET)
#define IS_IP6(x) (x->family == AF_INET6)
/* XXX make sure these aren't getting confused with sfip_is_valid within the code */
#define IPH_IS_VALID(p) iph_is_valid(p)

#define IP_CLEAR(x) x.bits = x.family = x.ip32[0] = x.ip32[1] = x.ip32[2] = x.ip32[3] = 0;

#define IS_SET(x) sfip_is_set(&x)

/* This loop trickery is intentional.  If each copy is performed 
 * individually on each field, then the following expression gets broken:
 *  
 *      if(conditional) IP_COPY_VALUE(a,b);
 *      
 * If the macro is instead enclosed in braces, then having a semicolon
 * trailing the macro causes compile breakage. 
 * So: use loop. */
#define IP_COPY_VALUE(x,y) \
        do {  \
                x.bits = y->bits; \
                x.family = y->family; \
                x.ip32[0] = y->ip32[0]; \
                x.ip32[1] = y->ip32[1]; \
                x.ip32[2] = y->ip32[2]; \
                x.ip32[3] = y->ip32[3]; \
        } while(0)

#define GET_IPH_HLEN(p) (p->iph_api.iph_ret_hlen(p))
#define SET_IPH_HLEN(p, val)

#define GET_IP_DGMLEN(p) IS_IP6(p) ? (ntohs(GET_IPH_LEN(p)) + (GET_IPH_HLEN(p) << 2)) : ntohs(GET_IPH_LEN(p))
#define GET_IP_PAYLEN(p) IS_IP6(p) ? ntohs(GET_IPH_LEN(p)) : (ntohs(GET_IPH_LEN(p)) - (GET_IPH_HLEN(p) << 2))

#else
///////////////
/* IPv4 only */
#include <sys/types.h>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

typedef u_int32_t snort_ip; /* 32 bits only -- don't use unsigned long */
typedef u_int32_t snort_ip_p; /* 32 bits only -- don't use unsigned long */

#define IP_SRC_EQUALITY(x,y) (x->ip_addr == (y->iph->ip_src.s_addr & x->netmask))
#define IP_DST_EQUALITY(x,y) (x->ip_addr == (y->iph->ip_dst.s_addr & x->netmask))

#define GET_SRC_IP(x) x->iph->ip_src.s_addr
#define GET_DST_IP(x) x->iph->ip_dst.s_addr

#define GET_ORIG_SRC(p) (p->orig_iph->ip_src.s_addr)
#define GET_ORIG_DST(p) (p->orig_iph->ip_dst.s_addr)

#define GET_SRC_ADDR(x) x->iph->ip_src
#define GET_DST_ADDR(x) x->iph->ip_dst

#define IP_CLEAR_SRC(x) x->iph->ip_src.s_addr = 0
#define IP_CLAR_DST(x) x->iph->ip_dst.s_addr = 0

#define IP_EQUALITY(x,y) (x == y)
#define IP_LESSER(x,y) (x < y)
#define IP_GREATER(x,y) (x > y)

#define GET_IPH_PROTO(p) p->iph->ip_proto
#define GET_IPH_TOS(p) p->iph->ip_tos
#define GET_IPH_LEN(p) p->iph->ip_len
#define GET_IPH_TTL(p) p->iph->ip_ttl
#define GET_IPH_VER(p) ((p->iph->ip_verhl & 0xf0) >> 4)
#define GET_IPH_ID(p) p->iph->ip_id
#define GET_IPH_OFF(p) p->iph->ip_off

#define GET_ORIG_IPH_VER(p) IP_VER(p->orig_iph)
#define GET_ORIG_IPH_LEN(p) p->orig_iph->ip_len
#define GET_ORIG_IPH_OFF(p) p->orig_iph->ip_off
#define GET_ORIG_IPH_PROTO(p) p->orig_iph->ip_proto

#define IS_IP4(x) 1
#define IS_IP6(x) 0
#define IPH_IS_VALID(p) p->iph

#define IP_CLEAR(x) x = 0;
#define IS_SET(x) x

#define IP_COPY_VALUE(x,y) x = y

#define GET_IPH_HLEN(p) ((p)->iph->ip_verhl & 0x0f)
#define SET_IPH_HLEN(p, val) (((IPHdr *)(p)->iph)->ip_verhl = (unsigned char)(((p)->iph->ip_verhl & 0xf0) | ((val) & 0x0f)))

#define GET_IP_DGMLEN(p) ntohs(GET_IPH_LEN(p))
#define GET_IP_PAYLEN(p) ntohs(GET_IPH_LEN(p)) - (GET_IPH_HLEN(p) << 2)

#endif /* SUP_IP6 */

#if !defined(IPPROTO_IPIP) && defined(WIN32)  /* Needed for some Win32 */
#define IPPROTO_IPIP 4
#endif

#endif /* IPV6_PORT_H */
/*
** Copyright (C) 2002-2008 Sourcefire, Inc.
** Copyright (C) 1998-2002 Martin Roesch <roesch@sourcefire.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License Version 2 as
** published by the Free Software Foundation.  You may not use, modify or
** distribute this program under any other version of the GNU General
** Public License.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/* $Id$ */

#ifndef __MSTRING_H__
#define __MSTRING_H__

/*  I N C L U D E S  ******************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>




/*  P R O T O T Y P E S  *************************************************/
char **mSplit(char *, const char *, int, int *, char);
void mSplitFree(char ***toks, int numtoks);
int mContainsSubstr(const char *, int, const char *, int);
int mSearch(const char *, int, const char *, int, int *, int *);
int mSearchCI(const char *, int, const char *, int, int *, int *);
int mSearchREG(const char *, int, const char *, int, int *, int *);
int *make_skip(char *, int);
int *make_shift(char *, int);




#endif  /* __MSTRING_H__ */
/****************************************************************************
 *
 * Copyright (C) 2005-2008 Sourcefire, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation.  You may not use, modify or
 * distribute this program under any other version of the GNU General
 * Public License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ****************************************************************************/
 
#ifndef _PREPROC_IDS_H_
#define _PREPROC_IDS_H

/*
**  Preprocessor Communication Defines
**  ----------------------------------
**  These defines allow preprocessors to be turned
**  on and off for each packet.  Preprocessors can be
**  turned off and on before preprocessing occurs and
**  during preprocessing.
**
**  Currently, the order in which the preprocessors are
**  placed in the snort.conf determine the order of 
**  evaluation.  So if one module wants to turn off
**  another module, it must come first in the order.
*/
//#define PP_ALL                    0xffffffff
//#define PP_LOADBALANCING          1
//#define PP_PORTSCAN               2
#define PP_HTTPINSPECT            3
//#define PP_PORTSCAN_IGNORE_HOSTS  4
#define PP_RPCDECODE              5
#define PP_BO                     6
#define PP_TELNET                 7
#define PP_STREAM4                8
//#define PP_FRAG2                  9
#define PP_ARPSPOOF               10
//#define PP_ASN1DECODE             11
//#define PP_FNORD                  12
//#define PP_CONVERSATION           13
//#define PP_PORTSCAN2              14
//#define PP_HTTPFLOW               15
#define PP_PERFMONITOR            16
//#define PP_STREAM4_REASSEMBLE     17
#define PP_FRAG3                  18
#define PP_FTPTELNET              19
#define PP_SMTP                   20
#define PP_SFPORTSCAN             21
#define PP_FLOW                   22
#define PP_ISAKMP                 23
#define PP_SSH                    24
#define PP_DNS                    25
#define PP_STREAM5                26
#define PP_DCERPC                 27
#define PP_SKYPE                  28
#define PP_SSL                    29
#define PP_RULES                  30

#define PRIORITY_FIRST 0x0
#define PRIORITY_NETWORK 0x10
#define PRIORITY_TRANSPORT 0x100
#define PRIORITY_TUNNEL  0x105
#define PRIORITY_SCANNER 0x110
#define PRIORITY_APPLICATION 0x200
#define PRIORITY_LAST 0xffff

#endif /* _PREPROC_IDS_H */

