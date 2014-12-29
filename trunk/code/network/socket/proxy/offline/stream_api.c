/* $Id$ */

/*
 * ** Copyright (C) 2005-2008 Sourcefire, Inc.
 * ** AUTHOR: Steven Sturges
 * **
 * ** This program is free software; you can redistribute it and/or modify
 * ** it under the terms of the GNU General Public License Version 2 as
 * ** published by the Free Software Foundation.  You may not use, modify or
 * ** distribute this program under any other version of the GNU General
 * ** Public License.
 * **
 * ** This program is distributed in the hope that it will be useful,
 * ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 * ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * ** GNU General Public License for more details.
 * **
 * ** You should have received a copy of the GNU General Public License
 * ** along with this program; if not, write to the Free Software
 * ** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * */

/* stream_api.c
 *
 * Purpose: Declaration of the StreamAPI global;
 *
 * Arguments:
 *
 * Effect:
 *
 * Comments:
 *
 * Any comments?
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "decode.h"
#include "stream_api.h"

StreamAPI *stream_api = NULL;
/* $Id$ */

/*
** Copyright (C) 2005-2008 Sourcefire, Inc.
** AUTHOR: Steven Sturges
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

/* stream_ignore.c
 * 
 * Purpose: Handle hash table storage and lookups for ignoring
 *          entire data streams.
 *
 * Arguments:
 *   
 * Effect:
 *
 * Comments:
 *
 * Any comments?
 *
 */
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifndef WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif /* WIN32 */
#include <time.h>

#include "debug.h"
#include "decode.h"
#include "stream_api.h"
#include "sfhashfcn.h"
#include "util.h"

/* Reasonably small, and prime */
#define IGNORE_HASH_SIZE 1021
typedef struct _IgnoreNode
{
    snort_ip ip1;
    short port1;
    snort_ip ip2;
    short port2;
    char protocol;
    time_t expires;
    int direction;
    int numOccurances;
} IgnoreNode;

typedef struct _IgnoreHashKey
{
    snort_ip ip1;
    snort_ip ip2;
    short port;
    char protocol;
    char pad;
} IgnoreHashKey;

/* The hash table of ignored channels */
static SFGHASH *channelHash = NULL;

int IgnoreChannel(snort_ip_p cliIP, u_int16_t cliPort,
                  snort_ip_p srvIP, u_int16_t srvPort,
                  char protocol, char direction, char flags,
                  u_int32_t timeout)
{
    IgnoreHashKey hashKey;
    time_t now;
    IgnoreNode *node = NULL;
    short portToHash = cliPort != UNKNOWN_PORT ? cliPort : srvPort;
    snort_ip_p ip1, ip2;
    snort_ip zeroed, oned;
    IP_CLEAR(zeroed);
#ifdef SUP_IP6
    memset(oned.ip8, 1, 16);
    oned.family = cliIP->family;
#else
    oned = 0xffffffff;
#endif

    if (!channelHash)
    {
        /* Create the hash table */
        channelHash = sfghash_new(IGNORE_HASH_SIZE,
                                  sizeof(IgnoreHashKey), 0, free);
    }
   
    time(&now);

    /* Add the info to a tree that marks this channel as one to ignore.
     * Only one of the port values may be UNKNOWN_PORT.  
     * As a sanity check, the IP addresses may not be 0 or 255.255.255.255.
     */
    if ((cliPort == UNKNOWN_PORT) && (srvPort == UNKNOWN_PORT))
        return -1;

#ifdef SUP_IP6
    if (IP_EQUALITY(cliIP, &zeroed) || IP_EQUALITY(cliIP, &oned) ||
        IP_EQUALITY(srvIP, &zeroed) || IP_EQUALITY(srvIP, &oned) )
#else
    if (IP_EQUALITY(cliIP, zeroed) || IP_EQUALITY(cliIP, oned) ||
        IP_EQUALITY(srvIP, zeroed) || IP_EQUALITY(srvIP, oned) )
#endif
        return -1;

    if (IP_LESSER(cliIP, srvIP))
    {
        ip1 = cliIP;
        ip2 = srvIP;
    }
    else
    {
        ip1 = srvIP;
        ip2 = cliIP;
    }

    /* Actually add it to the hash table with a timestamp of now.
     * so we can expire entries that are older than a configurable
     * time.  Those entries will be for sessions that we missed or
     * never occured.  Should not keep the entry around indefinitely.
     */
    IP_COPY_VALUE(hashKey.ip1, ip1);
    IP_COPY_VALUE(hashKey.ip2, ip2);
    hashKey.port = portToHash;
    hashKey.protocol = protocol;
    hashKey.pad = 0;

    node = sfghash_find(channelHash, &hashKey);
    if (node)
    {
        /*
         * This handles the case where there is already an entry
         * for this key (IP addresses/port).  It could occur when
         * multiple users from behind a NAT'd Firewall all go to the
         * same site when in FTP Port mode.  To get around this issue,
         * we keep a counter of the number of pending open channels
         * with the same known endpoints (2 IPs & a port).  When that
         * channel is actually opened, the counter is decremented, and
         * the entry is removed when the counter hits 0.
         * Because all of this is single threaded, there is no potential
         * for a race condition.
         */
        int expired = (node->expires != 0) && (now > node->expires);
        if (expired)
        {
            IP_COPY_VALUE(node->ip1, cliIP);
            node->port1 = cliPort;
            IP_COPY_VALUE(node->ip2, srvIP);
            node->port2 = srvPort;
            node->direction = direction;
            node->protocol = protocol;
        }
        else
        {
            node->numOccurances++;
        }
        if (flags & IGNORE_FLAG_ALWAYS)
            node->expires = 0;
        else
            node->expires = now + timeout;
        DEBUG_WRAP(DebugMessage(DEBUG_STREAM,
                   "Updating ignore channel node\n"););
    }
    else
    {
        DEBUG_WRAP(DebugMessage(DEBUG_STREAM,
                   "Adding ignore channel node\n"););

        node = SnortAlloc(sizeof(IgnoreNode));
        if (!node)
        {
            DEBUG_WRAP(DebugMessage(DEBUG_STREAM, "Memory alloc error\n"););
            return -1;
        }
        IP_COPY_VALUE(node->ip1, cliIP);
        node->port1 = cliPort;
        IP_COPY_VALUE(node->ip2, srvIP);
        node->port2 = srvPort;
        node->direction = direction;
        node->protocol = protocol;
        /* now + 5 minutes (configurable?)
         *
         * use the time that we keep sessions around
         * since this info would effectively be invalid
         * after that anyway because the session that
         * caused this will be gone.
         */
        if (flags & IGNORE_FLAG_ALWAYS)
            node->expires = 0;
        else
            node->expires = now + timeout;
        node->numOccurances = 1;

        /* Add it to the table */
        if (sfghash_add(channelHash, &hashKey, (void *)node)
            != SFGHASH_OK)
        {
            /* Uh, shouldn't get here...
             * There is already a node or couldn't alloc space
             * for key.  This means bigger problems, but fail
             * gracefully.
             */
            DEBUG_WRAP(DebugMessage(DEBUG_STREAM,
                       "Failed to add channel node to hash table\n"););
            free(node);
            return -1;
        }
    }

    return 0;
}

