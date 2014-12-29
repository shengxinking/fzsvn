/* $Id$ */
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

/*
 *
 * Program: Snort
 *
 * Purpose: Check out the README file for info on what you can do
 *          with Snort.
 *
 * Author: Martin Roesch (roesch@clark.net)
 *
 * Comments: Ideas and code stolen liberally from Mike Borella's IP Grab
 *           program. Check out his stuff at http://www.borella.net.  I
 *           also have ripped some util functions from TCPdump, plus Mike's
 *           prog is derived from it as well.  All hail TCPdump....
 *
 */

/*  I N C L U D E S  **********************************************************/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "decode.h"
#ifdef TIMESTATS
#include <signal.h> /* added for new hourly stats function in util.c */
#include <time.h>   /* added for new time stats function in util.c */
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <sys/stat.h>
#ifndef WIN32
#include <grp.h>
#include <pwd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif  /* !WIN32 */
#ifdef HAVE_GETOPT_LONG
//#define _GNU_SOURCE
/* A GPL copy of getopt & getopt_long src code is now in sfutil */
#undef HAVE_GETOPT_LONG
#endif
#include <getopt.h>
//#include <timersub.h>
#include <setjmp.h>

#include "cap.h"
#include "rules.h"
#include "plugbase.h"
#include <signal.h>
#include "debug.h"
#include "util.h"
//#include "tag.h"
#include "mempool.h"
/* Undefine the one from sf_dynamic_preprocessor.h */
#include "profiler.h"
#ifdef PERF_PROFILING
extern PreprocStats detectPerfStats, decodePerfStats,
       totalPerfStats, eventqPerfStats, rulePerfStats, mpsePerfStats;
extern PreprocStats ruleCheckBitPerfStats, ruleSetBitPerfStats, ruleFailedFlowbitsPerfStats;
extern PreprocStats ruleRTNEvalPerfStats, ruleOTNEvalPerfStats, ruleHeaderNoMatchPerfStats;
extern PreprocStats ruleAddEventQPerfStats, ruleNQEventQPerfStats;
#endif

extern char *optarg;                /* for getopt */
extern int   optind,opterr,optopt;  /* for getopt */

extern char *file_name;        /* parser.c - current rules file being processed */
extern int file_line;          /* parser.c - current line being processed in the rules */

/* set/cleared in otnx_match */
OptTreeNode * current_otn=0;

/*
 * used to identifiy code in use when segv signal happened
 * SIGLOC_xxxx
 */
enum { SIGLOC_PARSE_RULES_FILE=1, SIGLOC_PCAP_LOOP };
int signal_location=0;

//static struct bpf_program fcode;        /* Finite state machine holder */

#ifndef DLT_LANE8023
/*
 * Old OPEN BSD Log format is 17.
 * Define DLT_OLDPFLOG unless DLT_LANE8023 (Suse 6.3) is already
 * defined in bpf.h.
 */
#define DLT_OLDPFLOG 17
#endif

/*  G L O B A L S  ************************************************************/

#ifdef TIMESTATS
time_t start_time;    /* tracks how many seconds snort actually ran */
#endif

extern int errno;

/* exported variables *********************************************************/
u_int8_t runMode = 0;   /* snort run mode */
PV pv;                  /* program vars */
int datalink;           /* the datalink value */
char *progname;         /* name of the program (from argv[0]) */
char **progargs;
char *username;
char *groupname;
unsigned long userid = 0;
unsigned long groupid = 0;
struct passwd *pw;
struct group *gr;
char *pcap_cmd;         /* the BPF command string */
char *pktidx;           /* index ptr for the current packet */
pcap_t *pd = NULL;      /* pcap handle */

int g_drop_pkt;        /* inline drop pkt flag */ 
int g_pcap_test;       /* pcap test mode */

/* deprecated? */
FILE *alert;            /* alert file ptr */
FILE *binlog_ptr;       /* binary log file ptr */
int flow;               /* flow var (probably obsolete) */
int thiszone;           /* time zone info */
PacketCount pc;         /* packet count information */
u_long netmasks[33];    /* precalculated netmask array */
struct pcap_pkthdr *g_pkthdr;   /* packet header ptr */
u_char *g_pkt;          /* ptr to the packet data */
u_long g_caplen;        /* length of the current packet */
char *protocol_names[256];
//u_int snaplen = SNAPLEN;


grinder_t grinder;
runtime_config snort_runtime;   /* run-time configuration struct */

//PORTLISTS
        

/*
 * you may need to adjust this on the systems which don't have standard
 * paths defined
 */
#ifndef _PATH_VARRUN
char _PATH_VARRUN[STD_BUF];
#endif

SFPERF sfPerf;

/* locally defined functions **************************************************/
char *ConfigFileSearch(void);
int ProcessAlertCommandLine(void);
int ProcessLogCommandLine(void);
void Restart(void);
void PcapReset(void);
#ifdef DYNAMIC_PLUGIN
void LoadDynamicPlugins(void);
#endif
void PrintVersion(void);
#ifdef INLINE_FAILOPEN
void *InlinePatternMatcherInitThread(void *arg);
void PcapIgnorePacket(char *user, struct pcap_pkthdr * pkthdr, u_char * pkt);
#endif

