/*
**  $Id$
**
**  perf.c
**
** Copyright (C) 2002-2008 Sourcefire, Inc.
** Dan Roelker <droelker@sourcefire.com>
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
**
**  DESCRIPTION
**    These are the basic functions that are needed to call performance
**    functions.
**
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#ifndef WIN32
#include <time.h>
#include <unistd.h>
#endif /* WIN32 */

#include "util.h"
#include "perf.h"

int InitPerfStats(SFPERF *sfPerf);
int UpdatePerfStats(SFPERF *sfPerf, const unsigned char *pucPacket, int len,
        int iRebuiltPkt);
int ProcessPerfStats(SFPERF *sfPerf);


int sfInitPerformanceStatistics(SFPERF *sfPerf)
{
    memset(sfPerf, 0x00, sizeof(SFPERF));
    sfSetPerformanceSampleTime(sfPerf, 0);
    sfSetPerformanceStatistics(sfPerf, 0);

    return 0;
}

int sfSetPerformanceSampleTime(SFPERF *sfPerf, int iSeconds)
{
    sfPerf->sample_time = 0;
    
    if(iSeconds < 0)
    {
        iSeconds = 0;
    }

    sfPerf->sample_interval = iSeconds;

    return 0;
}


int sfSetPerformanceAccounting(SFPERF *sfPerf, int iReset)
{
    sfPerf->sfBase.iReset = iReset;
    
    return 0;
}


int sfSetPerformanceStatistics(SFPERF *sfPerf, int iFlag)
{
    if(iFlag & SFPERF_BASE)
    {
        sfPerf->iPerfFlags = sfPerf->iPerfFlags | SFPERF_BASE;
    }

#ifndef LINUX_SMP

    if(iFlag & SFPERF_BASE_MAX)
    {
        sfPerf->sfBase.iFlags |= MAX_PERF_STATS;
    }

#endif

    if(iFlag & SFPERF_FLOW)
    {
        sfPerf->iPerfFlags = sfPerf->iPerfFlags | SFPERF_FLOW;
    }
    if(iFlag & SFPERF_EVENT)
    {
        sfPerf->iPerfFlags = sfPerf->iPerfFlags | SFPERF_EVENT;
    }
    if(iFlag & SFPERF_CONSOLE)
    {
        sfPerf->iPerfFlags = sfPerf->iPerfFlags | SFPERF_CONSOLE;
    }
    
    return 0;
}

int sfSetPerformanceStatisticsEx(SFPERF *sfPerf, int iFlag, void * p)
{
#ifndef WIN32    
    mode_t old_umask;
#endif 
    
    if(iFlag & SFPERF_FILE)
    {
        static char start_up = 1;

        sfPerf->iPerfFlags = sfPerf->iPerfFlags | SFPERF_FILE;
        
        SnortStrncpy(sfPerf->file, (char *)p, sizeof(sfPerf->file));

        /* this file needs to be readable by everyone */
#ifndef WIN32
        old_umask = umask(022);
#endif         

        /* append to existing perfmon file if just starting up */
        if (start_up)
        {
            sfPerf->fh = fopen(sfPerf->file, "a");
            start_up = 0;
        }
        /* otherwise we've rotated - start a new one */
        else
        {
            sfPerf->fh = fopen(sfPerf->file, "w");
        }

#ifndef WIN32
        umask(old_umask);
#endif
        
        if( !sfPerf->fh )
            return -1;
    }
    else if(iFlag & SFPERF_FILECLOSE)
    {
        if (sfPerf->fh)
        {
            fclose(sfPerf->fh);
            sfPerf->fh = NULL;
        }
    }
    else if(iFlag & SFPERF_PKTCNT)
    {
        sfPerf->iPktCnt = *(int*)p;
    }
    else if (iFlag & SFPERF_SUMMARY)
    {
        sfPerf->iPerfFlags |= SFPERF_SUMMARY;
    }
    return 0;
}


#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#ifndef FILE_MAX
#define FILE_MAX  (PATH_MAX + 256)
#endif

int sfRotatePerformanceStatisticsFile(SFPERF *sfPerf)
{
    int        ret;
    time_t     t;
    struct tm *tm;
    char       newfile[FILE_MAX];
    char      *ptr;
    int        prefix_len = 0;
#ifdef WIN32
    struct _stat stat_buf;
#else
    struct stat stat_buf;
#endif

    /* Close current stats file - if it is open already */
    if(!sfPerf->fh)
    {
        LogMessage("Performance log file '%s' not open",
                        sfPerf->file);

        return(1);
    }
    
    ret = fclose(sfPerf->fh);
    
    if ( ret != 0 )
    {
        FatalError("Cannot close performance log file '%s': %s\n",
                                    sfPerf->file, strerror(errno));
    }
    
    /* Rename current stats file with yesterday's date */
#ifndef WIN32
    ptr = strrchr(sfPerf->file, '/');
#else
    ptr = strrchr(sfPerf->file, '\\');
#endif

    if (ptr != NULL)
    {
        /* take length of string up to path separator and add 
         * one to include path separator */
        prefix_len = (ptr - &sfPerf->file[0]) + 1;
    }

    /* Get current time, then subtract one day to get yesterday */
    t = time(&t);
    t -= (24*60*60);
    tm = localtime(&t);
    SnortSnprintf(newfile, FILE_MAX, "%.*s%d-%02d-%02d",
                  prefix_len, sfPerf->file, tm->tm_year + 1900,
                  tm->tm_mon + 1, tm->tm_mday);

    /* Checking return code from rename */
#ifdef WIN32
    if (_stat(newfile, &stat_buf) == -1)
#else
    if (stat(newfile, &stat_buf) == -1)
#endif
    {
        /* newfile doesn't exist - just rename sfPerf->file to newfile */
        if(rename(sfPerf->file, newfile) != 0)
        {
            LogMessage("Cannot move performance log file '%s' to '%s': %s\n",
                       sfPerf->file, newfile,strerror(errno));
        }
    }
    else
    {
        /* append to current archive file */
        FILE *newfh, *curfh;
        char read_buf[1024];
        size_t num_read, num_wrote;

        do
        {
            newfh = fopen(newfile, "a");
            if (newfh == NULL)
            {
                LogMessage("Cannot open performance log archive file "
                           "'%s' for writing: %s\n",
                           newfile, strerror(errno));
                break;
            }

            curfh = fopen(sfPerf->file, "r");
            if (curfh == NULL)
            {
                LogMessage("Cannot open performance log file '%s' for reading: %s\n",
                           sfPerf->file, strerror(errno));
                fclose(newfh);
                break;
            }

            while (!feof(curfh))
            {
                num_read = fread(read_buf, sizeof(char), sizeof(read_buf), curfh);
                if (num_read < sizeof(read_buf))
                {
                    if (ferror(curfh))
                    {
                        /* a read error occurred */
                        LogMessage("Error reading performance log file '%s': %s\n",
                                   sfPerf->file, strerror(errno));
                        break;
                    }
                }

                if (num_read > 0)
                {
                    num_wrote = fwrite((const char *)read_buf, sizeof(char), num_read, newfh);
                    if (num_wrote != num_read)
                    {
                        if (ferror(newfh))
                        {
                            /* a bad write occurred */
                            LogMessage("Error writing to performance log "
                                       "archive file '%s': %s\n",
                                       newfile, strerror(errno));
                            break;
                        }
                    }
                }
            }

            fclose(newfh);
            fclose(curfh);

        } while (0);
    }

    ret = sfSetPerformanceStatisticsEx(sfPerf, SFPERF_FILE, sfPerf->file);

    if( ret != 0 )
    {
        FatalError("Cannot open performance log file '%s': %s\n",
                                    sfPerf->file, strerror(errno));
    }

    return 0;
}

int sfPerformanceStats(SFPERF *sfPerf, const unsigned char *pucPacket, int len,
                       int iRebuiltPkt)
{
    static unsigned int cnt=0;

    if (( cnt==0 || cnt >= sfPerf->iPktCnt ) &&
        !(sfPerf->iPerfFlags & SFPERF_SUMMARY))
    {
       cnt=1;
       CheckSampleInterval(time(NULL), sfPerf);
    }

    cnt++;

    UpdatePerfStats(sfPerf, pucPacket, len, iRebuiltPkt);

    return 0;
}

int CheckSampleInterval(time_t curr_time, SFPERF *sfPerf)
{
    time_t prev_time = sfPerf->sample_time;

    /*
    *  This is for when sfBasePerformance is
    *  starting up.
    */
    if(prev_time == 0)
    {
        InitPerfStats(sfPerf);
    }
    else if((curr_time - prev_time) >= sfPerf->sample_interval)
    {
        ProcessPerfStats(sfPerf);
        InitPerfStats(sfPerf);
    }

    return 0;
}

int InitPerfStats(SFPERF *sfPerf)
{
    /*
    *  Reset sample time for next sampling
    */
    sfPerf->sample_time = time(NULL);

    if(sfPerf->iPerfFlags & SFPERF_BASE)
    {  
        if(InitBaseStats(&(sfPerf->sfBase)))
            return -1;
    }

    if(sfPerf->iPerfFlags & SFPERF_FLOW)
    {  
        InitFlowStats(&(sfPerf->sfFlow));
    }

    if(sfPerf->iPerfFlags & SFPERF_EVENT)
    {  
        InitEventStats(&(sfPerf->sfEvent));
    }

    return 0;
}

int ResetPerfStats(SFPERF *sfPerf)
{
    return InitPerfStats(sfPerf);
}

int UpdatePerfStats(SFPERF *sfPerf, const unsigned char *pucPacket, int len,
                    int iRebuiltPkt)
{
    if(sfPerf->iPerfFlags & SFPERF_BASE)
    {
        UpdateBaseStats(&(sfPerf->sfBase), len, iRebuiltPkt);
    }
    if(sfPerf->iPerfFlags & SFPERF_FLOW)
    {
        UpdateFlowStats(&(sfPerf->sfFlow), pucPacket, len, iRebuiltPkt);
    }

    return 0;
}

int sfProcessPerfStats(SFPERF *sfPerf)
{
    return ProcessPerfStats(sfPerf);
}

int ProcessPerfStats(SFPERF *sfPerf)
{
    if(sfPerf->iPerfFlags & SFPERF_BASE)
    {
        /* Allow this to go out to console and/or a file */
        ProcessBaseStats(&(sfPerf->sfBase),
                sfPerf->iPerfFlags & SFPERF_CONSOLE,
                sfPerf->iPerfFlags & SFPERF_FILE,
                sfPerf->fh );
    }
    
    /* Always goes to the console */
    if(sfPerf->iPerfFlags & SFPERF_FLOW)
    {
        if( sfPerf->iPerfFlags & SFPERF_CONSOLE )
            ProcessFlowStats(&(sfPerf->sfFlow));
    }
   
    if(sfPerf->iPerfFlags & SFPERF_EVENT)
    {
        if( sfPerf->iPerfFlags & SFPERF_CONSOLE )
            ProcessEventStats(&(sfPerf->sfEvent));
    }

    return 0;
}
    