char CheckIgnoreChannel(Packet *p)
{
    snort_ip_p srcIP, dstIP;
    short srcPort, dstPort;
    char protocol;

    IgnoreHashKey hashKey;
    time_t now;
    int match = 0;
    int retVal = 0;
    IgnoreNode *node = NULL;
    int expired = 0;
    int i;

    /* No hash table, or its empty?  Get out of dodge.  */
    if (!channelHash || channelHash->count == 0)
        return retVal;

    srcIP = GET_SRC_IP(p);
    dstIP = GET_DST_IP(p);
    srcPort = p->sp;
    dstPort = p->dp;
    protocol = GET_IPH_PROTO(p);
    
    /* First try the hash table using the dstPort.
     * For FTP data channel this would be the client's port when the PORT
     * command is used and the server is initiating the connection.
     * This is done first because it is the most common case for FTP clients.
     */
    if (IP_LESSER(dstIP,srcIP))
    {
        IP_COPY_VALUE(hashKey.ip1, dstIP);
        IP_COPY_VALUE(hashKey.ip2, srcIP);
    }
    else
    {
        IP_COPY_VALUE(hashKey.ip1, srcIP);
        IP_COPY_VALUE(hashKey.ip2, dstIP);
    }
    hashKey.port = dstPort;
    hashKey.protocol = protocol;
    hashKey.pad = 0;

    node = sfghash_find(channelHash, &hashKey);

    if (!node)
    {
        /* Okay, next try the hash table using the srcPort.
         * For FTP data channel this would be the servers's port when the
         * PASV command is used and the client is initiating the connection.
         */
        hashKey.port = srcPort;
        node = sfghash_find(channelHash, &hashKey);

        /* We could also check the reverses of these, ie. use 
         * srcIP then dstIP in the hashKey.  Don't need to, though.
         *
         * Here's why:
         * 
         * Since there will be an ACK that comes back from the server
         * side, we don't need to look for the hash entry the other
         * way -- it will be found when we get the ACK.  This approach
         * results in 2 checks per packet -- and 2 checks on the ACK.
         * If we find a match, cool.  If not we've done at most 4 checks
         * between the packet and the ACK.
         * 
         * Whereas, if we check the reverses, we do 4 checks on each
         * side, or 8 checks between the packet and the ACK.  While
         * this would more quickly find the channel to ignore, it is
         * a performance hit when we the session in question is
         * NOT being ignored.  Err on the side of performance here.
         */
    }


    /* Okay, found the key --> verify that the info in the node
     * does in fact match and has not expired.
     */
    time(&now);
    if (node)
    {
        /* If the IPs match and if the ports match (or the port is
         * "unknown"), we should ignore this channel.
         */
        if(
#ifdef SUP_IP6
        IP_EQUALITY(&node->ip1, srcIP) && IP_EQUALITY(&node->ip2, dstIP) &&
#else
        IP_EQUALITY(node->ip1, srcIP) && IP_EQUALITY(node->ip2, dstIP) &&
#endif
            (node->port1 == srcPort || node->port1 == UNKNOWN_PORT) &&
            (node->port2 == dstPort || node->port2 == UNKNOWN_PORT) )
        {
            match = 1;
        }
        else if (
#ifdef SUP_IP6
        IP_EQUALITY(&node->ip2, srcIP) && IP_EQUALITY(&node->ip1, dstIP) &&
#else
        IP_EQUALITY(node->ip2, srcIP) && IP_EQUALITY(node->ip1, dstIP) &&
#endif
                 (node->port2 == srcPort || node->port2 == UNKNOWN_PORT) &&
                 (node->port1 == dstPort || node->port1 == UNKNOWN_PORT) )
        {
            match = 1;
        }

        /* Make sure the packet direction is correct */
        switch (node->direction)
        {
            case SSN_DIR_BOTH:
                break;
            case SSN_DIR_CLIENT:
                if (!(p->packet_flags & PKT_FROM_CLIENT))
                    match = 0;
                break;
            case SSN_DIR_SERVER:
                if (!(p->packet_flags & PKT_FROM_SERVER))
                    match = 0;
                break;
        }

        if (node->expires)
            expired = (now > node->expires);
        if (match)
        {
            /* Uh, just check to be sure it hasn't expired,
             * in case we missed a packet and this is a
             * different connection.  */
            if ((node->numOccurances > 0) && (!expired))
            {
                node->numOccurances--;
                /* Matched & Still valid --> ignore it! */
                retVal = node->direction;

#ifdef DEBUG
                {
                    /* Have to allocate & copy one of these since inet_ntoa
                     * clobbers the info from the previous call. */

#ifdef SUP_IP6
                    sfip_t *tmpAddr;
                    char srcAddr[40];
                    tmpAddr = srcIP;
                    SnortStrncpy(srcAddr, sfip_ntoa(tmpAddr), sizeof(srcAddr));
                    tmpAddr = dstIP;
#else

                    struct in_addr tmpAddr;
                    char srcAddr[17];
                    tmpAddr.s_addr = srcIP;
                    SnortStrncpy(srcAddr, inet_ntoa(tmpAddr), sizeof(srcAddr));
                    tmpAddr.s_addr = dstIP;
#endif

                    DEBUG_WRAP(DebugMessage(DEBUG_STREAM,
                           "Ignoring channel %s:%d --> %s:%d\n",
                           srcAddr, srcPort,
                           inet_ntoa(tmpAddr), dstPort););
                }
#endif
            }
        }

        if (((node->numOccurances <= 0) || (expired)) &&
                (node->expires != 0))

        {
            /* Either expired or was the only one in the hash
             * table.  Remove this node.  */
            sfghash_remove(channelHash, &hashKey);
        }
    }

    /* Clean the hash table of at most 5 expired nodes */
    for (i=0;i<5 && channelHash->count>0;i++)
    {
        SFGHASH_NODE *hash_node = sfghash_findfirst(channelHash);
        if (hash_node)
        {
            node = hash_node->data;
            if (node)
            {
                expired = (node->expires != 0) && (now > node->expires);
                if (expired)
                {
                    /* sayonara baby... */
                    sfghash_remove(channelHash, hash_node->key);
                }
                else
                {
                    /* This one's not expired, fine...
                     * no need to prune further.
                     */
                    break;
                }
            }
        }
    }

    return retVal;
}