/* Signal handler declarations ************************************************/
void SigTermHandler(int signal);
void SigIntHandler(int signal);
void SigQuitHandler(int signal);
void SigHupHandler(int signal);
void SigUsrHandler(int signal);
#ifdef TIMESTATS
void SigAlrmHandler(int signal);
#endif
#ifdef CATCH_SEGV
static void SigSegvHandler(int signal);
#else
#ifndef WIN32
#include <sys/resource.h>
#endif
#endif

/*
 *  Check for SIGHUP and invoke restart to
 *  cleanup
 */
void PcapProcessPacket(char *user, struct pcap_pkthdr * pkthdr, u_char * pkt)
{
    PROFILE_VARS;

    PREPROC_PROFILE_START(totalPerfStats);

#ifdef EXIT_CHECK
    if ( pv.exit_check && pc.total_from_pcap >= pv.exit_check ){
        //ExitCheckStart(); //wxh
}
#endif

    /* First thing we do is process a Usr signal that we caught */
    /*if( sig_check() ) wxh
    {
        PREPROC_PROFILE_END(totalPerfStats);
        return;
    }*/

#ifdef TARGET_BASED
    /* Load in a new attribute table if we need to... */
    AttributeTableReloadCheck();
#endif

    pc.total_from_pcap++;

    /*
    ** Save off the time of each and every packet 
    */ 
    //packet_time_update(pkthdr->ts.tv_sec);


    /* reset the thresholding subsystem checks for this packet */
    //sfthreshold_reset();

    PREPROC_PROFILE_START(eventqPerfStats);
    //SnortEventqReset();
    PREPROC_PROFILE_END(eventqPerfStats);

#if defined(WIN32) && defined(ENABLE_WIN32_SERVICE)
    if( pv.terminate_service_flag || pv.pause_service_flag )
    {
        //ClearDumpBuf();  /* cleanup and return without processing */
        return;
    }
#endif  /* WIN32 && ENABLE_WIN32_SERVICE */

    BsdPseudoPacket = NULL;

    ProcessPacket(user, pkthdr, pkt, NULL);
    
    /* Collect some "on the wire" stats about packet size, etc */
    UpdateWireStats(&(sfPerf.sfBase), pkthdr->caplen);

    PREPROC_PROFILE_END(totalPerfStats);
    return;
}

void ProcessPacket(char *user, const struct pcap_pkthdr * pkthdr, const u_char * pkt, void *ft)
{
    Packet p;
    //MemBucket *preproc_bitop_bucket = NULL;
    //MemBucket *preproc_reas_pkt_bitop_bucket = NULL;
   // BITOP preprocBitOp;
    //BITOP preprocReasPktBitOp;
#if defined(MIMICK_IPV6) && defined(SUP_IP6)
    struct pcap_pkthdr pkthdrx;
    static u_char pktx[65536+256];
    EHDR   *ehdr=0;
    VHDR   *vhdr;
    int etype;

    if( !conv_ip4_to_ip6(pkthdr,pkt,&pkthdrx,pktx, 0 /* encap46 flag 0=6, 1=4(6), 2=6(4)*/) )
    {
        /* reset to point to new pkt */
        if( mimick_ip6 )
        {
            pkthdr = &pkthdrx;
            pkt = pktx;
        
        }      
        pc.ipv6_up++;
     }
     else 
     { 
        pc.ipv6_upfail++; 
#ifdef SUP_IP6
        //return;
#endif
     }
    
#endif

    /*if (!s_bitOpInit)
    {
        unsigned int bitop_numbytes;

        bitop_numbits = num_preprocs + 1;
        bitop_numbytes = bitop_numbits >> 3;

        if(bitop_numbits & 7) 
            bitop_numbytes++;

        if (mempool_init(&bitop_pool, num_bitops, bitop_numbytes) == 1)
            FatalError("Out of memory initializing BitOp memory pool\n");

        s_bitOpInit = 1;
    }*/

    /* reset the packet flags for each packet */
    p.packet_flags = 0;
#ifndef GIDS
    g_drop_pkt = 0;
#endif

    /* call the packet decoder */
    (*grinder) (&p, pkthdr, pkt);

    if(!p.pkth || !p.pkt)
    {
        return;
    }
        Preprocess(&p);

#if 0
    /* Set preprocessor bits
     * These bits are used to determine which preprocessors
     * to run */
    preproc_bitop_bucket = mempool_alloc(&bitop_pool);
    if (preproc_bitop_bucket == NULL)
    {
        memset(&preprocBitOp, 0, sizeof(preprocBitOp));
        boInitBITOP(&preprocBitOp, bitop_numbits);
    }
    else 
    {
        boInitStaticBITOP(&preprocBitOp, bitop_pool.obj_size,
                          (unsigned char *)preproc_bitop_bucket->data);
    }

    p.preprocessor_bits = &preprocBitOp;

    /* Set preprocessor rebuilt packet bits 
     * These bits are used to determine whether or not a preprocessor
     * has a reassembled packet that needs to go through the
     * detection engine */
    preproc_reas_pkt_bitop_bucket = mempool_alloc(&bitop_pool);
    if (preproc_reas_pkt_bitop_bucket == NULL)
    {
        memset(&preprocReasPktBitOp, 0, sizeof(preprocBitOp));
        boInitBITOP(&preprocReasPktBitOp, bitop_numbits);
    }
    else 
    {
        boInitStaticBITOP(&preprocReasPktBitOp, bitop_pool.obj_size,
                          (unsigned char *)preproc_reas_pkt_bitop_bucket->data);
    }

    p.preproc_reassembly_pkt_bits = &preprocReasPktBitOp;

    /* Make sure this packet skips the rest of the preprocessors */
    /* Remove once the IPv6 frag code is moved into frag 3 */
    if(p.packet_flags & PKT_NO_DETECT)
    {
        DisableAllDetect(&p);
    }

#ifdef GRE
    if (ft && !p.encapsulated)
#else
    if (ft)
#endif  /* GRE */
    {
        p.packet_flags |= PKT_REBUILT_FRAG;
        p.fragtracker = ft;
    }

    /* print the packet to the screen */
    if(pv.verbose_flag)
    {
        if(p.iph != NULL)
#ifdef SUP_IP6
            PrintIPPkt(stdout, p.iph_api.iph_ret_proto(&p), &p);
#else
            PrintIPPkt(stdout, p.iph->ip_proto, &p);
#endif
        else if(p.ah != NULL)
            PrintArpHeader(stdout, &p);
        else if(p.eplh != NULL)
        {
            PrintEapolPkt(stdout, &p);
        }
        else if(p.wifih && pv.showwifimgmt_flag)
        {
            PrintWifiPkt(stdout, &p);
        }
    }

    switch(runMode)
    {
        case MODE_PACKET_LOG:
            CallLogPlugins(&p, NULL, NULL, NULL);
            break;
        case MODE_IDS:
            /* allow the user to throw away TTLs that won't apply to the
               detection engine as a whole. */
            if(pv.min_ttl && p.iph != NULL && (p.iph->ip_ttl < pv.min_ttl))
            {
                DEBUG_WRAP(DebugMessage(DEBUG_DECODE,
                            "MinTTL reached in main detection loop\n"););

                free_packetBitOp(&preprocBitOp, &bitop_pool, preproc_bitop_bucket);
                free_packetBitOp(&preprocReasPktBitOp, &bitop_pool, preproc_reas_pkt_bitop_bucket);
                return;
            } 
            
            /* just throw away the packet if we are configured to ignore this port */
            if ( p.packet_flags & PKT_IGNORE_PORT )
            {
                free_packetBitOp(&preprocBitOp, &bitop_pool, preproc_bitop_bucket);
                free_packetBitOp(&preprocReasPktBitOp, &bitop_pool, preproc_reas_pkt_bitop_bucket);
                return;
            }

            /* start calling the detection processes */
            Preprocess(&p);
            break;
        default:
            break;
    }

    free_packetBitOp(&preprocBitOp, &bitop_pool, preproc_bitop_bucket);
    free_packetBitOp(&preprocReasPktBitOp, &bitop_pool, preproc_reas_pkt_bitop_bucket);

#endif
    //ClearDumpBuf();
}

