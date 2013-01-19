/****************************************************************************
 *
 * Copyright (C) 2003-2008 Sourcefire, Inc.
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
 
#ifndef _FLOW_H
#define _FLOW_H

#include "util.h"

#undef inet_ntoa

#define FROM_INITIATOR 1
#define FROM_RESPONDER 2

/* flow flags */
#define FLOW_REVERSED 0x00000001 /**< this flow was swapped */
#define FLOW_CLOSEME  0x00000002 /**< shutdown this flow ASAP */

typedef struct _FLOWDATA
{
    BITOP boFlowbits;
    unsigned char flowb[1];
} FLOWDATA;

typedef enum {
    FLOW_NEW, /**< first packet in flow */
    FLOW_FIRST_BIDIRECTIONAL,  /**< first response packet in flow */
    FLOW_ADDITIONAL, /**< additional data on an existing flow */
    FLOW_SHUTDOWN,  /**< shutdown of a existing flow due to timeout or protocol layer */
    FLOW_MAX /** this should not be used and should always be the
                 biggest in the enum for flow_callbacks() */
} FLOW_POSITION;

typedef struct _FLOWKEY
{
    u_int32_t init_address;
    u_int32_t resp_address;
    u_int16_t init_port;
    u_int16_t resp_port;
    u_int8_t  protocol;
} FLOWKEY;

typedef struct _FLOWSTATS
{
    time_t first_packet;
    time_t last_packet;

    u_int32_t packets_sent;
    u_int32_t packets_recv;

    u_int32_t bytes_sent;
    u_int32_t bytes_recv;

    u_int32_t flow_flags; /* normal, timeout, etc. */
    
    char first_talker;
    char last_talker;    
    u_int16_t alerts_seen;

    char direction;

} FLOWSTATS;

typedef struct _FLOW
{
    FLOWKEY key; 
    FLOWSTATS stats;
    FLOWDATA data;
} FLOW;

typedef enum {
    HASH1 = 1,
    HASH2 = 2
} FLOWHASHID;
    

int flow_init(FLOW *flow, char protocol,
              u_int32_t init_address, u_int16_t init_port,
              u_int32_t resp_address, u_int16_t resp_port);

int flow_alloc(int family, FLOW **flow, int *size);

/** 
 * Mark a flow with a particular flag
 * 
 * @param flow 
 * @param flags 
 */
static INLINE void flow_mark(FLOW *f, int flags)
{
    f->stats.flow_flags |= flags;
}

/** 
 * Check to see if a particular flag exists
 * 
 * @param flow 
 * @param flags 
 */
static INLINE int flow_checkflag(FLOW *f, u_long flags)
{
    return ((f->stats.flow_flags & flags) == flags);
}



#endif /* _FLOW_H */
/****************************************************************************
 *
 * Copyright (C) 2003-2008 Sourcefire, Inc.
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
 
#ifndef _FLOW_PRINT_H
#define _FLOW_PRINT_H

#include "util.h"

int flow_printf(const char *format, ...);
NORETURN void flow_fatalerror(const char *format, ...);
NORETURN void flow_errormsg(const char *format, ...);
int flow_set_daemon(void);


#endif /* _FLOW_PRINT_H */

/****************************************************************************
 *
 * Copyright (C) 2003-2008 Sourcefire, Inc.
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
 
/**
 * @file   flow_packet.h
 * @author Chris Green <cmg@sourcefire.com>
 * @date   Wed Jun 25 09:20:41 2003
 * 
 * @brief  interface for packet structures between snort and flow
 *
 *
 * Camel Hump notation for cleaner integration w/ snort
 * 
 * 
 */

#ifndef _FLOW_PACKET_H
#define _FLOW_PACKET_H

#include "decode.h"
#include "util.h"
#include <string.h>

typedef Packet FLOWPACKET;

/** 
 * Determine if this is an IPV4 packet
 * 
 * @param p packet to determine if it's ipv4
 * 
 * @return 1 if it is an IPv4 Packet, 0 otherwise
 */
static int INLINE IsIPv4Packet(FLOWPACKET *p)
{
    FLOWASSERT(p);

    if(p && p->iph)
        return 1;
    
    return 0;
}

/** 
 * Determine if this is an Tcp packet
 * 
 * @param p packet to determine if it's tcp
 * 
 * @return 1 if it is an tcp Packet, 0 otherwise
 */