void CleanupIgnore()
{
    if (channelHash)
    {
        sfghash_delete(channelHash);
        channelHash = NULL;
    }
}
/* $Id$ */

/*
** Copyright (C) 2005-2008 Sourcefire, Inc.
** AUTHOR: Steven Sturges <ssturges@sourcefire.com>
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

/* snort_stream4_udp.c
 * 
 * Purpose: UDP Support for Stream4.  Used only to establish packet direction
 *          and provide session data.
 *
 * Arguments:
 *   
 * Effect:
 *
 * Comments:
 *
 * Any comments?
 *
 */

#ifdef STREAM4_UDP

#define _STREAM4_INTERNAL_USAGE_ONLY_

#include "decode.h"
#include "debug.h"
#include "util.h"
#include "checksum.h"
#include "detect.h"
#include "plugbase.h"
#include "plugin_enum.h"
#include "rules.h"
#include "snort.h"
#include "sfsnprintfappend.h"

#include "sp_dynamic.h"

#include "perf.h"

extern OptTreeNode *otn_tmp;

#include "stream.h"
#include "snort_stream4_session.h"
#include "stream_api.h"
#include "stream_ignore.h"

#include "portscan.h" /* To know when to create sessions for all UDP */

extern Stream4Data s4data;

#ifdef PERF_PROFILING
extern PreprocStats stream4PerfStats;
PreprocStats stream4UdpPerfStats;
PreprocStats stream4UdpPrunePerfStats;
#endif


/** 
 * See if we can get ignore this as a UDP packet
 *
 * The Emergency Status stuff is taken care of here.
 * 
 * @param p Packet
 * 
 * @return 1 if this packet isn't destined to be processeed, 0 otherwise
 */
static INLINE int NotForStream4Udp(Packet *p)
{
    if(!p)
    {
        return 1;
    }

    if (!s4data.enable_udp_sessions)
    {
        DEBUG_WRAP(DebugMessage(DEBUG_STREAM, "Not tracking UDP Sessions\n"););
        return 1;
    }

    if(p->udph == NULL)
    {
        DEBUG_WRAP(DebugMessage(DEBUG_STREAM, "p->udph is null, returning\n"););
        return 1;
    }
    
    /* don't accept packets w/ bad checksums */
    if(p->csum_flags & CSE_IP || p->csum_flags & CSE_UDP)
    {
        DEBUG_WRAP(
                   u_int8_t c1 = (p->csum_flags & CSE_IP);
                   u_int8_t c2 = (p->csum_flags & CSE_UDP);
                   DebugMessage(DEBUG_STREAM, "IP CHKSUM: %d, CSE_UDP: %d",
                                c1,c2);
                   DebugMessage(DEBUG_STREAM, "Bad checksum returning\n");
                   );
        
        p->packet_flags |= PKT_STREAM_UNEST_UNI;
        return 1;
    }

    DEBUG_WRAP(DebugMessage(DEBUG_STREAM, "Packet is for stream4...\n"););
    return 0;
}