int SetPktProcessor(void)
{
	  grinder = DecodeEthPkt;
	return 1;
#ifdef GIDS
    if (InlineMode())
    {

#ifndef IPFW
        if(!pv.quiet_flag)
            LogMessage("Setting the Packet Processor to decode packets "
                    "from iptables\n");

        grinder = DecodeIptablesPkt;
#else
        if(!pv.quiet_flag)
            LogMessage("Setting the Packet Processor to decode packets "
                    "from ipfw divert\n");

        grinder = DecodeIpfwPkt;
#endif /* IPFW */

        return 0;

    }
#endif /* GIDS */

    switch(datalink)
    {
        case DLT_EN10MB:        /* Ethernet */
            if(!pv.readmode_flag)
            {
                if(!pv.quiet_flag)
                    LogMessage("Decoding Ethernet on interface %s\n", 
                            PRINT_INTERFACE(pv.interface));
            }

            grinder = DecodeEthPkt;
            break;

#ifdef DLT_IEEE802_11
        case DLT_IEEE802_11:
            if (!pv.readmode_flag)
            {
                if (!pv.quiet_flag)
                    LogMessage("Decoding IEEE 802.11 on interface %s\n",
                            PRINT_INTERFACE(pv.interface));
            }

            grinder = DecodeIEEE80211Pkt;
            break;
#endif
#ifdef DLT_ENC
        case DLT_ENC:           /* Encapsulated data */
            if (!pv.readmode_flag)
            {
                if (!pv.quiet_flag)
                    LogMessage("Decoding Encapsulated data on interface %s\n",
                           PRINT_INTERFACE(pv.interface));
            }

            grinder = DecodeEncPkt;
            break;

#else
        case 13:
#endif /* DLT_ENC */
        case DLT_IEEE802:                /* Token Ring */
            if(!pv.readmode_flag)
            {
                if(!pv.quiet_flag)
                    LogMessage("Decoding Token Ring on interface %s\n", 
                            PRINT_INTERFACE(pv.interface));
            }

            grinder = DecodeTRPkt;

            break;

        case DLT_FDDI:                /* FDDI */
            if(!pv.readmode_flag)
            {
                if(!pv.quiet_flag)
                    LogMessage("Decoding FDDI on interface %s\n", 
                            PRINT_INTERFACE(pv.interface));
            }

            grinder = DecodeFDDIPkt;

            break;

#ifdef DLT_CHDLC
        case DLT_CHDLC:              /* Cisco HDLC */
            if (!pv.readmode_flag && !pv.quiet_flag)
                LogMessage("Decoding Cisco HDLC on interface %s\n", 
                        PRINT_INTERFACE(pv.interface));

            grinder = DecodeChdlcPkt;

            break;
#endif

        case DLT_SLIP:                /* Serial Line Internet Protocol */
            if(!pv.readmode_flag)
            {
                if(!pv.quiet_flag)
                    LogMessage("Decoding Slip on interface %s\n", 
                            PRINT_INTERFACE(pv.interface));
            }

            if(pv.show2hdr_flag == 1)
            {
                LogMessage("Second layer header parsing for this datalink "
                        "isn't implemented yet\n");

                pv.show2hdr_flag = 0;
            }

            grinder = DecodeSlipPkt;

            break;

        case DLT_PPP:                /* point-to-point protocol */
            if(!pv.readmode_flag)
            {
                if(!pv.quiet_flag)
                    LogMessage("Decoding PPP on interface %s\n", 
                            PRINT_INTERFACE(pv.interface));
            }

            if(pv.show2hdr_flag == 1)
            {
                /* do we need ppp header showup? it's only 4 bytes anyway ;-) */
                LogMessage("Second layer header parsing for this datalink "
                        "isn't implemented yet\n");
                pv.show2hdr_flag = 0;
            }

            grinder = DecodePppPkt;

            break;

#ifdef DLT_PPP_SERIAL
        case DLT_PPP_SERIAL:         /* PPP with full HDLC header*/
            if(!pv.readmode_flag)
            {
                if(!pv.quiet_flag)
                    LogMessage("Decoding PPP on interface %s\n", 
                            PRINT_INTERFACE(pv.interface));
            }

            if(pv.show2hdr_flag == 1)
            {
                /* do we need ppp header showup? it's only 4 bytes anyway ;-) */
                LogMessage("Second layer header parsing for this datalink "
                        "isn't implemented yet\n");
                pv.show2hdr_flag = 0;
            }

            grinder = DecodePppSerialPkt;

            break;
#endif

#ifdef DLT_LINUX_SLL
        case DLT_LINUX_SLL:
            if(!pv.readmode_flag)
            {
                if(!pv.quiet_flag)
                    LogMessage("Decoding 'ANY' on interface %s\n", 
                            PRINT_INTERFACE(pv.interface));
            }

            grinder = DecodeLinuxSLLPkt;

            break;
#endif

#ifdef DLT_PFLOG
        case DLT_PFLOG:
            if(!pv.readmode_flag)
            {
                if(!pv.quiet_flag)
                    LogMessage("Decoding OpenBSD PF log on interface %s\n",
                            PRINT_INTERFACE(pv.interface));
            }

            grinder = DecodePflog;

            break;
#endif

#ifdef DLT_OLDPFLOG
        case DLT_OLDPFLOG:
            if(!pv.readmode_flag)
            {
                if(!pv.quiet_flag)
                    LogMessage("Decoding old OpenBSD PF log on interface %s\n",
                            PRINT_INTERFACE(pv.interface));
            }

            grinder = DecodeOldPflog;

            break;
#endif

#ifdef DLT_LOOP
        case DLT_LOOP:
#endif
        case DLT_NULL:            /* loopback and stuff.. you wouldn't perform
                                   * intrusion detection on it, but it's ok for
                                   * testing. */
            if(!pv.readmode_flag)
            {
                if(!pv.quiet_flag)
                {
                    LogMessage("Decoding LoopBack on interface %s\n", 
                            PRINT_INTERFACE(pv.interface));
                }
            }

            if(pv.show2hdr_flag == 1)
            {
                LogMessage("Data link layer header parsing for this network "
                        " type isn't implemented yet\n");
                pv.show2hdr_flag = 0;
            }
            grinder = DecodeNullPkt;

            break;

#ifdef DLT_RAW /* Not supported in some arch or older pcap
                * versions */
        case DLT_RAW:
            if(!pv.readmode_flag)
            {
                if(!pv.quiet_flag)
                    LogMessage("Decoding raw data on interface %s\n", 
                            PRINT_INTERFACE(pv.interface));
            }

            if(pv.show2hdr_flag == 1)
            {
                LogMessage("There's no second layer header available for "
                        "this datalink\n");
                pv.show2hdr_flag = 0;
            }
            grinder = DecodeRawPkt;

            break;
#endif
            /*
             * you need the I4L modified version of libpcap to get this stuff
             * working
             */
#ifdef DLT_I4L_RAWIP
        case DLT_I4L_RAWIP:
            if (! pv.readmode_flag && !pv.quiet_flag)
                LogMessage("Decoding I4L-rawip on interface %s\n", 
                        PRINT_INTERFACE(pv.interface));

            grinder = DecodeI4LRawIPPkt;

            break;
#endif

#ifdef DLT_I4L_IP
        case DLT_I4L_IP:
            if (! pv.readmode_flag && !pv.quiet_flag)
                LogMessage("Decoding I4L-ip on interface %s\n", 
                        PRINT_INTERFACE(pv.interface));

            grinder = DecodeEthPkt;

            break;
#endif

#ifdef DLT_I4L_CISCOHDLC
        case DLT_I4L_CISCOHDLC:
            if (! pv.readmode_flag && !pv.quiet_flag)
                LogMessage("Decoding I4L-cisco-h on interface %s\n", 
                        PRINT_INTERFACE(pv.interface));

            grinder = DecodeI4LCiscoIPPkt;

            break;
#endif

        default:                        /* oops, don't know how to handle this one */
            ErrorMessage("\n%s cannot handle data link type %d\n",
                    progname, datalink);
            CleanExit(1);
    }

    return 0;
}