static int INLINE IsTcpPacket(FLOWPACKET *p)
{
    FLOWASSERT(p);

    if(p && p->tcph)
        return 1;

    return 0;
}

/** 
 * Determine if this is an Tcp packet
 * 
 * @param p packet to determine if it's tcp
 * 
 * @return 1 if it is an tcp Packet, 0 otherwise
 */
static u_int8_t INLINE GetTcpFlags(FLOWPACKET *p)
{
    FLOWASSERT(p && p->tcph);
    
    if(p && p->tcph)
        return p->tcph->th_flags;

    return 0;
}


/** 
 * Returns the Source Port portion of a packet in host byte
 * order.
 *
 * This function assumes that there this packet is has been properly
 * identified to contain an IPv4 Header.
 * 
 * @param p packet 
 * 
 * @return the sport || 0
 */
static u_int16_t INLINE GetIPv4SrcPort(FLOWPACKET *p)     
{
    FLOWASSERT(p);

    if(p)
        return p->sp;

    return 0;
}


/** 
 * Returns the Destination Port portion of a packet in host byte
 * order.
 *
 * This function assumes that there this packet is has been properly
 * identified to contain an IPv4 Header.
 * 
 * @param p packet 
 * 
 * @return the sport || 0
 */
static u_int16_t INLINE GetIPv4DstPort(FLOWPACKET *p)     
{
    FLOWASSERT(p);
    
    if(p)
        return p->dp;

    return 0;
}


/** 
 * Returns the IP Protocol portion of a packet.
 *
 * This function assumes that there this packet is has been properly
 * identified to contain an IPv4 Header.
 * 
 * @param p packet 
 * 
 * @return the sport || 0
 */
static u_int8_t INLINE GetIPv4Proto(FLOWPACKET *p)     
{
    FLOWASSERT(p && p->iph);
        
    if(p && p->iph)
        return p->iph->ip_proto;

    return 0;
}

/** 
 * Returns the SIP portion of a packet.
 *
 * This function assumes that there this packet is has been properly
 * identified to contain an IPv4 Header.
 *
 * This performs memcpy's incase the IPH is not aligned in snort.
 * 
 * @param p packet 
 * 
 * @return the sport || 0
 */
static u_int32_t INLINE GetIPv4SrcIp(FLOWPACKET *p)     
{
    FLOWASSERT(p && p->iph);
    
    if(p && p->iph)
        return p->iph->ip_src.s_addr;
    
    return 0;
}


/** 
 * Returns the DIP portion of a packet.
 *
 * This function assumes that there this packet is has been properly
 * identified to contain an IPv4 Header.
 *
 * This performs memcpy's incase the IPH is not aligned in snort.
 * 
 * @param p packet 
 * 
 * @return the sport || 0
 */
static u_int32_t INLINE GetIPv4DstIp(FLOWPACKET *p)     
{
    FLOWASSERT(p && p->iph);
    
    if(p && p->iph)
        return p->iph->ip_dst.s_addr;

    return 0;
}


/** 
 * Get the IP length of a packet.  
 * 
 * @param p packet to operate on
 * 
 * @return size of the packet
 */
static int INLINE GetIPv4Len(FLOWPACKET *p)
{
    FLOWASSERT(p);

    if(p)
    {
        if(p->iph)
            return ntohs(p->iph->ip_len);
        else
            return p->dsize;
    }

    return 0;
}


int flowkey_reverse(FLOWKEY *key);
int flowkey_make(FLOWKEY *key, FLOWPACKET *p);
int flowkey_print(FLOWKEY *key);
int flowkey_normalize(FLOWKEY *dst, const FLOWKEY *src);
int flowkeycmp_fcn(const void *s1, const void *s2, size_t n);

#endif /* _FLOW_PACKET_H */

/****************************************************************************
 *
 * Copyright (C) 2003-2008 Sourcefire, Inc.
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
 
#ifndef _FLOW_ERROR_H
#define _FLOW_ERROR_H

#define FLOW_SUCCESS  0
#define FLOW_ENULL    1
#define FLOW_EINVALID 2
#define FLOW_ENOMEM   3
#define FLOW_NOTFOUND 4
#define FLOW_BADJUJU  5
#define FLOW_DISABLED 6

#endif /* _FLOW_ERROR_H */