/*
** $Id$
**
** perf-flow.c
**
**
** Copyright (C) 2002-2008 Sourcefire, Inc.
** Marc Norton <mnorton@sourcefire.com>
** Dan Roelker <droelker@sourcefire.com>
**
** NOTES
**   4.10.02 - Initial Checkin.  Norton
**   5.5.02  - Changed output format and added output structure for
**             easy stat printing. Roelker
**   5.29.02 - Added ICMP traffic stats and overall protocol flow 
**             stats. Roelker
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
**  DESCRIPTION
**    The following subroutines track eand analyze the traffic flow
**  statistics.
**
**   PacketLen vs Packet Count
**   TCP-Port vs Packet Count
**   UDP-Port vs Packet Count
**   TCP High<->High Port Count 
**   UDP High<->High Port Count
**
**
*/

#include <time.h>
#ifndef WIN32
#include <sys/time.h>
#include <sys/resource.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cap.h"
#include "util.h"
#include "sf_types.h" 

int DisplayFlowStats(SFFLOW_STATS *sfFlowStats);

/*
**  Quick function to return the correct
**  FlowStats ptr for us
*/
SFFLOW *sfGetFlowPtr() { return &sfPerf.sfFlow; }

/*
*  Allocate Memory, initialize arrays, etc...
*/
int InitFlowStats(SFFLOW *sfFlow)
{
    static char first = 1;

    if (first)
    {
        sfFlow->pktLenCnt = (UINT64*)SnortAlloc(sizeof(UINT64) * (SF_MAX_PKT_LEN + 1));
        sfFlow->portTcpSrc = (UINT64*)SnortAlloc(sizeof(UINT64) * SF_MAX_PORT);
        sfFlow->portTcpDst = (UINT64*)SnortAlloc(sizeof(UINT64) * SF_MAX_PORT);
        sfFlow->portUdpSrc = (UINT64*)SnortAlloc(sizeof(UINT64) * SF_MAX_PORT);
        sfFlow->portUdpDst = (UINT64*)SnortAlloc(sizeof(UINT64) * SF_MAX_PORT);
        sfFlow->typeIcmp = (UINT64 *)SnortAlloc(sizeof(UINT64) * 256);

        first = 0;
    }
    else
    {
        memset(sfFlow->pktLenCnt, 0, sizeof(UINT64) * (SF_MAX_PKT_LEN + 1));
        memset(sfFlow->portTcpSrc, 0, sizeof(UINT64) * SF_MAX_PORT);
        memset(sfFlow->portTcpDst, 0, sizeof(UINT64) * SF_MAX_PORT);
        memset(sfFlow->portUdpSrc, 0, sizeof(UINT64) * SF_MAX_PORT);
        memset(sfFlow->portUdpDst, 0, sizeof(UINT64) * SF_MAX_PORT);
        memset(sfFlow->typeIcmp, 0, sizeof(UINT64) * 256);
    }

    sfFlow->pktTotal = 0;
    sfFlow->byteTotal = 0;

    sfFlow->portTcpHigh=0;
    sfFlow->portTcpTotal=0;

    sfFlow->portUdpHigh=0;
    sfFlow->portUdpTotal=0;

    sfFlow->typeIcmpTotal = 0;
    
    return 0;
}

int UpdateTCPFlowStats(SFFLOW *sfFlow, int sport, int dport, int len )
{
    /*
    ** Track how much data on each port, and hihg<-> high port data
    */
    /*
    if( sport < sfFlow->maxPortToTrack )
    {
        sfFlow->portTcpSrc  [ sport ]+= len;
    }
   
    if( dport < sfFlow->maxPortToTrack )
    {
        sfFlow->portTcpDst  [ dport ]+= len;
    }
    
    if( sport > 1023 && dport > 1023 )
    {
        sfFlow->portTcpHigh += len;
    }
    */
    if( sport <  1024 && dport > 1023 ) //sfFlow->maxPortToTrack )
    {
        sfFlow->portTcpSrc  [ sport ]+= len;
    }
    else if( dport < 1024 && sport > 1023 ) //sfFlow->maxPortToTrack )
    {
        sfFlow->portTcpDst  [ dport ]+= len;
    }
    else if( sport < 1023 && dport < 1023 )
    {
        sfFlow->portTcpSrc  [ sport ]+= len;
        sfFlow->portTcpDst  [ dport ]+= len;
    }
    else if( sport > 1023 && dport > 1023 )
    {
        sfFlow->portTcpSrc  [ sport ]+= len;
        sfFlow->portTcpDst  [ dport ]+= len;
        
        sfFlow->portTcpHigh += len;
    }


    sfFlow->portTcpTotal += len;

    return 0;
}

int UpdateTCPFlowStatsEx(int sport, int dport, int len )
{
    if(!(sfPerf.iPerfFlags & SFPERF_FLOW))
       return 1;

    return UpdateTCPFlowStats( sfGetFlowPtr(), sport, dport, len );
}

int UpdateUDPFlowStats(SFFLOW *sfFlow, int sport, int dport, int len )
{
    /*
     * Track how much data on each port, and hihg<-> high port data
     */
    if( sport <  1024 && dport > 1023 ) //sfFlow->maxPortToTrack )
    {
        sfFlow->portUdpSrc  [ sport ]+= len;
    }
    else if( dport < 1024 && sport > 1023 ) //sfFlow->maxPortToTrack )
    {
        sfFlow->portUdpDst  [ dport ]+= len;
    }
    else if( sport < 1023 && dport < 1023 )
    {
        sfFlow->portUdpSrc  [ sport ]+= len;
        sfFlow->portUdpDst  [ dport ]+= len;
    }
    else if( sport > 1023 && dport > 1023 )
    {
        sfFlow->portUdpSrc  [ sport ]+= len;
        sfFlow->portUdpDst  [ dport ]+= len;
        
        sfFlow->portUdpHigh += len;
    }

    sfFlow->portUdpTotal += len;

    return 0;
}

int UpdateUDPFlowStatsEx(int sport, int dport, int len )
{
    if(!(sfPerf.iPerfFlags & SFPERF_FLOW))
       return 1;

    return UpdateUDPFlowStats( sfGetFlowPtr(), sport, dport, len );
}

int UpdateICMPFlowStats(SFFLOW *sfFlow, int type, int len)
{
    if(type < 256)
    {
        sfFlow->typeIcmp[type] += len;
    }

    sfFlow->typeIcmpTotal += len;

    return 0;
}

int UpdateICMPFlowStatsEx(int type, int len)
{
    if(!(sfPerf.iPerfFlags & SFPERF_FLOW))
        return 1;

    return UpdateICMPFlowStats(sfGetFlowPtr(), type, len);
}

/*
*   Add in stats for this packet
*
*   Packet lengths
*/
int UpdateFlowStats(SFFLOW *sfFlow, const unsigned char *pucPacket, int len,
        int iRebuiltPkt)
{
    /*
    * Track how many packets of each length
    */
    if( len <= SF_MAX_PKT_LEN )
    {
        sfFlow->pktLenCnt[ len ]++;
        sfFlow->pktTotal++;
        sfFlow->byteTotal += len;
    }

    return 0;
}

/*
*   Analyze/Calc Stats and Display them.
*/
int ProcessFlowStats(SFFLOW *sfFlow)
{
    static SFFLOW_STATS sfFlowStats;
    int i;
    double rate, srate, drate, totperc;
    UINT64 tot;

    memset(&sfFlowStats, 0x00, sizeof(sfFlowStats));

    /*
    **  Calculate the percentage of TCP, UDP and ICMP
    **  and other traffic that consisted in the stream.
    */
    sfFlowStats.trafficTCP = 100.0 * (double)(sfFlow->portTcpTotal) /
                 (double)(sfFlow->byteTotal);
    sfFlowStats.trafficUDP = 100.0 * (double)(sfFlow->portUdpTotal) /
                 (double)(sfFlow->byteTotal);
    sfFlowStats.trafficICMP = 100.0 * (double)(sfFlow->typeIcmpTotal) /
                 (double)(sfFlow->byteTotal);
    sfFlowStats.trafficOTHER = 100.0 *
                   (double)((double)sfFlow->byteTotal -
                   ((double)sfFlow->portTcpTotal +
                   (double)sfFlow->portUdpTotal +
                   (double)sfFlow->typeIcmpTotal)) /
                   (double)sfFlow->byteTotal;
    
    /*
    **  Calculate Packet percent of total pkt length
    **  distribution.
    */
    for(i=1;i<SF_MAX_PKT_LEN;i++)
    {
        if( !sfFlow->pktLenCnt[i]  ) continue;
     
        rate =  100.0 * (double)(sfFlow->pktLenCnt[i]) / 
                (double)(sfFlow->pktTotal);

        if( rate > .10 )
        {
            sfFlowStats.pktLenPercent[i] = rate;
        }
        else
        {
            sfFlowStats.pktLenPercent[i] = 0;
        }  
      
        sfFlow->pktLenCnt[i]=0;
    }

    /*
    **  Calculate TCP port distribution by src, dst and
    **  total percentage.
    */
    for(i=0;i<sfFlow->maxPortToTrack;i++)
    {
        tot = sfFlow->portTcpSrc[i]+sfFlow->portTcpDst[i];
        if(!tot)
        {
            sfFlowStats.portflowTCP.totperc[i] = 0;
            continue;
        }

        totperc = 100.0 * tot / sfFlow->portTcpTotal;
        
        if(totperc > .1)
        {
            srate =  100.0 * (double)(sfFlow->portTcpSrc[i]) / tot ;
            drate =  100.0 * (double)(sfFlow->portTcpDst[i]) / tot ;
        
            sfFlowStats.portflowTCP.totperc[i]    = totperc;
            sfFlowStats.portflowTCP.sport_rate[i] = srate;
            sfFlowStats.portflowTCP.dport_rate[i] = drate;
        }
        else
        {
            sfFlowStats.portflowTCP.totperc[i] = 0;
        }
        
        sfFlow->portTcpSrc[i] = sfFlow->portTcpDst[i] = 0;
    }

    sfFlowStats.portflowHighTCP = 100.0 * sfFlow->portTcpHigh /
                                  sfFlow->portTcpTotal;

    /*
    **  Reset counters for next go round.
    */
    sfFlow->portTcpHigh=0;
    sfFlow->portTcpTotal=0;
    
    /*
    **  Calculate UDP port processing based on src, dst and
    **  total distributions.
    */
    for(i=0;i<sfFlow->maxPortToTrack;i++)
    {
        tot = sfFlow->portUdpSrc[i]+sfFlow->portUdpDst[i];
        if(!tot)
        {
            sfFlowStats.portflowUDP.totperc[i] = 0;
            continue;
        }

        totperc= 100.0 * tot / sfFlow->portUdpTotal;
        
        if(totperc > .1)
        {
            srate =  100.0 * (double)(sfFlow->portUdpSrc[i]) / tot ;
            drate =  100.0 * (double)(sfFlow->portUdpDst[i]) / tot ;

            sfFlowStats.portflowUDP.totperc[i]    = totperc;
            sfFlowStats.portflowUDP.sport_rate[i] = srate;
            sfFlowStats.portflowUDP.dport_rate[i] = drate;
        }
        else
        {
            sfFlowStats.portflowUDP.totperc[i] = 0;
        }
        
        sfFlow->portUdpSrc[i] = sfFlow->portUdpDst[i] = 0;
    }

    sfFlowStats.portflowHighUDP = 100.0 * sfFlow->portUdpHigh /
                                  sfFlow->portUdpTotal;

    /*
    **  Reset counters for next go round
    */
    sfFlow->portUdpHigh=0;
    sfFlow->portUdpTotal=0;

    /*
    **  Calculate ICMP statistics
    */
    for(i=0;i<256;i++)
    {
        tot = sfFlow->typeIcmp[i];
        if(!tot)
        {
            sfFlowStats.flowICMP.totperc[i] = 0;
            continue;
        }

        totperc= 100.0 * tot / sfFlow->typeIcmpTotal;
        
        if(totperc > .1)
        {
            sfFlowStats.flowICMP.totperc[i]  = totperc;
        }
        else
        {
            sfFlowStats.flowICMP.totperc[i] = 0;
        }

        sfFlow->typeIcmp[i] = 0;
    }

    sfFlow->typeIcmpTotal = 0;

    sfFlow->byteTotal = 0;
   
    sfFlow->pktTotal  = 0; 
 
    DisplayFlowStats(&sfFlowStats);

    return 0;
}
                                                