void *InterfaceThread(void *arg)
{
    int pcap_ret;
    struct timezone tz;
	pv.pkt_cnt=-1;
    int pkts_to_read = 	pv.pkt_cnt;

    bzero((char *) &tz, sizeof(tz));
    //gettimeofday(&starttime, &tz); wxh

    signal_location =  SIGLOC_PCAP_LOOP;

    /* Read all packets on the device.  Continue until cnt packets read */
#ifdef USE_PCAP_LOOP
    pcap_ret = pcap_loop(pd, pv.pkt_cnt, (pcap_handler) PcapProcessPacket, NULL);
#else

    while(1)
    {
        if (pv.pcap_show)
        {
            fprintf(stdout, "Reading network traffic from \"%s\" with snaplen = %d\n",
                    pv.readfile, pcap_snapshot(pd));
        }

        pcap_ret = pcap_dispatch(pd, pkts_to_read, (pcap_handler)PcapProcessPacket, NULL);
        /*if (pv.usr_signal == SIGHUP)
        {
            pv.done_processing = 1;
            return NULL;
        }*/

        if (pcap_ret < 0)
        {
            break;
        }

        /* If reading from a file... 0 packets at EOF */
        if (pv.readmode_flag && (pcap_ret == 0))
        {
            char reopen_pcap = 0;

            if (sfqueue_count(pv.pcap_queue) > 0)
            {
                reopen_pcap = 1;
            }
            else if (pv.pcap_loop_count)
            {
                if (pv.pcap_loop_count > 0)
                    pv.pcap_loop_count--;

                if (pv.pcap_loop_count != 0)
                {
                    SF_QUEUE *tmp;

                    /* switch pcap lists */
                    tmp = pv.pcap_queue;
                    pv.pcap_queue = pv.pcap_save_queue;
                    pv.pcap_save_queue = tmp;

                    reopen_pcap = 1;
                }
            }

            if (reopen_pcap)
            {
                if (pv.pcap_reset){
                    //PcapReset(); wxh
		}

                /* reinitialize pcap */
                pcap_close(pd);
                OpenPcap();
                //SetPktProcessor(); wxh

                /* open a new tcpdump file - necessary because the snaplen could be
                 * different between pcaps */
                if (pv.log_bitmap & LOG_TCPDUMP)
                {
                    /* this sleep is to ensure we get a new log file since it has a
                     * time stamp with resolution to the second */
#ifdef WIN32
                    Sleep(1000);
#else
                    sleep(1);
#endif
                    //LogTcpdumpReset(); wxh
                }

                continue;
            }

            break;
        }

        /* continue... pcap_ret packets that time around. */
        pkts_to_read -= pcap_ret;

        if ((pkts_to_read <= 0) && (pv.pkt_cnt != -1))
        {
            break;
        }
      
        /* check for signals */
#if 0
        if (sig_check())
        {
            if (hup_check())
            {
                /* Actually return so we can restart */
                return NULL;
            }
        }
#endif

        /* idle time processing..quick things to check or do ... */
        //snort_idle(); wxh
    }
#endif
    if (pcap_ret < 0)
    {
        if(pv.daemon_flag)
        {
            syslog(LOG_PID | LOG_CONS | LOG_DAEMON,
                    "pcap_loop: %s", pcap_geterr(pd));
        }
        else
        {
            ErrorMessage("pcap_loop: %s\n", pcap_geterr(pd));
        }
        //CleanExit(1); wxh
    }
    
    signal_location =  0;

    pv.done_processing = 1;

    //CleanExit(0); wxh

    return NULL;                /* avoid warnings */
}