static INLINE int GetDirectionUdp(Session *ssn, Packet *p)
{
#ifndef SUP_IP6
    if(p->iph->ip_src.s_addr == ssn->client.ip)
    {
        return FROM_CLIENT;
    }
#endif
        
    return FROM_SERVER;
}

static INLINE u_int8_t GetUdpAction(Packet *p)
{
    u_int8_t ret = s4data.udp_ports[p->sp] | s4data.udp_ports[p->dp];

    return ret;
}

/**
 * Prune The state machine if we need to
 *
 * Also updates all variables related to pruning that only have to
 * happen at initialization
 *
 * For want of packet time at plugin initialization. (It only happens once.)
 * It wood be nice to get the first packet and do a little extra before
 * getting into the main snort processing loop.
 *   -- cpw
 * 
 * @param p Packet ptr
 */
static INLINE void UDPPruneCheck(Packet *p)
{
    PROFILE_VARS;

    if (!s4data.last_udp_prune_time)
    {
        s4data.last_udp_prune_time = p->pkth->ts.tv_sec;
        return;
    }

    if( (u_int)(p->pkth->ts.tv_sec) > s4data.last_udp_prune_time + s4data.timeout)
    {
        DEBUG_WRAP(DebugMessage(DEBUG_STREAM, "Prune time quanta exceeded, pruning "
                    "udp cache\n"););

        PREPROC_PROFILE_START(stream4UdpPrunePerfStats);
        PruneSessionCache(IPPROTO_UDP, p->pkth->ts.tv_sec, 0, NULL);
        PREPROC_PROFILE_END(stream4UdpPrunePerfStats);
        s4data.last_udp_prune_time = p->pkth->ts.tv_sec;

        DEBUG_WRAP(DebugMessage(DEBUG_STREAM, "Pruned for timeouts, %lu udp sessions "
                    "active\n", 
                    (unsigned long int) GetSessionCount(p)););
    }
}

void Stream4ProcessUdp(Packet *p)
{
#ifndef SUP_IP6
    int direction;
    char ignore;
#endif
    u_int8_t action;

    if (NotForStream4Udp(p))
    {
        return;
    }

    DEBUG_WRAP(
            DebugMessage((DEBUG_STREAM|DEBUG_STREAM_STATE), 
                "Got UDP Packet 0x%X:%d ->  0x%X:%d\n", 
                p->iph->ip_src.s_addr,
                p->sp,
                p->iph->ip_dst.s_addr,
                p->dp);
            );

    action = GetUdpAction(p);

    if (!action)
    {
        if (s4data.udp_ignore_any)
        {
            /* Ignore this UDP packet entirely */
            DisableDetect(p);
            SetPreprocBit(p, PP_SFPORTSCAN);
            SetPreprocBit(p, PP_PERFMONITOR);
            otn_tmp = NULL;
            DEBUG_WRAP(
                DebugMessage((DEBUG_STREAM|DEBUG_STREAM_STATE),
                    "Not inspecting UDP Packet because of ignore any\n"););
        }
        return;
    }

#ifndef SUP_IP6
    if (action & UDP_SESSION)
    {
        Session *ssn = GetSession(p);

        if (!ssn)
        {
            ssn = GetNewSession(p);

            if (ssn)
            {
                AddUDPSession(&sfPerf.sfBase);
                ssn->flush_point = 0;

                ssn->client.ip = p->iph->ip_src.s_addr;
                ssn->server.ip = p->iph->ip_dst.s_addr;
                ssn->client.port = p->sp;
                ssn->server.port = p->dp;

                /* New session, Sender is the first one we see. */

                /* UDP Sessions are AWLAYS considered 'midstream'
                 * since there is no real way to know if this is
                 * the first packet or the 100th.
                 */
                ssn->session_flags = SSNFLAG_SEEN_SENDER | SSNFLAG_MIDSTREAM;
                ssn->start_time = p->pkth->ts.tv_sec;
            }
            else
            {
                DEBUG_WRAP(DebugMessage((DEBUG_STREAM|DEBUG_STREAM_STATE),
                                        "Couldn't get a new udp session\n"););
                return;
            }
        }
        else
        {
            if (ssn->client.ip == p->iph->ip_src.s_addr)
            {
                ssn->client.pkts_sent++;
                ssn->client.bytes_sent += p->dsize;
            }
            else
            {
                ssn->session_flags |= SSNFLAG_SEEN_RESPONDER;
                ssn->server.pkts_sent++;
                ssn->server.bytes_sent += p->dsize;
            }
        }

        p->ssnptr = ssn;

        /* update the time for this session */
        ssn->last_session_time = p->pkth->ts.tv_sec;

        /* Check if stream is to be ignored per session flags */
        if ( ssn->ignore_flag )
        {
            DEBUG_WRAP(DebugMessage(DEBUG_STREAM, 
                    "Nothing to do -- stream is set to be ignored.\n"););

            stream_api->stop_inspection(NULL, p, SSN_DIR_BOTH, -1, 0);

#ifdef DEBUG
            {
                /* Have to allocate & copy one of these since inet_ntoa
                 * clobbers the info from the previous call. */
                struct in_addr tmpAddr;
                char srcAddr[17];
                tmpAddr.s_addr = p->iph->ip_src.s_addr;
                SnortStrncpy(srcAddr, (char *)inet_ntoa(tmpAddr), sizeof(srcAddr));
                tmpAddr.s_addr = p->iph->ip_dst.s_addr;

                DEBUG_WRAP(DebugMessage(DEBUG_STREAM,
                       "Ignoring channel %s:%d --> %s:%d\n",
                       srcAddr, p->sp,
                       inet_ntoa(tmpAddr), p->dp););
            }
#endif
            return;
        }

        /* Check if this packet is one of the "to be ignored" channels.
         * If so, set flag, flush any data that may be buffered up on
         * the connection, and bail. */
        ignore = CheckIgnoreChannel(p);
        if (ignore)
        {
            stream_api->stop_inspection(ssn, p, ignore, -1, 0);

            return;
        }

        /* update the packet flags */
        if((direction = GetDirectionUdp(ssn, p)) == FROM_SERVER)
        {
            p->packet_flags |= PKT_FROM_SERVER;
            DEBUG_WRAP(DebugMessage(DEBUG_STREAM,  "UDP Listener packet\n"););

        }
        else
        {
            p->packet_flags |= PKT_FROM_CLIENT;
            DEBUG_WRAP(DebugMessage(DEBUG_STREAM,  "UDP Sender packet\n"););
        }

        /* Mark session as established if packet from server
         * (and not already established)
         */
        if (!(ssn->session_flags & SSNFLAG_ESTABLISHED) &&
            (p->packet_flags & PKT_FROM_SERVER))
        {
            ssn->session_flags |= SSNFLAG_SEEN_RESPONDER;
            ssn->session_flags |= SSNFLAG_ESTABLISHED;
        }

        /* Update packet flags for session 'UDP state' info */
        if (ssn->session_flags & SSNFLAG_ESTABLISHED)
        {
            p->packet_flags |= PKT_STREAM_EST;
        }
        else
        {
            p->packet_flags |= PKT_STREAM_UNEST_BI;
        }
    }
#endif

    /* see if we need to prune the session cache */
    UDPPruneCheck(p);

    return;
}