int DisplayFlowStats(SFFLOW_STATS *sfFlowStats)
{
    int i;
  
    LogMessage("\n\nProtocol Byte Flows - %%Total Flow\n");
    LogMessage(    "--------------------------------------\n");
    LogMessage("TCP:   %.2f%%\n", sfFlowStats->trafficTCP);
    LogMessage("UDP:   %.2f%%\n", sfFlowStats->trafficUDP);
    LogMessage("ICMP:  %.2f%%\n", sfFlowStats->trafficICMP);
    LogMessage("OTHER: %.2f%%\n", sfFlowStats->trafficOTHER);

    LogMessage("\n\nPacketLen - %%TotalPackets\n");
    LogMessage(    "-------------------------\n"); 
    for(i=1;i<SF_MAX_PKT_LEN;i++)
    {
        if( sfFlowStats->pktLenPercent[i] < .1 ) continue;
     
        LogMessage("Bytes[%d] %.2f%%\n", i, sfFlowStats->pktLenPercent[i]);
    }

    LogMessage("\n\nTCP Port Flows\n");
    LogMessage(    "--------------\n"); 
    for(i=0;i<SF_MAX_PORT;i++)
    {
        if(sfFlowStats->portflowTCP.totperc[i] && 
           sfFlowStats->portflowTCP.dport_rate[i]  )
        {
            LogMessage("Port[%d] %.2f%% of Total, Src: %6.2f%% Dst: %6.2f%%\n",
                        i, sfFlowStats->portflowTCP.totperc[i],
                        sfFlowStats->portflowTCP.sport_rate[i],
                        sfFlowStats->portflowTCP.dport_rate[i]);
        }
    }

    if(sfFlowStats->portflowHighTCP > .1)
    {
        LogMessage("Ports[High<->High]: %.2f%%\n", 
                sfFlowStats->portflowHighTCP);
    }

    LogMessage("\n\nUDP Port Flows\n");
    LogMessage(    "--------------\n"); 
    for(i=0;i<SF_MAX_PORT;i++)
    {
        if(sfFlowStats->portflowUDP.totperc[i] && 
           sfFlowStats->portflowUDP.dport_rate[i]  )
        {
            LogMessage("Port[%d] %.2f%% of Total, Src: %6.2f%% Dst: %6.2f%%\n",
                        i, sfFlowStats->portflowUDP.totperc[i],
                        sfFlowStats->portflowUDP.sport_rate[i],
                        sfFlowStats->portflowUDP.dport_rate[i]);
        }
    }

    if(sfFlowStats->portflowHighUDP > .1)
    {
        LogMessage("Ports[High<->High]: %.2f%%\n", 
                sfFlowStats->portflowHighUDP);
    }

    LogMessage("\n\nICMP Type Flows\n");
    LogMessage(    "---------------\n");
    for(i=0;i<256;i++)
    {
        if(sfFlowStats->flowICMP.totperc[i])
        {
            LogMessage("Type[%d] %.2f%% of Total\n",
                        i, sfFlowStats->flowICMP.totperc[i]);
        }
    }

         
    return 0;
}

/*
**  $Id$
**
**  perf-event.c
**
**  Copyright (C) 2002-2008 Sourcefire, Inc.
**  Marc Norton <mnorton@sourcefire.com>
**  Dan Roelker <droelker@sourcefire.com>
**
**  NOTES
**  5.28.02 - Initial Source Code. Norton/Roelker
**
**
**  This program is free software; you can redistribute it and/or modify
**  it under the terms of the GNU General Public License Version 2 as
**  published by the Free Software Foundation.  You may not use, modify or
**  distribute this program under any other version of the GNU General
**  Public License.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program; if not, write to the Free Software
**  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
*/

#include "cap.h"
#include "util.h"

int DisplayEventPerfStats(SFEVENT_STATS *sfEventStats);

SFEVENT *GetEventPtr() { return &sfPerf.sfEvent; }

int InitEventStats(SFEVENT *sfEvent)
{
    sfEvent->NQEvents = 0;
    sfEvent->QEvents  = 0;

    return 0;
}

int UpdateNQEvents()
{
    SFEVENT *sfEvent = GetEventPtr();

    if(!(sfPerf.iPerfFlags & SFPERF_EVENT))
    {
        return 0;
    }

    sfEvent->NQEvents++;
    sfEvent->TotalEvents++;

    return 0;
}

int UpdateQEvents()
{
    SFEVENT *sfEvent = GetEventPtr();

    if(!(sfPerf.iPerfFlags & SFPERF_EVENT))
    {
        return 0;
    }

    sfEvent->QEvents++;
    sfEvent->TotalEvents++;

    return 0;
}

int ProcessEventStats(SFEVENT *sfEvent)
{
    SFEVENT_STATS sfEventStats;

    sfEventStats.NQEvents = sfEvent->NQEvents;
    sfEventStats.QEvents = sfEvent->QEvents;
    sfEventStats.TotalEvents = sfEvent->TotalEvents;

    if(sfEvent->TotalEvents)
    {
        sfEventStats.NQPercent = 100.0 * (double)sfEvent->NQEvents / 
                                 (double)sfEvent->TotalEvents;
        sfEventStats.QPercent  = 100.0 * (double)sfEvent->QEvents / 
                                 (double)sfEvent->TotalEvents;
    }
    else
    {
        sfEventStats.NQPercent = 0;
        sfEventStats.QPercent = 0;
    }

    sfEvent->NQEvents    = 0;
    sfEvent->QEvents     = 0;
    sfEvent->TotalEvents = 0;

    DisplayEventPerfStats(&sfEventStats);

    return 0;
}

int DisplayEventPerfStats(SFEVENT_STATS *sfEventStats)
{
    LogMessage("\n\nSnort Setwise Event Stats\n");
    LogMessage(    "-------------------------\n");

    LogMessage( "Total Events:           " STDu64 "\n", sfEventStats->TotalEvents);
    LogMessage( "Qualified Events:       " STDu64 "\n", sfEventStats->QEvents);
    LogMessage( "Non-Qualified Events:   " STDu64 "\n", sfEventStats->NQEvents);

    LogMessage("%%Qualified Events:      %.4f%%\n", sfEventStats->QPercent);
    LogMessage("%%Non-Qualified Events:  %.4f%%\n", sfEventStats->NQPercent);

    return 0;
}
    

/*
** $Id$
**
** perf-base.c
**
** Copyright (C) 2002-2008 Sourcefire, Inc.
** Dan Roelker <droelker@sourcefire.com>
** Marc Norton <mnorton@sourcefire.com>
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
**  DESCRIPTION
**    The following subroutines are concerned with getting
**    basic stats on packet bytes and times that an app
**    takes in processing packets.  The times measured are
**    kernel and user time for the process.   Real-time
**    (wall clock) is also measured to show when processing
**    has reached capacity and to measure the true processing 
**    that the app is currently doing.
**
**  NOTES
**    4.8.02  : Initial Code (DJR,MAN)
**    4.22.02 : Added Comments (DJR)
**    7.10.02 : Added sfprocpidstats code for SMP linux (DJR)
**    8.8.02  : Added stream4 instrumentation (cmg)
**    9.1.04  : Removed NO_PKTS, ACCUMULATE/RESET #defines, now we use SFBASE->iReset
**              and the permonitor command has 'reset' and 'accrue' commands instead.(MAN)
**    10.4.06 : Added UDP Session Stats (SAS)
**    4.3.07  : Added stats for TCP sessions (SAS)
*/

#include <time.h>
#ifndef WIN32
#include <sys/time.h>
#include <sys/resource.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>

#include "cap.h"
//#include "inline.h"
#include "util.h"
//#include "mpse.h"
#include "stream_api.h"
#include "sf_types.h"

int GetPktDropStats(SFBASE *sfBase, SFBASE_STATS *sfBaseStats);
int DisplayBasePerfStatsConsole(SFBASE_STATS *sfBaseStats, int iFlags);
int CalculateBasePerfStats(SFBASE *sfPerf, SFBASE_STATS *sfBaseStats);
int LogBasePerfStats(SFBASE_STATS *sfBaseStats,  FILE * fh );