int OpenPcap()
{
    bpf_u_int32 localnet, netmask;        /* net addr holders */
    char errorbuf[PCAP_ERRBUF_SIZE];      /* buffer to put error strings in */
    bpf_u_int32 defaultnet = 0xFFFFFF00;
    static char first_pcap = 1;           /* for backwards compatibility only show first pcap */
    int ret;


    errorbuf[0] = '\0';

    /* if we're not reading packets from a file */
    if(pv.interface == NULL)
    {
        if (!pv.readmode_flag)
        {
            DEBUG_WRAP(DebugMessage(DEBUG_INIT,
                    "pv.interface is NULL, looking up interface....   "););
            /* look up the device and get the handle */
            pv.interface = pcap_lookupdev(errorbuf);
    
            DEBUG_WRAP(DebugMessage(DEBUG_INIT,
                    "found interface %s\n", PRINT_INTERFACE(pv.interface)););
            /* uh oh, we couldn't find the interface name */
            if(pv.interface == NULL)
            {
                FatalError("OpenPcap() interface lookup: \n        %s\n",
               errorbuf);
            }
        }
        else
        {
            /* interface is null and we are in readmode */
            /* some routines would hate it to be NULL */
            pv.interface = "[reading from a file]"; 
        }
    }

    if(!pv.quiet_flag)
    {
        if (!pv.readmode_flag)
        {
            LogMessage("\nInitializing Network Interface %s\n", 
                       PRINT_INTERFACE(pv.interface));
        }
        else if (first_pcap)
        {
            LogMessage("TCPDUMP file reading mode.\n");
        }
    }

    if (!pv.readmode_flag)
    {
        if(pv.pkt_snaplen)        /* if it's set let's try it... */
        {
            if(pv.pkt_snaplen < MIN_SNAPLEN)        /* if it's < MIN set it to
                                                     * MIN */
            {
                /* XXX: Warning message, specidifed snaplen too small,
                 * snaplen set to X
                 */
                 snaplen = MIN_SNAPLEN;
            }
            else
            {
                 snaplen = pv.pkt_snaplen;
            }
         }
         else
         {
             snaplen = SNAPLEN;        /* otherwise let's put the compiled value in */
         }
        
        DEBUG_WRAP(DebugMessage(DEBUG_INIT,
                "snaplength info: set=%d/compiled=%d/wanted=%d\n",
                snaplen,  SNAPLEN, pv.pkt_snaplen););
    
        /* get the device file descriptor */
        pd = pcap_open_live(pv.interface, snaplen,
                pv.promisc_flag ? PROMISC : 0, READ_TIMEOUT, errorbuf);

    }
    else
    {   /* reading packets from a file */

        if (sfqueue_count(pv.pcap_queue) > 0)
        {
            char *pcap = NULL;
            
            pcap = (char *)sfqueue_remove(pv.pcap_queue);
            if (pcap == NULL)
            {
                FatalError("Could not get pcap from list\n");
            }

            ret = SnortStrncpy(pv.readfile, pcap, sizeof(pv.readfile));
            if (ret != SNORT_STRNCPY_SUCCESS)
                FatalError("Could not copy pcap name to pv.readfile\n");

            ret = sfqueue_add(pv.pcap_save_queue, (NODE_DATA)pcap);
            if (ret == -1)
                FatalError("Could not add pcap to saved list\n");
        }

        if (!pv.quiet_flag)
        {
            if (first_pcap)
            {
                LogMessage("Reading network traffic from \"%s\" file.\n", 
                           pv.readfile);
            }
        }

        /* open the file */
        pd = pcap_open_offline(pv.readfile, errorbuf);

        /* the file didn't open correctly */
        if(pd == NULL)
        {
            FatalError("unable to open file \"%s\" for readback: %s\n",
                       pv.readfile, errorbuf);
        }

        /*
         * set the snaplen for the file (so we don't get a lot of extra crap
         * in the end of packets
         */
        snaplen = pcap_snapshot(pd);

        if(!pv.quiet_flag)
        {
            if (first_pcap)
            {
                LogMessage("snaplen = %d\n", snaplen);
            }
        }
    }

    /* something is wrong with the opened packet socket */
    if(pd == NULL)
    {
        if(strstr(errorbuf, "Permission denied"))
        {
            FatalError("You don't have permission to"
                       " sniff.\nTry doing this as root.\n");
        }
        else
        {
            FatalError("OpenPcap() device %s open: \n        %s\n",
                       PRINT_INTERFACE(pv.interface), errorbuf);
        }
    }

    if (strlen(errorbuf) > 0)
    {
        LogMessage("Warning: OpenPcap() device %s success with warning:"
                   "\n        %s\n", PRINT_INTERFACE(pv.interface), errorbuf);
    }

    /* get local net and netmask */
    if(pcap_lookupnet(pv.interface, &localnet, &netmask, errorbuf) < 0)
    {
       if (!pv.readmode_flag)
       {
           ErrorMessage("OpenPcap() device %s network lookup: \n"
                        "        %s\n",
                        PRINT_INTERFACE(pv.interface), errorbuf);

       }
        /*
         * set the default netmask to 255.255.255.0 (for stealthed
         * interfaces)
         */
        netmask = htonl(defaultnet);
    }

    /* compile BPF filter spec info fcode FSM */
#if 0
    if(pcap_compile(pd, &fcode, pv.pcap_cmd, 1, netmask) < 0)
    {
        FatalError("OpenPcap() FSM compilation failed: \n        %s\n"
                   "PCAP command: %s\n", pcap_geterr(pd), pv.pcap_cmd);
    }
    /* set the pcap filter */
    if(pcap_setfilter(pd, &fcode) < 0)
    {
        FatalError("OpenPcap() setfilter: \n        %s\n",
                   pcap_geterr(pd));
    }
    /* we can do this here now instead */
    /* of later before every pcap_close() */
    pcap_freecode(&fcode);
    
#endif
    /* get data link type */
    datalink = pcap_datalink(pd);

    if(datalink < 0)
    {
        FatalError("OpenPcap() datalink grab: \n        %s\n",
                   pcap_geterr(pd));
    }

    first_pcap = 0;

    return 0;
}