void Stream4UdpConfigure()
{
    int16_t sport, dport;
    RuleListNode *rule;
    RuleTreeNode *rtn;
    OptTreeNode *otn;
    extern RuleListNode *RuleLists;
    char buf[STD_BUF+1];
    int i, j=0;
    char inspectSrc, inspectDst;
    char any_any_flow = 0;

#ifdef PERF_PROFILING
    RegisterPreprocessorProfile("s4UDP", &stream4UdpPerfStats, 1, &stream4PerfStats);
    RegisterPreprocessorProfile("s4UDPPrune", &stream4UdpPrunePerfStats, 1, &stream4PerfStats);
#endif

    /* Post-process UDP rules to establish UDP ports to inspect. */
    for (rule=RuleLists; rule; rule=rule->next)
    {
        if(!rule->RuleList)
            continue;

        /*
        **  Get UDP rules
        */
        if(rule->RuleList->UdpList)
        {
            for(rtn = rule->RuleList->UdpList; rtn != NULL; rtn = rtn->right)
            {
                inspectSrc = inspectDst = 0;

                sport = (rtn->hsp == rtn->lsp) ? rtn->hsp : -1;

                if (rtn->flags & ANY_SRC_PORT)
                {
                    sport = -1;
                }

                if (sport > 0 &&  rtn->not_sp_flag > 0 )
                {
                    sport = -1;
                }

                /* Set the source port to inspect */
                if (sport != -1)
                {
                    inspectSrc = 1;
                    s4data.udp_ports[sport] |= UDP_INSPECT;
                }

                dport = (rtn->hdp == rtn->ldp) ? rtn->hdp : -1;

                if (rtn->flags & ANY_DST_PORT)
                {
                    dport = -1;
                }

                if (dport > 0 && rtn->not_dp_flag > 0 )
                {
                    dport = -1;
                }

                /* Set the dest port to inspect */
                if (dport != -1)
                {
                    inspectDst = 1;
                    s4data.udp_ports[dport] |= UDP_INSPECT;
                }

                if (inspectSrc || inspectDst)
                {
                    /* Look for an OTN with flow keyword */
                    for (otn = rtn->down; otn; otn = otn->next)
                    {
                        if (otn->ds_list[PLUGIN_CLIENTSERVER] ||
#ifdef DYNAMIC_PLUGIN
                            DynamicHasFlow(otn) ||
                            DynamicHasFlowbit(otn) ||
#endif
                            otn->ds_list[PLUGIN_FLOWBIT])
                        {
                            if (inspectSrc)
                            {
                                s4data.udp_ports[sport] |= UDP_SESSION;
                            }
                            if (inspectDst)
                            {
                                s4data.udp_ports[dport] |= UDP_SESSION;
                            }
                        }
                    }
                }
                else
                {
                    /* any -> any rule */
                    if (any_any_flow == 0)
                    {
                        for (otn = rtn->down; otn; otn = otn->next)
                        {
                            /* Look for an OTN with flow or flowbits keyword */
                            if (otn->ds_list[PLUGIN_CLIENTSERVER] ||
#ifdef DYNAMIC_PLUGIN
                                DynamicHasFlow(otn) ||
                                DynamicHasFlowbit(otn) ||
#endif
                                otn->ds_list[PLUGIN_FLOWBIT])
                            {
                                for (i=1;i<=65535;i++)
                                {
                                    /* track sessions for ALL ports becuase
                                     * of any -> any with flow/flowbits */
                                    s4data.udp_ports[i] |= UDP_SESSION;
                                }
                                any_any_flow = 1;
                                break;
                            }
                            else if (any_any_flow == 0)
                            {
                                if (!s4data.udp_ignore_any)
                                {
                                    /* Not ignoring any any rules... */
                                    break;
                                }
                            }
                        } /* for (otn=...) */
                    }
                }
            }
        }
    }

    /* If portscan is tracking UDP, need to create
     * sessions for all UDP ports */
    if (ps_get_protocols() & PS_PROTO_UDP)
    {
        int k;
        for (k=0; k<65535; k++)
        {
            s4data.udp_ports[k] |= UDP_SESSION;
        }
    }
    
    memset(buf, 0, STD_BUF+1);
    snprintf(buf, STD_BUF, "    Stream4 UDP Ports: ");       

    for(i=0;i<65535;i++)
    {
        if(s4data.udp_ports[i])
        {
            switch (s4data.udp_ports[i])
            {
            case UDP_INSPECT:
                sfsnprintfappend(buf, STD_BUF, "%d(%s) ", i, "I");
                break;
            case UDP_SESSION:
                /* Shouldn't have only a "session" */
                s4data.udp_ports[i] |= UDP_INSPECT;
                /* Fall through */
            case UDP_INSPECT|UDP_SESSION:
                sfsnprintfappend(buf, STD_BUF, "%d(%s) ", i, "SI");
                break;
            }
            j++;
        }

        if(j > 20)
        { 
            LogMessage("%s...\n", buf);
            return;
        }
    }
    LogMessage("%s\n", buf);
}
#endif /* STREAM4_UDP */