/*
**  NAME
**    InitBaseStats
**  DESCRIPTION
**    Initializes structs and variables for the next performance
**    sample.
**
**  FORMAL INPUTS
**    SFBASE * -- pointer to structure to initialize
** 
**  FORMAL OUTPUTS
**    int -- 0 is successful
*/ 
int InitBaseStats(SFBASE *sfBase)
{
    int todRet = -1;
    struct timeval tvTime;

#ifndef WIN32
#ifndef LINUX_SMP
    struct rusage  rusage;
    int rusageRet = -1;
#endif
    
#ifdef LINUX_SMP
    static int first_time = 0;

    if(!first_time)
    {
        sfInitProcPidStats(&(sfBase->sfProcPidStats));
        first_time = 1;
    }
    todRet = gettimeofday(&tvTime, NULL);
#else
    
    rusageRet = getrusage(RUSAGE_SELF, &rusage);
    todRet = gettimeofday(&tvTime, NULL);

    if (rusageRet >= 0)
    {
        sfBase->usertime_sec   = (double)rusage.ru_utime.tv_sec +
                                 ((double)rusage.ru_utime.tv_usec * 1.0e-6);
        sfBase->systemtime_sec = (double)rusage.ru_stime.tv_sec +
                                 ((double)rusage.ru_stime.tv_usec * 1.0e-6);
    }
    else
    {
        sfBase->usertime_sec = 0;
        sfBase->systemtime_sec = 0;
    }

#endif  /* !LINUX_SMP */
#else
    sfBase->usertime_sec = 0;
    sfBase->systemtime_sec = 0;
    todRet = gettimeofday(&tvTime, NULL);
#endif  /* !WIN32 */

    if(todRet >= 0)
    {
        sfBase->realtime_sec = (double)tvTime.tv_sec +
                               ((double)tvTime.tv_usec * 1.0e-6);
    }
    else
    {
        sfBase->realtime_sec = 0;
    }

    sfBase->total_blocked_packets = 0;
    sfBase->total_wire_packets = 0;
    sfBase->total_ipfragmented_packets = 0;
    sfBase->total_ipreassembled_packets = 0;
    sfBase->total_packets = 0;
    sfBase->total_rebuilt_packets = 0;

    sfBase->total_wire_bytes = 0;
    sfBase->total_ipfragmented_bytes = 0;
    sfBase->total_ipreassembled_bytes = 0;
    sfBase->total_bytes = 0;
    sfBase->total_rebuilt_bytes = 0;
    sfBase->total_blocked_bytes = 0;

    sfBase->iNewSessions = 0;
    sfBase->iDeletedSessions = 0;

    sfBase->iStreamFlushes = 0;
    sfBase->iStreamFaults = 0;
    sfBase->iStreamTimeouts = 0;
    //sfBase->iMaxSessions = 0;
    //sfBase->iMaxSessionsInterval = 0;
    //sfBase->iMidStreamSessions = 0;
    //sfBase->iClosedSessions = 0;
    //sfBase->iPrunedSessions = 0;
    //sfBase->iDroppedAsyncSessions = 0;
    //sfBase->iSessionsInitializing = 0;
    //sfBase->iSessionsEstablished = 0;
    //sfBase->iSessionsClosing = 0;
    
    sfBase->iFragCreates = 0;
    sfBase->iFragCompletes = 0;
    sfBase->iFragInserts = 0;
    sfBase->iFragDeletes = 0;
    sfBase->iFragAutoFrees = 0;
    sfBase->iFragFlushes = 0;
    sfBase->iFragTimeouts = 0;
    sfBase->iFragFaults = 0;

    sfBase->iNewUDPSessions = 0;
    sfBase->iDeletedUDPSessions = 0;

    //sfBase->iAttributeHosts = 0;
    //sfBase->iAttributeReloads = 0;
    
    return 0;
}

/*
**  NAME
**    UpdateBaseStats
**
**  DESCRIPTION
**    Simple update of stats.
**
**  FORMAL INPUTS
**    SFBASE * - structure to update
**    int      - length of packet payload in bytes
**
**  FORMAL OUTPUTS
**    int - 0 is successful
**
**  Add in Ethernet Overhead - assume a standerd Ethernet service
**
**   Ethernet Frame
**   ---------------
**           | <-----------   PCAP Packet  --------> |
**   Preamble  Dest Mac  Src Mac   Type      Payload   CRC        IFG
** | 8 bytes | 6 Bytes | 6 Bytes | 2-Bytes | 46-1500 | 4 Bytes |  12      |
**
** Len = PCAP Packet + 4 bytes for CRC
** Overhead = 20 bytes
** Min on the wire == 84 bytes
** Min Size of PCAP packet = 60 bytes (84 - 20 overhead - 4 CRC)
**
** Len is the amount of user data being sent.  This will be less then
** actual wire-speed, because of the interframe gap (96 bits) and preamble
** (8 bytes).
**
** A 60 byte minimum packet uses 672 bits (60 bytes + 4 CRC), this limits a
** 1000 Mbit network to 1.488 Million packets with a bandwidth of 760
** Mbits.  The lost 240 Mbits is due to interframe gap (96 bits) and preamble
** (8 bytes).
**
** Even if the actual data is only 40 bytes per packet (ie, an empty
** TCP ACK), wire data is still 64 bytes per packet, even though actual
** packet size is 40 bytes.  Bandwith drops to 480 Mbits.  
**
** This explains why when a network goes over 50% capactiy you are closer to
** the edge than you realize, depending on the traffic profile.  At 75% you 
** are at the limit of your network, if you can get there.
**
** iRebuiltPkt determines whether the packet is rebuilt or not.  We keep
** separate statistics between wire pkts and rebuilt pkts.
**
*/
int UpdateBaseStats(SFBASE *sfBase, int len, int iRebuiltPkt)
{
    /* If rebuilt, count info for TCP rebuilt packet */
    if(iRebuiltPkt)
    {
        sfBase->total_rebuilt_bytes += len;
        sfBase->total_rebuilt_packets++;
    }
    else
    {
        len += 4; /* for the CRC */
    }

    /* Includes wire, IP reassembled & TCP rebuilt packets
     * that make it to the application layer.
     */
    sfBase->total_packets++;

    sfBase->total_bytes += len;

    return 0;
}

/*
**  NAME
**    UpdateWireStats
**
**  DESCRIPTION
**    Simple update of stats for "on the wire".
**
**  FORMAL INPUTS
**    SFBASE * - structure to update
**    int      - length of packet payload in bytes
**
**  FORMAL OUTPUTS
**    none
*/
void UpdateWireStats(SFBASE *sfBase, int len)
{
    sfBase->total_wire_packets++;

    len += 4; /* for the CRC */
    sfBase->total_wire_bytes += len;
   
#if 0
    if( InlineWasPacketDropped() )
    {
      sfBase->total_blocked_packets++;
      sfBase->total_blocked_bytes += len;
    }
#endif
}

/*
**  NAME
**    UpdateIPFragStats
**
**  DESCRIPTION
**    Simple update of stats for IP fragmented packets
**
**  FORMAL INPUTS
**    SFBASE * - structure to update
**    int      - length of packet payload in bytes
**
**  FORMAL OUTPUTS
**    none
*/

/*
**  NAME
**    UpdateIPReassStats
**
**  DESCRIPTION
**    Simple update of stats for IP reassembled packets
**
**  FORMAL INPUTS
**    SFBASE * - structure to update
**    int      - length of packet payload in bytes
**
**  FORMAL OUTPUTS
**    none
*/
void UpdateIPReassStats(SFBASE *sfBase, int len)
{
    sfBase->total_ipreassembled_packets++;

    len += 4; /* for the CRC */
    sfBase->total_wire_bytes += len;
    sfBase->total_ipreassembled_bytes += len;
}

/*
**  NAME
**    AddStreamSession
**
**  DESCRIPTION
**    Add a session count
**
**  FORMAL INPUTS
**    SFBASE * - ptr to update.
**
**  FORMAL OUTPUTS
**    int - 0 is successful
*/

int AddStreamSession(SFBASE *sfBase, u_int32_t flags)
{    
    sfBase->iTotalSessions++;
    sfBase->iNewSessions++;

    if (flags & SSNFLAG_MIDSTREAM)
        sfBase->iMidStreamSessions++;

    if(sfBase->iTotalSessions > sfBase->iMaxSessions)
        sfBase->iMaxSessions = sfBase->iTotalSessions;

    if(sfBase->iTotalSessions > sfBase->iMaxSessionsInterval)
        sfBase->iMaxSessionsInterval = sfBase->iTotalSessions;

    return 0;
}

/*
**  NAME
**    CloseStreamSession
**
**  DESCRIPTION
**    Add a session count
**
**  FORMAL INPUTS
**    SFBASE * - ptr to update.
**
**  FORMAL OUTPUTS
**    int - 0 is successful
*/

int CloseStreamSession(SFBASE *sfBase, char flags)
{
    if (flags & SESSION_CLOSED_NORMALLY)
        sfBase->iClosedSessions++;
    else if (flags & SESSION_CLOSED_TIMEDOUT)
        sfBase->iStreamTimeouts++;
    else if (flags & SESSION_CLOSED_PRUNED)
        sfBase->iPrunedSessions++;
    else if (flags & SESSION_CLOSED_ASYNC)
        sfBase->iDroppedAsyncSessions++;

    return 0;
}

/*
**  NAME
**    RemoveStreamSession
**
**  DESCRIPTION
**    Add a session count
**
**  FORMAL INPUTS
**    SFBASE * - ptr to update.
**
**  FORMAL OUTPUTS
**    int - 0 is successful
*/

int RemoveStreamSession(SFBASE *sfBase)
{
    sfBase->iTotalSessions--;
    sfBase->iDeletedSessions++;
    return 0;
}

/*
**  NAME
**    AddUDPSession
**
**  DESCRIPTION
**    Add a session count
**
**  FORMAL INPUTS
**    SFBASE * - ptr to update.
**
**  FORMAL OUTPUTS
**    int - 0 is successful
*/
int AddUDPSession(SFBASE *sfBase)
{    
    sfBase->iTotalUDPSessions++;
    sfBase->iNewUDPSessions++;

    if(sfBase->iTotalUDPSessions > sfBase->iMaxUDPSessions)
        sfBase->iMaxUDPSessions = sfBase->iTotalUDPSessions;

    return 0;
}

/*
**  NAME
**    RemoveUDPSession
**
**  DESCRIPTION
**    Add a session count
**
**  FORMAL INPUTS
**    SFBASE * - ptr to update.
**
**  FORMAL OUTPUTS
**    int - 0 is successful
*/

int RemoveUDPSession(SFBASE *sfBase)
{
    sfBase->iTotalUDPSessions--;
    sfBase->iDeletedUDPSessions++;
    return 0;
}