void
InitPcap( int test_flag )
{
#ifndef MUST_SPECIFY_DEVICE    
    if((pv.interface == NULL) && !pv.readmode_flag && !pv.print_version &&
#ifdef DYNAMIC_PLUGIN
        !pv.dump_dynamic_rules_flag &&
#endif
        !pv.test_mode_flag)
    {
        char errorbuf[PCAP_ERRBUF_SIZE];
#ifdef GIDS
        if (!InlineMode())
        {
#endif /* GIDS */
        pv.interface = pcap_lookupdev(errorbuf);

        if(pv.interface == NULL)
            FatalError( "Failed to lookup for interface: %s."
                    " Please specify one with -i switch\n", errorbuf);
        else
            LogMessage("***\n*** interface device lookup found: %s\n***\n",pv.interface);
#ifdef GIDS
        }
#endif /* GIDS */
    }
#else /* MUST_SPECIFY_DEVICE */
    if((pv.interface == NULL) && !pv.readmode_flag && !pv.print_version &&
#ifdef DYNAMIC_PLUGIN
        !pv.dump_dynamic_rules_flag &&
#endif
        !pv.test_mode_flag)
    {
            FatalError( "You must specify either: a network interface (-i), "
#ifdef DYNAMIC_PLUGIN
                        "dump dynamic rules to a file (--dump-dynamic-rules), "
#endif
                        "a capture file (-r), or the test flag (-T)\n");
    }
#endif /* MUST_SPECIFY_DEVICE */

    //g_pcap_test = test_flag;

    if (pv.test_mode_flag)
    {
       // ValidateBPF(); wxh
    }
    else
    {
        if(!pv.readmode_flag && !pv.print_version)
        {
            DEBUG_WRAP(DebugMessage(DEBUG_INIT, "%s interface: %s\n", 
                        test_flag ? "Testing" : "Opening", 
                        PRINT_INTERFACE(pv.interface)););
            /* open up our libpcap packet capture interface */
            OpenPcap();
        }
        else if (!pv.print_version)
        {
            DEBUG_WRAP(DebugMessage(DEBUG_INIT, "%s file: %s\n", 
                        test_flag ? "Testing" : "Opening", 
                        pv.readfile););
    
            /* open the packet file for readback */
            OpenPcap();
        }
    }

    /* If test mode, need to close pcap again. */
    if ( test_flag )
    {
#ifdef GIDS
        if (pd && !InlineMode())
#else
        if (pd)
#endif
        {
           pcap_close(pd);
           pd = NULL;
        }
    }
}