/* $Id$ */

/*
** Copyright (C) 2005-2008 Sourcefire, Inc.
** AUTHOR: Steven Sturges <ssturges@sourcefire.com>
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

/* snort_stream4_session.c
 * 
 * Purpose: Hash Table implementation of session management functions for
 *          TCP stream preprocessor.
 *
 * Arguments:
 *   
 * Effect:
 *
 * Comments:
 *
 * Any comments?
 *
 */

#define _STREAM4_INTERNAL_USAGE_ONLY_

#include "sfhashfcn.h"
#include "decode.h"
#include "debug.h"
#include "stream.h"
#include "util.h"

/* Stuff defined in stream4.c that we use */
extern void DeleteSession(Session *, u_int32_t, char flags);
extern Stream4Data s4data;
extern u_int32_t stream4_memory_usage;

static SFXHASH *sessionHashTable = NULL;
static SFXHASH *udpSessionHashTable = NULL;

#include "cap.h"
#include "profiler.h"
#ifdef PERF_PROFILING
extern PreprocStats stream4LUSessPerfStats;
#endif

#ifndef WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

int GetSessionCount(Packet *p)
{
    if (IPH_IS_VALID(p))
    {
        if (GET_IPH_PROTO(p) == IPPROTO_TCP)
        {
            if (sessionHashTable)
                return sessionHashTable->count;
            else
                return 0;
        }
        else
        {
            if (udpSessionHashTable)
                return udpSessionHashTable->count;
            else
                return 0;
        }
    }
    return 0;
}

int GetSessionKey(Packet *p, SessionHashKey *key)
{
    snort_ip_p srcIp, dstIp;
    u_int16_t srcPort, dstPort;

    if (!key)
        return 0;

    memset(key, 0, sizeof(SessionHashKey));

    srcIp = GET_SRC_IP(p);
    dstIp = GET_DST_IP(p);
    if (p->tcph)
    {
        srcPort = p->tcph->th_sport;
        dstPort = p->tcph->th_dport;
    }
#ifdef STREAM4_UDP
    else if (p->udph)
    {
        srcPort = p->udph->uh_sport;
        dstPort = p->udph->uh_dport;
    }
#endif
    else
    {
        srcPort = 0;
        dstPort = 0;
    }
    
    if (IP_LESSER(srcIp, dstIp))
    {
        IP_COPY_VALUE(key->lowIP, srcIp);
        key->port = srcPort;
        IP_COPY_VALUE(key->highIP, dstIp);
        key->port2 = dstPort;
    }
    else if (IP_EQUALITY(srcIp, dstIp))
    {
        IP_COPY_VALUE(key->lowIP, srcIp);
        IP_COPY_VALUE(key->highIP, dstIp);
        if (srcPort < dstPort)
        {
            key->port = srcPort;
            key->port2 = dstPort;
        }
        else
        {
            key->port2 = srcPort;
            key->port = dstPort;
        }
    }
    else
    {
        IP_COPY_VALUE(key->lowIP, dstIp);
        key->port = dstPort;
        IP_COPY_VALUE(key->highIP, srcIp);
        key->port2 = srcPort;
    }

    key->proto = GET_IPH_PROTO(p);

#ifdef _LP64
    key->pad1 = key->pad2 = key->pad3 = 0;
#endif

    return 1;
}