/*
**  NAME
**    ProcessBaseStats
**
**  DESCRIPTION
**    Main function to process Base Stats.
**
**  FORMAL INPUTS
**    SFBASE * - ptr to update.
**
**  FORMAL OUTPUTS
**    int - 0 is successful
*/
int ProcessBaseStats(SFBASE *sfBase, int console, int file, FILE * fh)
{
    SFBASE_STATS sfBaseStats;

    if( console || file )
    {
        if(CalculateBasePerfStats(sfBase, &sfBaseStats))
            return -1;
    }


    if( console )
        DisplayBasePerfStatsConsole(&sfBaseStats, sfBase->iFlags);
    
    if( file )
        LogBasePerfStats(&sfBaseStats, fh );

    return 0;
}

int GetProcessingTime(SYSTIMES *Systimes, SFBASE *sfBase)
{
    int todRet = -1;
    struct timeval tvTime;
#ifdef LINUX_SMP

    if(sfProcessProcPidStats(&(sfBase->sfProcPidStats)))
        return -1;
    todRet = gettimeofday(&tvTime, NULL);
#else
    struct rusage  rusage;
    int rusageRet;
#ifndef WIN32
    rusageRet = getrusage(RUSAGE_SELF, &rusage);
#else
    rusageRet = -1;
#endif  /* !WIN32 */
    todRet = gettimeofday(&tvTime, NULL);

    if (rusageRet < 0)
    {
        rusage.ru_utime.tv_sec = 0;
        rusage.ru_utime.tv_usec = 0;
        rusage.ru_stime.tv_sec = 0;
        rusage.ru_stime.tv_usec = 0;
    }
    Systimes->usertime   = ((double)rusage.ru_utime.tv_sec +
                           ((double)rusage.ru_utime.tv_usec * 1.0e-6)) -
                           sfBase->usertime_sec;
    Systimes->systemtime = ((double)rusage.ru_stime.tv_sec +
                           ((double)rusage.ru_stime.tv_usec * 1.0e-6)) -
                           sfBase->systemtime_sec;
    Systimes->totaltime  = Systimes->usertime + Systimes->systemtime;
#endif  /* LINUX_SMP */

    if (todRet < 0)
    {
        return todRet;
    }

    Systimes->realtime =  ((double)tvTime.tv_sec + 
                          ((double)tvTime.tv_usec * 1.0e-6)) -
                          sfBase->realtime_sec;
    return 0;
}

int GetEventsPerSecond(SFBASE *sfBase, SFBASE_STATS *sfBaseStats, 
                       SYSTIMES *Systimes)
{
    sfBaseStats->alerts_per_second = 
        (double)(pc.alert_pkts - sfBase->iAlerts) / Systimes->realtime;

    sfBase->iAlerts = pc.alert_pkts;

    sfBaseStats->total_sessions = sfBase->iTotalSessions;
    sfBaseStats->max_sessions = sfBase->iMaxSessions;

    sfBaseStats->syns_per_second = 
        (double)(sfBase->iSyns) / Systimes->realtime;

    sfBaseStats->synacks_per_second = 
        (double)(sfBase->iSynAcks) / Systimes->realtime;

    sfBaseStats->deleted_sessions_per_second = 
        (double)(sfBase->iDeletedSessions) / Systimes->realtime;

    sfBaseStats->new_sessions_per_second = 
        (double)(sfBase->iNewSessions) / Systimes->realtime;

    sfBaseStats->tcp_sessions_midstream_per_second = 
        (double)(sfBase->iMidStreamSessions) / Systimes->realtime;

    sfBaseStats->tcp_sessions_closed_per_second = 
        (double)(sfBase->iClosedSessions) / Systimes->realtime;

    sfBaseStats->tcp_sessions_timedout_per_second = 
        (double)(sfBase->iStreamTimeouts) / Systimes->realtime;

    sfBaseStats->tcp_sessions_pruned_per_second = 
        (double)(sfBase->iPrunedSessions) / Systimes->realtime;

    sfBaseStats->tcp_sessions_dropped_async_per_second = 
        (double)(sfBase->iDroppedAsyncSessions) / Systimes->realtime;

    sfBaseStats->max_tcp_sessions_interval = sfBase->iMaxSessionsInterval;

    sfBaseStats->stream_flushes_per_second = 
        (double)sfBase->iStreamFlushes / Systimes->realtime;

    sfBaseStats->stream_faults = sfBase->iStreamFaults;
    sfBaseStats->stream_timeouts = sfBase->iStreamTimeouts;
    sfBaseStats->curr_tcp_sessions_initializing = sfBase->iSessionsInitializing;
    sfBaseStats->curr_tcp_sessions_established = sfBase->iSessionsEstablished;
    sfBaseStats->curr_tcp_sessions_closing = sfBase->iSessionsClosing;
    
    sfBaseStats->frag_creates_per_second = 
        (double)sfBase->iFragCreates / Systimes->realtime;
    
    sfBaseStats->frag_completes_per_second = 
        (double)sfBase->iFragCompletes / Systimes->realtime;
    
    sfBaseStats->frag_inserts_per_second = 
        (double)sfBase->iFragInserts / Systimes->realtime;
    
    sfBaseStats->frag_deletes_per_second = 
        (double)sfBase->iFragDeletes / Systimes->realtime;
    
    sfBaseStats->frag_autofrees_per_second = 
        (double)sfBase->iFragAutoFrees / Systimes->realtime;
    
    sfBaseStats->frag_flushes_per_second = 
        (double)sfBase->iFragFlushes / Systimes->realtime;

    sfBaseStats->max_frags = sfBase->iMaxFrags;
    sfBaseStats->current_frags = sfBase->iCurrentFrags;
    sfBaseStats->frag_timeouts = sfBase->iFragTimeouts;
    sfBaseStats->frag_faults = sfBase->iFragFaults;

    sfBase->iSyns = 0;
    sfBase->iSynAcks = 0;
    sfBase->iNewSessions = 0;
    sfBase->iDeletedSessions = 0;

    sfBase->iStreamFlushes = 0;
    sfBase->iStreamFaults = 0;
    sfBase->iStreamTimeouts = 0;
    
    sfBase->iFragCreates = 0;
    sfBase->iFragCompletes = 0;
    sfBase->iFragInserts = 0;
    sfBase->iFragDeletes = 0;
    sfBase->iFragAutoFrees = 0;
    sfBase->iFragFlushes = 0;
    sfBase->iFragTimeouts = 0;
    sfBase->iFragFaults = 0;

    sfBaseStats->total_udp_sessions = sfBase->iTotalUDPSessions;
    sfBaseStats->max_udp_sessions = sfBase->iMaxUDPSessions;
    sfBaseStats->deleted_udp_sessions_per_second = 
        (double)(sfBase->iDeletedUDPSessions) / Systimes->realtime;

    sfBaseStats->new_udp_sessions_per_second = 
        (double)(sfBase->iNewUDPSessions) / Systimes->realtime;

    sfBase->iNewUDPSessions = 0;
    sfBase->iDeletedUDPSessions = 0;
    
    sfBase->iMaxSessionsInterval = sfBase->iTotalSessions;
    sfBase->iMidStreamSessions = 0;
    sfBase->iClosedSessions = 0;
    sfBase->iPrunedSessions = 0;
    sfBase->iDroppedAsyncSessions = 0;

    return 0;
}
    
int GetPacketsPerSecond(SFBASE *sfBase, SFBASE_STATS *sfBaseStats,
                        SYSTIMES *Systimes)
{
    sfBaseStats->kpackets_per_sec.realtime   = 
        (double)((double)sfBase->total_packets / 1000) / Systimes->realtime;

    if(sfBase->iFlags & MAX_PERF_STATS)
    {
        sfBaseStats->kpackets_per_sec.usertime   = 
            (double)((double)sfBase->total_packets / 1000) / 
            Systimes->usertime;
        sfBaseStats->kpackets_per_sec.systemtime = 
            (double)((double)sfBase->total_packets / 1000) / 
            Systimes->systemtime;
        sfBaseStats->kpackets_per_sec.totaltime  = 
            (double)((double)sfBase->total_packets / 1000) / 
            Systimes->totaltime;
    }

    sfBaseStats->kpackets_wire_per_sec.realtime   = 
        (double)((double)sfBase->total_wire_packets / 1000) / Systimes->realtime;

    if(sfBase->iFlags & MAX_PERF_STATS)
    {
        sfBaseStats->kpackets_wire_per_sec.usertime   = 
            (double)((double)sfBase->total_wire_packets / 1000) / 
            Systimes->usertime;
        sfBaseStats->kpackets_wire_per_sec.systemtime = 
            (double)((double)sfBase->total_wire_packets / 1000) / 
            Systimes->systemtime;
        sfBaseStats->kpackets_wire_per_sec.totaltime  = 
            (double)((double)sfBase->total_wire_packets / 1000) / 
            Systimes->totaltime;
    }

    sfBaseStats->kpackets_ipfrag_per_sec.realtime   = 
        (double)((double)sfBase->total_ipfragmented_packets / 1000) / Systimes->realtime;

    if(sfBase->iFlags & MAX_PERF_STATS)
    {
        sfBaseStats->kpackets_ipfrag_per_sec.usertime   = 
            (double)((double)sfBase->total_ipfragmented_packets / 1000) / 
            Systimes->usertime;
        sfBaseStats->kpackets_ipfrag_per_sec.systemtime = 
            (double)((double)sfBase->total_ipfragmented_packets / 1000) / 
            Systimes->systemtime;
        sfBaseStats->kpackets_ipfrag_per_sec.totaltime  = 
            (double)((double)sfBase->total_ipfragmented_packets / 1000) / 
            Systimes->totaltime;
    }

    sfBaseStats->kpackets_ipreass_per_sec.realtime   = 
        (double)((double)sfBase->total_ipreassembled_packets / 1000) / Systimes->realtime;

    if(sfBase->iFlags & MAX_PERF_STATS)
    {
        sfBaseStats->kpackets_ipreass_per_sec.usertime   = 
            (double)((double)sfBase->total_ipreassembled_packets / 1000) / 
            Systimes->usertime;
        sfBaseStats->kpackets_ipreass_per_sec.systemtime = 
            (double)((double)sfBase->total_ipreassembled_packets / 1000) / 
            Systimes->systemtime;
        sfBaseStats->kpackets_ipreass_per_sec.totaltime  = 
            (double)((double)sfBase->total_ipreassembled_packets / 1000) / 
            Systimes->totaltime;
    }

    sfBaseStats->kpackets_rebuilt_per_sec.realtime   = 
        (double)((double)sfBase->total_rebuilt_packets / 1000) / Systimes->realtime;

    if(sfBase->iFlags & MAX_PERF_STATS)
    {
        sfBaseStats->kpackets_rebuilt_per_sec.usertime   = 
            (double)((double)sfBase->total_rebuilt_packets / 1000) / 
            Systimes->usertime;
        sfBaseStats->kpackets_rebuilt_per_sec.systemtime = 
            (double)((double)sfBase->total_rebuilt_packets / 1000) / 
            Systimes->systemtime;
        sfBaseStats->kpackets_rebuilt_per_sec.totaltime  = 
            (double)((double)sfBase->total_rebuilt_packets / 1000) / 
            Systimes->totaltime;
    }

    
    return 0;
}