/* $Id$ */
/*
** Copyright (C) 1998-2002 Martin Roesch <roesch@sourcefire.com>
** Copyright (C) 2002-2008 Sourcefire, Inc.
**    Dan Roelker <droelker@sourcefire.com>
**    Marc Norton <mnorton@sourcefire.com>
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
**
** NOTES
**   5.7.02: Added interface for new detection engine. (Norton/Roelker)
**
*/

#define FASTPKT

/*  I N C L U D E S  **********************************************/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "cap.h"
#include "plugbase.h"
#include "debug.h"
#include "util.h"
#include "stream_api.h" 
#include "spp_frag3.h"
#include "spp_stream4.h"

extern ListHead Alert;         /* Alert Block Header */
extern ListHead Log;           /* Log Block Header */
extern ListHead Pass;          /* Pass Block Header */
extern ListHead Activation;    /* Activation Block Header */
extern ListHead Dynamic;       /* Dynamic Block Header */
extern ListHead Drop;
#ifdef GIDS
extern ListHead SDrop;
extern ListHead Reject;
#endif /* GIDS */

extern RuleTreeNode *rtn_tmp;      /* temp data holder */
extern OptTreeNode *otn_tmp;       /* OptTreeNode temp ptr */
extern ListHead *head_tmp;         /* ListHead temp ptr */

extern RuleListNode *RuleLists;
extern RuleListNode *nonDefaultRules;

extern int dynamic_rules_present;
extern int active_dynamic_nodes;

extern PreprocessFuncNode *PreprocessList;  /* Preprocessor function list */
extern PreprocGetReassemblyPktFuncNode *PreprocGetReassemblyPktList;

/*
**  The HTTP decode structre
*/
extern HttpUri UriBufs[URI_COUNT];

int do_detect;
extern StreamAPI *stream_api ;

int do_detect_content;
u_int16_t event_id;
char check_tags_flag;

void printRuleListOrder(RuleListNode * node);
int CheckTagging(Packet *p);
RuleListNode *addNodeToOrderedList(RuleListNode *ordered_list, 
        RuleListNode *node, int evalIndex);

#ifdef PERF_PROFILING
PreprocStats eventqPerfStats;
#endif