Session *GetSessionFromHashTable(Packet *p)
{
    Session *returned = NULL;
    SFXHASH_NODE *hnode;
    SessionHashKey sessionKey;
    SFXHASH *table;

    if (!GetSessionKey(p, &sessionKey))
        return NULL;

    if (sessionKey.proto == IPPROTO_TCP)
    {
        table = sessionHashTable;
    }
    else /* Implied IPPROTO_UDP */
    {
        table = udpSessionHashTable;
    }

    hnode = sfxhash_find_node(table, &sessionKey);

    if (hnode && hnode->data)
    {
        /* This is a unique hnode, since the sfxhash finds the
         * same key before returning this node.
         */
        returned = (Session *)hnode->data;
    }
    return returned;
}

int RemoveSessionFromHashTable(Session *ssn)
{
    SFXHASH *table;
    if (ssn->hashKey.proto == IPPROTO_TCP)
    {
        table = sessionHashTable;
    }
    else /* Implied IPPROTO_UDP */
    {
        table = udpSessionHashTable;
    }

    return sfxhash_remove(table, &(ssn->hashKey));
}

int CleanHashTable(SFXHASH *table, u_int32_t thetime, Session *save_me, int memCheck)
{
    Session *idx;
    u_int32_t pruned = 0;
    u_int32_t timeout = s4data.timeout;

    if (thetime != 0)
    {
        char got_one;
        idx = (Session *) sfxhash_lru(table);

        if(idx == NULL)
        {
            return 0;
        }

        do
        {
            got_one = 0;            
            if(idx == save_me)
            {
                SFXHASH_NODE *lastNode = sfxhash_lru_node(table);
                sfxhash_gmovetofront(table, lastNode);
                lastNode = sfxhash_lru_node(table);
                if ((lastNode) && (lastNode->data != idx))
                {
                    idx = (Session *)lastNode->data;
                    continue;
                }
                else
                {
                    return pruned;
                }
            }

            timeout = s4data.timeout;
            if(idx->drop_traffic)
            {
                /* If we're dropping traffic on the session, keep
                 * it around longer.  */
                timeout = s4data.timeout * 2;
            }

            if((idx->last_session_time+timeout) < thetime)
            {
                Session *savidx = idx;

                if(sfxhash_count(table) > 1)
                {
                    DEBUG_WRAP(DebugMessage(DEBUG_STREAM, "pruning stale session\n"););
                    DeleteSession(savidx, thetime, SESSION_CLOSED_TIMEDOUT);
                    idx = (Session *) sfxhash_lru(table);
                    pruned++;
                    got_one = 1;
                }
                else
                {
                    DeleteSession(savidx, thetime, SESSION_CLOSED_TIMEDOUT);
                    pruned++;
                    return pruned;
                }
            }
            else
            {
                return pruned;
            }

            if (pruned > s4data.cache_clean_sessions)
            {
                /* Don't bother cleaning more than XXX at a time */
                break;
            }
        } while ((idx != NULL) && (got_one == 1));

        return pruned;
    }
    else
    {
        /* Free up xxx sessions at a time until we get under the
         * memcap or free enough sessions to be able to create
         * new ones.
         *
         * This loop repeats while we're over the memcap or we have
         * more sessions than the max less what we're supposed to
         * cleanup at a time.
         */
        while ((sfxhash_count(table) > 1) &&
                ((memCheck && (stream4_memory_usage > s4data.memcap)) ||
                 (table->count > (s4data.max_sessions - s4data.cache_clean_sessions)) ||
                 (pruned < s4data.cache_clean_sessions)))
        {
            u_int32_t i;
            idx = (Session *) sfxhash_lru(table);

            for (i=0;i<s4data.cache_clean_sessions && 
                     (sfxhash_count(table) > 1); i++)
            {
                if(idx != save_me)
                {
                    DeleteSession(idx, thetime, SESSION_CLOSED_PRUNED);
                    pruned++;
                    idx = (Session *) sfxhash_lru(table);
                }
                else
                {
                    SFXHASH_NODE *lastNode = sfxhash_lru_node(table);
                    sfxhash_gmovetofront(table, lastNode);
                    lastNode = sfxhash_lru_node(table);
                    if ((lastNode) && (lastNode->data == idx))
                    {
                        /* Okay, this session is the only one left */
                        break;
                    }
                    idx = (Session *) sfxhash_lru(table);
                    i--; /* Didn't clean this one */
                }
            }
        }
    }
    return pruned;
}

Session *GetNewSession(Packet *p)
{
    Session *retSsn = NULL;
    SessionHashKey sessionKey;
    SFXHASH_NODE *hnode = NULL;
    SFXHASH *table;

    if (!GetSessionKey(p, &sessionKey))
        return retSsn;

    if (sessionKey.proto == IPPROTO_TCP)
    {
        table = sessionHashTable;
    }
    else /* Implied IPPROTO_UDP */
    {
        table = udpSessionHashTable;
    }

    if (stream4_memory_usage < s4data.memcap)
    {
        hnode = sfxhash_get_node(table, &sessionKey);
    }

    if (!hnode)
    {
        DEBUG_WRAP(DebugMessage(DEBUG_STREAM, "HashTable full, clean it\n"););
        s4data.last_prune_time = p->pkth->ts.tv_sec;
        if (!CleanHashTable(table, p->pkth->ts.tv_sec, NULL, 0))
        {
            DEBUG_WRAP(DebugMessage(DEBUG_STREAM, "HashTable full, no timeouts, clean it\n"););

            CleanHashTable(table, 0, NULL, 0);
        }

        /* Should have some freed nodes now */
        hnode = sfxhash_get_node(table, &sessionKey);
        if (!hnode)
        {
            DEBUG_WRAP(DebugMessage(DEBUG_STREAM, "Problem, no freed nodes\n"););
        }
    }
    if (hnode && hnode->data)
    {
        retSsn = hnode->data;

        /* Zero everything out */
        memset(retSsn, 0, sizeof(Session));

        /* Save the session key for future use */
        memcpy(&(retSsn->hashKey), &sessionKey,
                        sizeof(SessionHashKey));
    }

    return retSsn;
}