int GetuSecondsPerPacket(SFBASE *sfBase, SFBASE_STATS *sfBaseStats, 
                         SYSTIMES *Systimes)
{
    sfBaseStats->usecs_per_packet.usertime   = (Systimes->usertime * 1.0e6) /
                                               (double)sfBase->total_packets;
    sfBaseStats->usecs_per_packet.systemtime = (Systimes->systemtime * 1.0e6) /
                                               (double)sfBase->total_packets;
    sfBaseStats->usecs_per_packet.totaltime  = (Systimes->totaltime * 1.0e6) /
                                               (double)sfBase->total_packets;
    sfBaseStats->usecs_per_packet.realtime   = (Systimes->realtime * 1.0e6) /
                                               (double)sfBase->total_packets;

    return 0;
}

int GetMbitsPerSecond(SFBASE *sfBase, SFBASE_STATS *sfBaseStats, 
                      SYSTIMES *Systimes)
{
    /*
    **  These Mbits stats are for the Snort Maximum Performance stats
    **  that can't reliably be gotten from Linux SMP kernels.  So
    **  we don't do them.
    */
    if(sfBase->iFlags & MAX_PERF_STATS)
    {
        sfBaseStats->mbits_per_sec.usertime   = ((double)
                                                (sfBase->total_bytes<<3) *
                                                1.0e-6) /
                                                Systimes->usertime;
        sfBaseStats->mbits_per_sec.systemtime = ((double)
                                                (sfBase->total_bytes<<3) *
                                                1.0e-6) /
                                                Systimes->systemtime;
        sfBaseStats->mbits_per_sec.totaltime  = ((double)
                                                (sfBase->total_bytes<<3) *
                                                1.0e-6) /
                                                Systimes->totaltime;
    }

    sfBaseStats->mbits_per_sec.realtime   = ((double)(sfBase->total_bytes<<3) *
                                             1.0e-6) /
                                            Systimes->realtime;
    sfBaseStats->wire_mbits_per_sec.realtime   = 
                                    ((double)(sfBase->total_wire_bytes<<3) *
                                    1.0e-6) /
                                    Systimes->realtime;
    sfBaseStats->rebuilt_mbits_per_sec.realtime   = 
                                    ((double)(sfBase->total_rebuilt_bytes<<3) *
                                    1.0e-6) /
                                    Systimes->realtime;

    sfBaseStats->ipfrag_mbits_per_sec.realtime   = 
                                    ((double)(sfBase->total_ipfragmented_bytes<<3) *
                                    1.0e-6) /
                                    Systimes->realtime;

    sfBaseStats->ipreass_mbits_per_sec.realtime   = 
                                    ((double)(sfBase->total_ipreassembled_bytes<<3) *
                                    1.0e-6) /
                                    Systimes->realtime;

    return 0;
}

int GetCPUTime(SFBASE *sfBase, SFBASE_STATS *sfBaseStats, SYSTIMES *Systimes)
{
#ifndef LINUX_SMP
    unsigned char needToNormalize = 0;
    sfBaseStats->user_cpu_time   = (Systimes->usertime   / 
                                   Systimes->realtime) * 100;
    sfBaseStats->system_cpu_time = (Systimes->systemtime / 
                                   Systimes->realtime) * 100;
    sfBaseStats->idle_cpu_time   = ((Systimes->realtime -
                                     Systimes->totaltime) /
                                     Systimes->realtime) * 100;

    /* percentages can be < 0 because of a small variance between
     * when the snapshot is taken of the CPU times and snapshot of
     * the real time.  So these are just a safe-guard to normalize
     * the data so we see positive values.
     */
    if (sfBaseStats->user_cpu_time < 0)
    {
        sfBaseStats->user_cpu_time = 0;
        needToNormalize = 1;
    }
    if (sfBaseStats->system_cpu_time < 0)
    {
        sfBaseStats->system_cpu_time = 0;
        needToNormalize = 1;
    }
    if (sfBaseStats->idle_cpu_time < 0)
    {
        sfBaseStats->idle_cpu_time = 0;
        needToNormalize = 1;
    }

    if (needToNormalize)
    {
        double totalPercent = sfBaseStats->user_cpu_time +
                              sfBaseStats->system_cpu_time +
                              sfBaseStats->idle_cpu_time;


        sfBaseStats->user_cpu_time = (sfBaseStats->user_cpu_time /
                                      totalPercent) * 100;
        sfBaseStats->system_cpu_time = ( sfBaseStats->system_cpu_time /
                                      totalPercent) * 100;
        sfBaseStats->idle_cpu_time = ( sfBaseStats->idle_cpu_time /
                                      totalPercent) * 100;

    }
#endif
    return 0;
}


/*
**  NAME
**    CalculateBasePerfStats
**
**  DESCRIPTION
**    This is the main function that calculates the stats. Stats 
**    that we caculate are:
**      *uSecs per Packet
**      *Packets per Second
**      *Mbits per Second
**      *Average bytes per Packet
**      *CPU Time
**      *Dropped Packets
**    These statistics are processed and then stored in the
**    SFBASE_STATS structure.  This allows output functions to
**    be easily formed and inserted.
**    NOTE: We can break up these statistics into functions for easier
**    reading.
**
**  FORMAL INPUTS
**    SFBASE *       - ptr to performance struct
**    SFBASE_STATS * - ptr to struct to fill in performance stats
**
**  FORMAL OUTPUTS
**    int - 0 is successful
*/
int CalculateBasePerfStats(SFBASE *sfBase, SFBASE_STATS *sfBaseStats)
{
	return 0;
    SYSTIMES       Systimes;
    time_t   clock;

#ifdef LINUX_SMP
    
    /*
    **  We also give sfBaseStats access to the CPU usage
    **  contained in sfProcPidStats.  This way we don't need
    **  to complicate sfBaseStats further.
    */
    sfBaseStats->sfProcPidStats = &(sfBase->sfProcPidStats);

#endif 
    if(GetProcessingTime(&Systimes, sfBase))
        return -1;

    sfBaseStats->total_blocked_packets = sfBase->total_blocked_packets;

    /*
    **  Avg. bytes per Packet
    */
    if (sfBase->total_packets > 0)
        sfBaseStats->avg_bytes_per_packet =
                (int)((double)(sfBase->total_bytes) /
                (double)(sfBase->total_packets));
    else
        sfBaseStats->avg_bytes_per_packet = 0;

    if (sfBase->total_wire_packets > 0)
        sfBaseStats->avg_bytes_per_wire_packet =
                (int)((double)(sfBase->total_wire_bytes) /
                (double)(sfBase->total_wire_packets));
    else
        sfBaseStats->avg_bytes_per_wire_packet = 0;

    if (sfBase->total_ipfragmented_packets > 0)
        sfBaseStats->avg_bytes_per_ipfrag_packet =
                (int)((double)(sfBase->total_ipfragmented_bytes) /
                (double)(sfBase->total_ipfragmented_packets));
    else
        sfBaseStats->avg_bytes_per_ipfrag_packet = 0;

    if (sfBase->total_ipreassembled_packets > 0)
        sfBaseStats->avg_bytes_per_ipreass_packet =
                (int)((double)(sfBase->total_ipreassembled_bytes) /
                (double)(sfBase->total_ipreassembled_packets));
    else
        sfBaseStats->avg_bytes_per_ipreass_packet = 0;

    if (sfBase->total_rebuilt_packets > 0)
        sfBaseStats->avg_bytes_per_rebuilt_packet =
                (int)((double)(sfBase->total_rebuilt_bytes) /
                (double)(sfBase->total_rebuilt_packets));
    else
        sfBaseStats->avg_bytes_per_rebuilt_packet = 0;

    /*
    **  CPU time
    */
    GetCPUTime(sfBase, sfBaseStats, &Systimes);

    /*
    **  Get Dropped Packets
    */
    GetPktDropStats(sfBase, sfBaseStats);

    /*
    **  Total packets
    */
    sfBaseStats->total_packets = sfBase->total_wire_packets;

    /*
    *   Pattern Matching Performance in Real and User time
    */
    //sfBaseStats->patmatch_percent = 100.0 * mpseGetPatByteCount() / sfBase->total_wire_bytes; // wxh
    //mpseResetByteCount();

    if(sfBase->iFlags & MAX_PERF_STATS)
    {
        /*
        **  uSeconds per Packet
        **  user, system, total time
        */
        GetuSecondsPerPacket(sfBase, sfBaseStats, &Systimes);
    }

    /*
    **  Mbits per sec
    **  user, system, total time
    */
    GetMbitsPerSecond(sfBase, sfBaseStats, &Systimes);

    /*
    **  EventsPerSecond
    **  We get the information from the global variable
    **  PacketCount.
    */
    GetEventsPerSecond(sfBase, sfBaseStats, &Systimes);

    /*
    **  Packets per seconds
    **  user, system, total time
    */
    GetPacketsPerSecond(sfBase, sfBaseStats, &Systimes);

    /*
    ** Attribute Table counters
    **
    */
    sfBaseStats->current_attribute_hosts = sfBase->iAttributeHosts;
    sfBaseStats->attribute_table_reloads = sfBase->iAttributeReloads;
    
    /*
    **  Set the date string for print out
    */
    time(&clock);
    sfBaseStats->time = clock;

    return 0;
}