extern int dir;
int (*global_waf_cb)(session_info* info, char * buf);
int reg_waf_cb(int (*func) (session_info *info, char* buf))
{
        global_waf_cb=func;
        return 0;
}
session_info global_session_info;
int Preprocess(Packet * p)
{
	Frag3Defrag(p, NULL);
	if (p->frag_flag){
	}else{
        	ReassembleStream4(p, NULL);
	}
   
	if ( p->packet_flags & PKT_REBUILT_STREAM ){
		if ( dir == 1){
		   global_session_info.tuple.sip=(u_int32_t)(p->iph->ip_src.s_addr);;
		   global_session_info.tuple.dip=(u_int32_t)(p->iph->ip_dst.s_addr);;
		   global_session_info.tuple.sport=p->sp;
		   global_session_info.tuple.dport=p->dp;
		   *((char*)p->pkt+54+p->dsize)='\0';
		   global_waf_cb(&global_session_info,(char*)p->pkt+54);
		}
	}
	return 0;

    PreprocessFuncNode *idx;
    int retval = 0;
    PROFILE_VARS;

#ifdef PPM_MGR
    UINT64 pktcnt=0;

    /* Begin Packet Performance Monitoring  */
    if( PPM_PKTS_ENABLED() )
    {
        pktcnt = PPM_INC_PKT_CNT();
        PPM_GET_TIME();
        PPM_INIT_PKT_TIMER();
        if( PPM_DEBUG_PKTS() )
        {
           /* for debugging, info gathering, so don't worry about
           *  (unsigned) casting of pktcnt, were not likely to debug
           *  4G packets
           */
           LogMessage("PPM: Process-BeginPkt[%u] caplen=%u\n",
             (unsigned)pktcnt,p->pkth->caplen);
        }
    }
#endif
    
    /*
     *  If the packet has an invalid checksum marked, throw that
     *  traffic away as no end host should accept it.
     *
     *  This can be disabled by config checksum_mode: none
     */
    if(!p->csum_flags)
    {
        do_detect = do_detect_content = 1;
        idx = PreprocessList;

        /*
        **  Reset the appropriate application-layer protocol fields
        */
        p->uri_count = 0;
        UriBufs[0].decode_flags = 0;

        /*
        **  Turn on all preprocessors
        */

        while ((idx != NULL) && (!(p->packet_flags & PKT_PASS_RULE)))
        {
            assert(idx->func != NULL);
            if (IsPreprocBitSet(p, idx->preproc_bit))
            {
                idx->func(p, idx->context);
            }
            idx = idx->next;
        }

        check_tags_flag = 1;
    
        if ((do_detect) && (p->bytes_to_inspect != -1))
        {
            /* Check if we are only inspecting a portion of this packet... */
            if (p->bytes_to_inspect > 0)
            {
                DEBUG_WRAP(DebugMessage(DEBUG_DETECT, "Ignoring part of server "
                    "traffic -- only looking at %d of %d bytes!!!\n",
                    p->bytes_to_inspect, p->dsize););
                p->dsize = (u_int16_t)p->bytes_to_inspect;
            }

            //Detect(p); wxh
        }
        else if (p->bytes_to_inspect == -1)
        {
            DEBUG_WRAP(DebugMessage(DEBUG_DETECT, "Ignoring server traffic!!!\n"););
        }
    }
    else
    {
        DEBUG_WRAP(DebugMessage(DEBUG_DETECT, "Invalid Checksum, Ignoring traffic!!!\n"););
        pc.invalid_checksums++;
    }

    /*
    ** By checking tagging here, we make sure that we log the
    ** tagged packet whether it generates an alert or not.
    */
    PREPROC_PROFILE_START(eventqPerfStats);
    //CheckTagging(p); wxh

    //retval = SnortEventqLog(p); wxh
    //SnortEventqReset(); wxh

    PREPROC_PROFILE_END(eventqPerfStats);

    /* Simulate above behavior for preprocessor reassembled packets */
    if (do_detect && (p->bytes_to_inspect != -1))
    {
        if (p->packet_flags & PKT_PREPROC_RPKT)
        {
            PreprocGetReassemblyPktFuncNode *rpkt_idx = PreprocGetReassemblyPktList;

            /* Loop through the preprocessors that have registered a 
             * function to get a reassembled packet */
            while (rpkt_idx != NULL)
            {
                Packet *pp = NULL;

                assert(rpkt_idx->func != NULL);

                /* If the preprocessor bit is set, get the reassembled packet */
                if (IsPreprocGetReassemblyPktBitSet(p, rpkt_idx->preproc_id))
                    pp = (Packet *)rpkt_idx->func();

                if (pp != NULL)
                {
                    /* If the original packet's bytes to inspect is set,
                     * set it for the reassembled packet */
                    if (p->bytes_to_inspect > 0)
                        pp->dsize = (u_int16_t)p->bytes_to_inspect;

                    //if (Detect(pp)) //wxh
                    if (1)
                    {
                        PREPROC_PROFILE_START(eventqPerfStats);

                        //retval |= SnortEventqLog(pp); wxh
                        //SnortEventqReset();wxh

                        PREPROC_PROFILE_END(eventqPerfStats);
                    }
                }

                rpkt_idx = rpkt_idx->next;
            }
        }
    }

    //otn_tmp = NULL;

    /*
    **  If we found events in this packet, let's flush
    **  the stream to make sure that we didn't miss any
    **  attacks before this packet.
    */
    if(retval && stream_api)
        stream_api->alert_flush_stream(p);

    /**
     * See if we should go ahead and remove this flow from the
     * flow_preprocessor -- cmg
     */
    //CheckFlowShutdown(p); wxh

#ifdef PPM_MGR
    if( PPM_PKTS_ENABLED() )
    {
        PPM_GET_TIME();
        PPM_TOTAL_PKT_TIME();
        PPM_ACCUM_PKT_TIME();
        if( PPM_DEBUG_PKTS() )
        {
            LogMessage("PPM: Pkt[%u] Used= ",(unsigned)pktcnt);
            PPM_PRINT_PKT_TIME("%g usecs\n");
            LogMessage("PPM: Process-EndPkt[%u]\n\n",(unsigned)pktcnt);
        }
        PPM_PKT_LOG();
     }
     if( PPM_RULES_ENABLED() )
     {
        PPM_RULE_LOG(pktcnt,p);
     }
     if( PPM_PKTS_ENABLED() )
     {
        /* resets current packet time to ignore ip/tcp reassembly time */
        PPM_END_PKT_TIMER();
        PPM_RESET_PKT_TIMER();
     }
#endif

    
    return retval;
}