Session *RemoveSession(Session *ssn)
{
    if (!RemoveSessionFromHashTable(ssn) )
        return ssn;
    else
        return NULL;
}

Session *GetSession(Packet *p)
{
    Session *ssn;
    PROFILE_VARS;
    PREPROC_PROFILE_START(stream4LUSessPerfStats);
    ssn = GetSessionFromHashTable(p);
    PREPROC_PROFILE_END(stream4LUSessPerfStats);

    /* Handle a timeout of existing session */
    if(ssn)
    {
        int timeout = s4data.timeout;
        if(ssn->drop_traffic)
        {
            /* If we're dropping traffic on the session, keep
             * it around longer.  */
            timeout *= 2;
        }
        if ((ssn->last_session_time+timeout) < p->pkth->ts.tv_sec)
        {
            DeleteSession(ssn, p->pkth->ts.tv_sec, SESSION_CLOSED_TIMEDOUT);
            ssn = NULL;
        }
    }

    return ssn;
}

void InitSessionCache()
{
    if (!sessionHashTable)
    {
        /* Create the hash table --
         * SESSION_HASH_TABLE_SIZE hash buckets
         * keysize = 12 bytes (2x 32bit IP, 2x16bit port)
         * data size = sizeof(Session) object
         * no max mem
         * no automatic node recovery
         * NULL node recovery free function
         * NULL user data free function
         * recycle nodes
         */
        /* Rule of thumb, size should be 1.4 times max to avoid
         * collisions.
         */
        int hashTableSize = sfxhash_calcrows((int)(s4data.max_sessions * 1.4));
        //int maxSessionMem = s4data.max_sessions * (
        //                     sizeof(Session) +
        //                     sizeof(SFXHASH_NODE) +
        //                     sizeof(SessionHashKey) +
        //                     sizeof (SFXHASH_NODE *));
        //int tableMem = (hashTableSize +1) * sizeof(SFXHASH_NODE*);
        //int maxMem = maxSessionMem + tableMem;
        sessionHashTable = sfxhash_new(hashTableSize,
                        sizeof(SessionHashKey),
                        //sizeof(Session), maxMem, 0, NULL, NULL, 1);
                        sizeof(Session), 0, 0, NULL, NULL, 1);

        sfxhash_set_max_nodes(sessionHashTable, s4data.max_sessions);
#ifdef STREAM4_UDP
        /* And create the udp one */
        hashTableSize = sfxhash_calcrows((int)(s4data.max_udp_sessions * 1.4));
        //maxSessionMem = s4data.max_udp_sessions * (
        //                     sizeof(Session) +
        //                     sizeof(SFXHASH_NODE) +
        //                     sizeof(SessionHashKey) +
        //                     sizeof (SFXHASH_NODE *));
        //tableMem = (hashTableSize +1) * sizeof(SFXHASH_NODE*);
        //maxMem = maxSessionMem + tableMem;
        udpSessionHashTable = sfxhash_new(hashTableSize,
                        sizeof(SessionHashKey),
                        //sizeof(Session), maxMem, 0, NULL, NULL, 1);
                        sizeof(Session), 0, 0, NULL, NULL, 1);

        sfxhash_set_max_nodes(udpSessionHashTable, s4data.max_udp_sessions);
#endif
    }
}

void DeleteSessionCache()
{
    if (sessionHashTable)
    {
        sfxhash_delete(sessionHashTable);
        sessionHashTable = NULL;
    }
#ifdef STREAM4_UDP
    if (udpSessionHashTable)
    {
        sfxhash_delete(udpSessionHashTable);
        udpSessionHashTable = NULL;
    }
#endif
}

void PurgeSessionCache()
{
    Session *ssn = NULL;
    ssn = (Session *)sfxhash_mru(sessionHashTable);
    while (ssn)
    {
        DeleteSession(ssn, 0, SESSION_CLOSED_PRUNED);
        ssn = (Session *)sfxhash_mru(sessionHashTable);
    }
}

void PrintSessionCache()
{
    DEBUG_WRAP(DebugMessage(DEBUG_STREAM, "%lu streams active, %u bytes in use\n", 
                            sfxhash_count(sessionHashTable), stream4_memory_usage););
    return;
}

int PruneSessionCache(u_int8_t proto, u_int32_t thetime, int mustdie, Session *save_me)
{
    SFXHASH *table;
    if (proto == IPPROTO_TCP)
    {
        table = sessionHashTable;
    }
    else /* Implied IPPROTO_UDP */
    {
        table = udpSessionHashTable;
    }

    return CleanHashTable(table, thetime, save_me, 1);
}