/*
**  NAME
**    GetPktDropStats
**
**  DESCRIPTION
**    Gets the packet drop statisitics from OS.
**    NOTE:  Currently only pcap-based sniffing is supported.  Should
**    add native OS calls.
**
**  FORMAL INPUT
**    SFBASE *       - ptr to struct
**    SFBASE_STATS * - ptr to struct to fill in with perf stats
**
**  FORMAL OUTPUT
**    int - 0 is successful
*/
int GetPktDropStats(SFBASE *sfBase, SFBASE_STATS *sfBaseStats)
{
#ifndef PCAP_CLOSE
    /*
    **  Network Interfaces.  Right now we only check
    **  the first interface
    */
    extern pcap_t *pd;
    
    if((!pd)
#ifdef WIN32
        || (pv.readmode_flag)
#endif
        )
    {
        if (sfBase->iReset == 1)
        {
            sfBaseStats->pkt_stats.pkts_recv = sfBase->total_wire_packets;
        }
        else
        {
            sfBaseStats->pkt_stats.pkts_recv += sfBase->total_wire_packets;
        }
        sfBaseStats->pkt_stats.pkts_drop = 0;
        sfBaseStats->pkt_drop_percent    = 0.0;
        return 0;
    }

    if (UpdatePcapPktStats() == -1)
#else
    //if (UpdatePcapPktStats(0) == -1) //wxh
    if (0)
#endif
    {
        if (sfBase->iReset == 1)
        {
            sfBaseStats->pkt_stats.pkts_recv = sfBase->total_wire_packets;
        }
        else
        {
            sfBaseStats->pkt_stats.pkts_recv += sfBase->total_wire_packets;
        }

        sfBaseStats->pkt_stats.pkts_drop = 0;
        sfBaseStats->pkt_drop_percent    = 0.0;
    }
    else
    {
        UINT64 recv, drop;

        recv = GetPcapPktStatsRecv();
        drop = GetPcapPktStatsDrop();

        if( sfBase->iReset == 1 )
        {
            sfBaseStats->pkt_stats.pkts_recv = recv - sfBase->pkt_stats.pkts_recv;

            sfBaseStats->pkt_stats.pkts_drop = drop - sfBase->pkt_stats.pkts_drop;
        }
        else
        {
            sfBaseStats->pkt_stats.pkts_recv = recv;
            sfBaseStats->pkt_stats.pkts_drop = drop;
        }
        
        sfBaseStats->pkt_drop_percent =
            ((double)sfBaseStats->pkt_stats.pkts_drop /
             (double)sfBaseStats->pkt_stats.pkts_recv) * 100;
        
        /*
        **  Reset sfBase stats for next go round.
        */
        sfBase->pkt_stats.pkts_recv = recv;
        sfBase->pkt_stats.pkts_drop = drop;
    }
    
    return 0;
}

/*
 *   
 *   Log Base Per Stats to File for Use by the MC 
 *
 * unixtime(in secs since epoch)
 * %pkts dropped
 * mbits/sec (wire)
 * alerts/sec
 * K-Packets/Sec (wire)
 * Avg Bytes/Pkt  (wire)
 * %bytes pattern matched 
 * syns/sec
 * synacks/sec
 * new-sessions/sec (tcp stream cache)
 * del-sessions/sec (tcp stream cache)
 * total-sessions open (tcp stream cache)
 * max-sessions, lifetime (tcp stream cache)
 * streamflushes/sec
 * streamfaults/sec
 * streamtimeouts
 * fragcreates/sec
 * fragcompletes/sec
 * fraginserts/sec
 * fragdeletes/sec
 * fragflushes/sec
 * current-frags open (frag cache)
 * max-frags (frag cache)
 * fragtimeouts
 * fragfaults
 * num cpus (following triple is repeated for each CPU)
 * %user-cpu usage
 * %sys-cpu usage
 * %idle-cpu usage
 * mbits/sec (wire)
 * mbits/sec (ip fragmented)
 * mbits/sec (ip reassembled)
 * mbits/sec (tcp stream rebuilt)
 * mbits/sec (app layer)
 * Avg Bytes/Pkt  (wire)
 * Avg Bytes/Pkt  (ip fragmented)
 * Avg Bytes/Pkt  (ip reassembled)
 * Avg Bytes/Pkt  (tcp stream rebuilt)
 * Avg Bytes/Pkt  (app layer)
 * K-Packets/Sec (wire)
 * K-Packets/Sec (ip fragmented)
 * K-Packets/Sec (ip reassembled)
 * K-Packets/Sec (tcp stream rebuilt)
 * K-Packets/Sec (app layer)
 * Pkts recieved
 * Pkts dropped
 * Blocked-KPackets  (wire)
 * udp-sessions
 * max-udp-sessions
 * del-udp-sessions/sec (udp stream cache)
 * new-udp-sessions/sec (udp stream cache)
 * max-sessions, interval (tcp stream cache)
 * curr-tcp-sessions-initializing (tcp stream cache, of total-sessions open)
 * curr-tcp-sessions-established (tcp stream cache, of total-sessions open)
 * curr-tcp-sessions-closing (tcp stream cache, of total-sessions open)
 * tcp-sessions-mistream/sec (tcp stream cache, of new-sessions/sec)
 * tcp-sessions-closed/sec (tcp stream cache, of del-sessions/sec)
 * tcp-sessions-timedout/sec (tcp stream cache, of del-sessions/sec)
 * tcp-sessions-pruned/sec (tcp stream cache, of del-sessions/sec)
 * tcp-sessions-dropped_async/sec (tcp stream cache, of del-sessions/sec)
 * hosts in attribute table
 * attribute table reloads
 *
 */

int LogBasePerfStats(SFBASE_STATS *sfBaseStats,  FILE * fh )
{
    double sys=0.0,usr=0.0,idle=0.0;

#ifdef LINUX_SMP
    int iCtr;
#endif 
    if( ! fh ) return 0;
 
    fprintf(fh,"%lu,%.3f,%.3f,%.3f,%.3f,%d,%.3f,",
                (unsigned long)sfBaseStats->time,
                sfBaseStats->pkt_drop_percent,
                sfBaseStats->wire_mbits_per_sec.realtime,
                sfBaseStats->alerts_per_second,
                sfBaseStats->kpackets_per_sec.realtime,
                sfBaseStats->avg_bytes_per_packet,
                sfBaseStats->patmatch_percent);
       
    /* Session estimation statistics */

    fprintf(fh, "%.3f,%.3f,%.3f,%.3f," CSVu64 CSVu64,
            sfBaseStats->syns_per_second,
            sfBaseStats->synacks_per_second,
            sfBaseStats->new_sessions_per_second,
            sfBaseStats->deleted_sessions_per_second,
            sfBaseStats->total_sessions,
            sfBaseStats->max_sessions);


    fprintf(fh, "%.3f," CSVu64 CSVu64,
            sfBaseStats->stream_flushes_per_second,
            sfBaseStats->stream_faults,
            sfBaseStats->stream_timeouts);

    fprintf(fh, "%.3f,%.3f,%.3f,%.3f,%.3f,%.3f," CSVu64 CSVu64 CSVu64 CSVu64,
            sfBaseStats->frag_creates_per_second,
            sfBaseStats->frag_completes_per_second,
            sfBaseStats->frag_inserts_per_second,
            sfBaseStats->frag_deletes_per_second,
            sfBaseStats->frag_autofrees_per_second,
            sfBaseStats->frag_flushes_per_second,
            sfBaseStats->current_frags,
            sfBaseStats->max_frags,
            sfBaseStats->frag_timeouts,
            sfBaseStats->frag_faults);
   
    /* CPU STATS - at the end of output record */ 
#ifdef LINUX_SMP
    /* First the number of CPUs */
    fprintf(fh, "%d,", sfBaseStats->sfProcPidStats->iCPUs); 

    /* Next, stats for each CPU (a triple) */
    for(iCtr = 0; iCtr < sfBaseStats->sfProcPidStats->iCPUs; iCtr++)
    {
        usr= sfBaseStats->sfProcPidStats->SysCPUs[iCtr].user;
        sys= sfBaseStats->sfProcPidStats->SysCPUs[iCtr].sys;
        idle= sfBaseStats->sfProcPidStats->SysCPUs[iCtr].idle;
    
        fprintf(fh,"%.3f,%.3f,%.3f,",usr,sys,idle);
    }

#else

    usr=sfBaseStats->user_cpu_time;
    sys=sfBaseStats->system_cpu_time;
    idle=sfBaseStats->idle_cpu_time;
  
    /* 1 CPU hardcoded */ 
    fprintf(fh,"1,%.3f,%.3f,%.3f,",usr,sys,idle);

#endif

    /* Status for MBits/s, Bytes/Pkt, KPkts/s for each of
     * wire, IP Fragmented, IP Reassembled, Stream Reassembled,
     * App Layer (data that reaches protocol decoders). */
    fprintf(fh,"%.3f,%.3f,%.3f,%.3f,%.3f,",
            sfBaseStats->wire_mbits_per_sec.realtime,
            sfBaseStats->ipfrag_mbits_per_sec.realtime,
            sfBaseStats->ipreass_mbits_per_sec.realtime,
            sfBaseStats->rebuilt_mbits_per_sec.realtime,
            sfBaseStats->mbits_per_sec.realtime);
            
    fprintf(fh,"%d,%d,%d,%d,%d,",
        sfBaseStats->avg_bytes_per_wire_packet,
        sfBaseStats->avg_bytes_per_ipfrag_packet,
        sfBaseStats->avg_bytes_per_ipreass_packet,
        sfBaseStats->avg_bytes_per_rebuilt_packet,
        sfBaseStats->avg_bytes_per_packet);
        
    fprintf(fh,"%.3f,%.3f,%.3f,%.3f,%.3f,",
        sfBaseStats->kpackets_wire_per_sec.realtime,
        sfBaseStats->kpackets_ipfrag_per_sec.realtime,
        sfBaseStats->kpackets_ipreass_per_sec.realtime,
        sfBaseStats->kpackets_rebuilt_per_sec.realtime,
        sfBaseStats->kpackets_per_sec.realtime);
    
    fprintf(fh, CSVu64,sfBaseStats->pkt_stats.pkts_recv);

    fprintf(fh, CSVu64, sfBaseStats->pkt_stats.pkts_drop);
    
    fprintf(fh, CSVu64, sfBaseStats->total_blocked_packets);

    fprintf(fh, "%.3f,%.3f," CSVu64 CSVu64,
            sfBaseStats->new_udp_sessions_per_second,
            sfBaseStats->deleted_udp_sessions_per_second,
            sfBaseStats->total_udp_sessions,
            sfBaseStats->max_udp_sessions);

    fprintf(fh, CSVu64 CSVu64 CSVu64 CSVu64 "%.3f,%.3f,%.3f,%.3f,%.3f,",
            sfBaseStats->max_tcp_sessions_interval,
            sfBaseStats->curr_tcp_sessions_initializing,
            sfBaseStats->curr_tcp_sessions_established,
            sfBaseStats->curr_tcp_sessions_closing,
            sfBaseStats->tcp_sessions_midstream_per_second,
            sfBaseStats->tcp_sessions_closed_per_second,
            sfBaseStats->tcp_sessions_timedout_per_second,
            sfBaseStats->tcp_sessions_pruned_per_second,
            sfBaseStats->tcp_sessions_dropped_async_per_second);

    fprintf(fh, CSVu64 CSVu64,
            sfBaseStats->current_attribute_hosts,
            sfBaseStats->attribute_table_reloads);

    fprintf(fh,"\n");
 
    fflush(fh);

#ifdef LINUX   
   //LogScheduler();
#endif
   
    return 0;
}


/*
**  NAME 
**    DisplayBasePerfStats
** 
**  DESCRIPTION
**    Output Function.  We can easily code multiple output buffers
**    because all that is received is a SFBASE_STATS struct which
**    holds all the information to output.  This current output
**    function just prints to stdout.
**
**  FORMAL INPUTS
**    SFBASE_STATS * - struct with perf information
**    int            - flags for output
**
**  FORMAL OUTPUTS
**    int - 0 is successful
*/
int DisplayBasePerfStatsConsole(SFBASE_STATS *sfBaseStats, int iFlags)
{
#ifdef LINUX_SMP
    int iCtr;
#endif

    LogMessage("\n\nSnort Realtime Performance  : %s--------------------------\n", 
               ctime(&sfBaseStats->time));

    LogMessage("Pkts Recv:   " STDu64 "\n", sfBaseStats->pkt_stats.pkts_recv);

    LogMessage("Pkts Drop:   " STDu64 "\n", sfBaseStats->pkt_stats.pkts_drop);

    LogMessage("%% Dropped:   %.3f%%\n", sfBaseStats->pkt_drop_percent);

    LogMessage("Blocked:     " STDu64 "\n\n", sfBaseStats->total_blocked_packets);

    LogMessage("Mbits/Sec:   %.3f (wire)\n", 
            sfBaseStats->wire_mbits_per_sec.realtime);
    LogMessage("Mbits/Sec:   %.3f (ip fragmented)\n",    
            sfBaseStats->ipfrag_mbits_per_sec.realtime);
    LogMessage("Mbits/Sec:   %.3f (ip reassembled)\n",    
            sfBaseStats->ipreass_mbits_per_sec.realtime);
    LogMessage("Mbits/Sec:   %.3f (tcp rebuilt)\n", 
            sfBaseStats->rebuilt_mbits_per_sec.realtime);
    LogMessage("Mbits/Sec:   %.3f (app layer)\n\n",    
            sfBaseStats->mbits_per_sec.realtime);

    LogMessage("Bytes/Pkt:   %d (wire)\n",
        sfBaseStats->avg_bytes_per_wire_packet);
    LogMessage("Bytes/Pkt:   %d (ip fragmented)\n",
        sfBaseStats->avg_bytes_per_ipfrag_packet);
    LogMessage("Bytes/Pkt:   %d (ip reassembled)\n",
        sfBaseStats->avg_bytes_per_ipreass_packet);
    LogMessage("Bytes/Pkt:   %d (tcp rebuilt)\n",
        sfBaseStats->avg_bytes_per_rebuilt_packet);
    LogMessage("Bytes/Pkt:   %d (app layer)\n\n",
        sfBaseStats->avg_bytes_per_packet);

    LogMessage("KPkts/Sec:   %.3f (wire)\n",
        sfBaseStats->kpackets_wire_per_sec.realtime);
    LogMessage("KPkts/Sec:   %.3f (ip fragmented)\n",
        sfBaseStats->kpackets_ipfrag_per_sec.realtime);
    LogMessage("KPkts/Sec:   %.3f (ip reassembled)\n",
        sfBaseStats->kpackets_ipreass_per_sec.realtime);
    LogMessage("KPkts/Sec:   %.3f (tcp rebuilt)\n",
        sfBaseStats->kpackets_rebuilt_per_sec.realtime);
    LogMessage("KPkts/Sec:   %.3f (app layer)\n\n",
        sfBaseStats->kpackets_per_sec.realtime);

    LogMessage("PatMatch:    %.3f%%\n\n",  sfBaseStats->patmatch_percent);

    /*
    **  The following ifdefs are for CPU stats dealing with multiple
    **  CPUs in Linux.  Snort will show user, system and idle time for
    **  each CPU.  The methods of calculating this are different though,
    **  since getrusage is broken for multiple CPUs in Linux.  We get the
    **  CPU stats instead from the proc filesystem on Linux.
    */
#ifdef LINUX_SMP

    for(iCtr = 0; iCtr < sfBaseStats->sfProcPidStats->iCPUs; iCtr++)
    {
    LogMessage("CPU%d Usage:  %.3f%% (user)  %.3f%% (sys)  %.3f%% (idle)\n", 
                iCtr,
                sfBaseStats->sfProcPidStats->SysCPUs[iCtr].user,
                sfBaseStats->sfProcPidStats->SysCPUs[iCtr].sys,
                sfBaseStats->sfProcPidStats->SysCPUs[iCtr].idle);
    }
    printf("\n");

#else

    LogMessage("CPU Usage:   %.3f%% (user)  %.3f%% (sys)  %.3f%% (idle)\n\n", 
                sfBaseStats->user_cpu_time,
                sfBaseStats->system_cpu_time,
                sfBaseStats->idle_cpu_time);

#endif

    /*
    **  Shows the number of snort alerts per second.
    */
    LogMessage("Alerts/Sec             :  %.3f\n",   sfBaseStats->alerts_per_second);

    /* Session estimation statistics */
    LogMessage("Syns/Sec               :  %.3f\n", sfBaseStats->syns_per_second);
    LogMessage("Syn-Acks/Sec           :  %.3f\n", sfBaseStats->synacks_per_second);
    LogMessage("New Cached Sessions/Sec:  %.3f\n", sfBaseStats->new_sessions_per_second);
    LogMessage("Midstream Sessions/Sec :  %.3f\n", sfBaseStats->tcp_sessions_midstream_per_second);
    LogMessage("Cached Sessions Del/Sec:  %.3f\n", sfBaseStats->deleted_sessions_per_second);    
    LogMessage("Closed Sessions/Sec    :  %.3f\n", sfBaseStats->tcp_sessions_closed_per_second);
    LogMessage("TimedOut Sessions/Sec  :  %.3f\n", sfBaseStats->tcp_sessions_timedout_per_second);
    LogMessage("Pruned Sessions/Sec    :  %.3f\n", sfBaseStats->tcp_sessions_pruned_per_second);
    LogMessage("Dropped Async Ssns/Sec :  %.3f\n", sfBaseStats->tcp_sessions_dropped_async_per_second);

    LogMessage("Current Cached Sessions:  " STDu64 "\n", sfBaseStats->total_sessions);
    LogMessage("Sessions Initializing  :  " STDu64 "\n", sfBaseStats->curr_tcp_sessions_initializing);
    LogMessage("Sessions Established   :  " STDu64 "\n", sfBaseStats->curr_tcp_sessions_established);
    LogMessage("Sessions Closing       :  " STDu64 "\n", sfBaseStats->curr_tcp_sessions_closing);
    LogMessage("Max Cached Sessions    :  " STDu64 "\n", sfBaseStats->max_sessions);
    LogMessage("Max Sessions (interval):  " STDu64 "\n", sfBaseStats->max_tcp_sessions_interval);

    /* more instrumentation for stream4/frag2 */
    LogMessage("Stream Flushes/Sec     :  %.3f\n", sfBaseStats->stream_flushes_per_second);
    LogMessage("Stream Cache Faults/Sec:  " STDu64 "\n", sfBaseStats->stream_faults);
    LogMessage("Stream Cache Timeouts  :  " STDu64 "\n", sfBaseStats->stream_timeouts);

    LogMessage("Frag Creates()s/Sec    :  %.3f\n", sfBaseStats->frag_creates_per_second);
    LogMessage("Frag Completes()s/Sec  :  %.3f\n", sfBaseStats->frag_completes_per_second);
    LogMessage("Frag Inserts()s/Sec    :  %.3f\n", sfBaseStats->frag_inserts_per_second);
    LogMessage("Frag Deletes/Sec       :  %.3f\n", sfBaseStats->frag_deletes_per_second);
    LogMessage("Frag AutoFrees/Sec     :  %.3f\n", sfBaseStats->frag_autofrees_per_second);
    LogMessage("Frag Flushes/Sec       :  %.3f\n", sfBaseStats->frag_flushes_per_second);

    LogMessage("Current Cached Frags   :  " STDu64 "\n", sfBaseStats->current_frags);
    LogMessage("Max Cached Frags       :  " STDu64 "\n", sfBaseStats->max_frags);
    LogMessage("Frag Timeouts          :  " STDu64 "\n", sfBaseStats->frag_timeouts);
    LogMessage("Frag Faults            :  " STDu64 "\n\n", sfBaseStats->frag_faults);

    LogMessage("New Cached UDP Ssns/Sec:  %.3f\n", sfBaseStats->new_udp_sessions_per_second);
    LogMessage("Cached UDP Ssns Del/Sec:  %.3f\n", sfBaseStats->deleted_udp_sessions_per_second);    

    LogMessage("Current Cached UDP Ssns:  " STDu64 "\n", sfBaseStats->total_udp_sessions);
    LogMessage("Max Cached UDP Ssns    :  " STDu64 "\n\n", sfBaseStats->max_udp_sessions);

#ifdef TARGET_BASED
    LogMessage("Attribute Table Hosts  :  " STDu64 "\n", sfBaseStats->current_attribute_hosts);
    LogMessage("Attribute Table Reloads:  " STDu64 "\n\n", sfBaseStats->attribute_table_reloads);
#endif

    /*
    **  Snort Maximum Performance Statistics
    **  These statistics calculate the maximum performance that 
    **  snort could attain by using the getrusage numbers.  We've
    **  seen in testing that these numbers come close to the actual
    **  throughput for Mbits/Sec and Pkt/Sec.  But note that these
    **  are not hard numbers and rigorous testing is necessary to
    **  establish snort performance on any hardware setting.
    */
    if(iFlags & MAX_PERF_STATS)
    {
    
        LogMessage("Snort Maximum Performance\n");
        LogMessage("-------------------------\n\n");
    
        LogMessage("Mbits/Second\n");
        LogMessage("----------------\n");
        LogMessage("Snort:       %.3f\n",sfBaseStats->mbits_per_sec.usertime);
        LogMessage("Sniffing:    %.3f\n",sfBaseStats->mbits_per_sec.systemtime);
        LogMessage("Combined:    %.3f\n\n",sfBaseStats->mbits_per_sec.totaltime);
    

        LogMessage("uSeconds/Pkt\n");
        LogMessage("----------------\n");
        LogMessage("Snort:       %.3f\n",sfBaseStats->usecs_per_packet.usertime);
        LogMessage("Sniffing:    %.3f\n",sfBaseStats->usecs_per_packet.systemtime);
        LogMessage("Combined:    %.3f\n\n",sfBaseStats->usecs_per_packet.totaltime);

        LogMessage("KPkts/Second\n");
        LogMessage("------------------\n");
        LogMessage("Snort:       %.3f\n",sfBaseStats->kpackets_per_sec.usertime);
        LogMessage("Sniffing:    %.3f\n",sfBaseStats->kpackets_per_sec.systemtime);
        LogMessage("Combined:    %.3f\n\n",sfBaseStats->kpackets_per_sec.totaltime);
    }

    return 0;
}

