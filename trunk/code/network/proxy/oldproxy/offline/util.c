/* $Id$ */
/*
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


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>

#ifndef WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <dirent.h>
#include <fnmatch.h>
#endif /* !WIN32 */

#include <stdarg.h>
#include <syslog.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <math.h> /* For ceil */

#ifndef WIN32
#include <grp.h>
#include <pwd.h>
#include <netdb.h>
#include <limits.h>
#endif /* !WIN32 */

#include <fcntl.h>

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include "cap.h"
#include "debug.h"
#include "util.h"
//#include "inline.h"
//#include "build.h"
#include "plugbase.h"
#include "sf_types.h"

//#include "pcre.h"

//#include "mpse.h"

//#include "ppm.h"


#ifdef TARGET_BASED
#include "sftarget_reader.h"
#endif

#ifdef WIN32
#include "win32/WIN32-Code/name.h"
#endif

//#include "stream5_common.h"

#ifdef PATH_MAX
#define PATH_MAX_UTIL PATH_MAX
#else
#define PATH_MAX_UTIL 1024
#endif /* PATH_MAX */

char _PATH_VARRUN[1024];
extern long start_time;
extern PreprocessStatsList *PreprocessStats;
//extern Stream5Stats s5stats;
extern int datalink;

static PcapPktStats pkt_stats;
char *protocol_names[256]; //wxh


#ifdef NAME_MAX
#define NAME_MAX_UTIL NAME_MAX
#else
#define NAME_MAX_UTIL 256
#endif /* NAME_MAX */

#define FILE_MAX_UTIL  (PATH_MAX_UTIL + NAME_MAX_UTIL)


/*
 * Function: GenHomenet(char *)
 *
 * Purpose: Translate the command line character string into its equivalent
 *          32-bit network byte ordered value (with netmask)
 *
 * Arguments: netdata => The address/CIDR block
 *
 * Returns: void function
 */
void GenHomenet(char *netdata)
{
#ifdef SUP_IP6
    sfip_pton(netdata, &pv.homenet);
#else
    struct in_addr net;    /* place to stick the local network data */
    char **toks;           /* dbl ptr to store mSplit return data in */
    int num_toks;          /* number of tokens mSplit returns */
    int nmask;             /* temporary netmask storage */

    /* break out the CIDR notation from the IP address */
    toks = mSplit(netdata, "/", 2, &num_toks, 0);

    if(num_toks > 1)
    {
        /* convert the CIDR notation into a real live netmask */
        nmask = atoi(toks[1]);

        if((nmask > 0) && (nmask < 33))
        {
            pv.netmask = netmasks[nmask];
        }
        else
        {
            FatalError("Bad CIDR block [%s:%d], 1 to 32 please!\n",
                       toks[1], nmask);
        }
    }
    else
    {
        FatalError("No netmask specified for home network!\n");
    }

    pv.netmask = htonl(pv.netmask);

    DEBUG_WRAP(DebugMessage(DEBUG_INIT, "homenet netmask = %#8lX\n", pv.netmask););

    /* convert the IP addr into its 32-bit value */
    if((net.s_addr = inet_addr(toks[0])) == -1)
    {
        FatalError("Homenet (%s) didn't translate\n",
                   toks[0]);
    }
    else
    {
#ifdef DEBUG
        struct in_addr sin;

        DebugMessage(DEBUG_INIT, "Net = %s (%X)\n", inet_ntoa(net), net.s_addr);
#endif
        /* set the final homenet address up */
        pv.homenet = ((u_long) net.s_addr & pv.netmask);

#ifdef DEBUG
        sin.s_addr = pv.homenet;
        DebugMessage(DEBUG_INIT, "Homenet = %s (%X)\n", inet_ntoa(sin), sin.s_addr);
#endif
    }

    mSplitFree(&toks, num_toks);
#endif
}



void GenObfuscationMask(char *netdata)
{
#ifdef SUP_IP6
    sfip_pton(netdata, &pv.obfuscation_net);
#else
    struct in_addr net;       /* place to stick the local network data */
    char **toks;              /* dbl ptr to store mSplit return data in */
    int num_toks;             /* number of tokens mSplit returns */
    int nmask;                /* temporary netmask storage */

    DEBUG_WRAP(DebugMessage(DEBUG_INIT, "Got obfus data: %s\n", netdata););

    /* break out the CIDR notation from the IP address */
    toks = mSplit(netdata, "/", 2, &num_toks, 0);

    if(num_toks > 1)
    {
        /* convert the CIDR notation into a real live netmask */
        nmask = atoi(toks[1]);

        if((nmask > 0) && (nmask < 33))
        {
            pv.obfuscation_mask = netmasks[nmask];
        }
        else
        {
            FatalError("Bad CIDR block in obfuscation mask [%s:%d], "
                       "1 to 32 please!\n", toks[1], pv.obfuscation_mask);
        }
    }
    else
    {
        FatalError("No netmask specified for obsucation mask!\n");
    }

    pv.obfuscation_mask = htonl(pv.obfuscation_mask);

    DEBUG_WRAP(DebugMessage(DEBUG_INIT, "obfuscation netmask = %#8lX\n", 
                pv.obfuscation_mask););

    /* convert the IP addr into its 32-bit value */
    if((net.s_addr = inet_addr(toks[0])) == -1)
    {
        FatalError("Obfuscation mask (%s) didn't translate\n",
                   toks[0]);
    }
    else
    {
        struct in_addr sin;

        DEBUG_WRAP(DebugMessage(DEBUG_INIT, "Obfuscation Net = %s (%X)\n", 
                inet_ntoa(net), net.s_addr););

        /* set the final homenet address up */
        pv.obfuscation_net = ((u_long) net.s_addr & pv.obfuscation_mask);

        sin.s_addr = pv.obfuscation_net;
        DEBUG_WRAP(DebugMessage(DEBUG_INIT, "Obfuscation Net = %s (%X)\n", 
                inet_ntoa(sin), sin.s_addr););
        pv.obfuscation_mask = ~pv.obfuscation_mask;
    }

    mSplitFree(&toks, num_toks);
#endif
}

/****************************************************************************
 *
 * Function  : DefineAllIfaceVars()
 * Purpose   : Find all up interfaces and define iface_ADDRESS vars for them
 * Arguments : none
 * Returns   : void function
 *
 ****************************************************************************/
#if 0
void DefineAllIfaceVars()
{
#ifndef SOURCEFIRE
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_if_t *alldevs;
    bpf_u_int32 net, netmask;

    if (pcap_findalldevs(&alldevs, errbuf) == -1)
        return;

    while (alldevs != NULL)
    {
        if (pcap_lookupnet(alldevs->name, &net, &netmask, errbuf) == 0)
        {
            DefineIfaceVar(PRINT_INTERFACE(alldevs->name),
                           (u_char *)&net, 
                           (u_char *)&netmask);
        }

        alldevs = alldevs->next;
    }

    pcap_freealldevs(alldevs);
#endif
}

#endif
/****************************************************************************
 *
 * Function  : DefineIfaceVar()
 * Purpose   : Assign network address and network mast to IFACE_ADDR_VARNAME
 *             variable.
 * Arguments : interface name (string) netaddress and netmask (4 octets each)
 * Returns   : void function
 *
 ****************************************************************************/
#if 0
void DefineIfaceVar(char *iname, u_char * network, u_char * netmask)
{
    char valbuf[32];
    char varbuf[BUFSIZ];

    SnortSnprintf(varbuf, BUFSIZ, "%s_ADDRESS", iname);

    SnortSnprintf(valbuf, 32, "%d.%d.%d.%d/%d.%d.%d.%d",
            network[0] & 0xff, network[1] & 0xff, network[2] & 0xff, 
            network[3] & 0xff, netmask[0] & 0xff, netmask[1] & 0xff, 
            netmask[2] & 0xff, netmask[3] & 0xff);

    VarDefine(varbuf, valbuf);
}
#endif

/****************************************************************************
 *
 * Function: CalcPct(UINT64, UINT64)
 *
 * Purpose:  Calculate the percentage of a value compared to a total
 *
 * Arguments: cnt => the numerator in the equation
 *            total => the denominator in the calculation
 *
 * Returns: pct -> the percentage of cnt to value
 *
 ****************************************************************************/
double CalcPct(UINT64 cnt, UINT64 total)
{
    double pct = 0.0;

    if (total == 0.0)
    {
        pct = (double)cnt;
    }
    else
    {
        pct = (double)cnt / (double)total;
    }

    pct *= 100.0;

    return pct;
}


/****************************************************************************
 *
 * Function: DisplayBanner()
 *
 * Purpose:  Show valuable proggie info
 *
 * Arguments: None.
 *
 * Returns: 0 all the time
 *
 ****************************************************************************/
#if 0
int DisplayBanner()
{
    const char * info;
    const char * pcre_ver;

    info = getenv("HOSTTYPE");
    if( !info )
    {
        info="";
    }

    pcre_ver = pcre_version();

    fprintf(stderr, "\n"
        "   ,,_     -*> Snort! <*-\n"
        "  o\"  )~   Version %s%s%s (Build %s) %s %s\n"
        "   ''''    By Martin Roesch & The Snort Team: http://www.snort.org/team.html\n"
        "           (C) Copyright 1998-2008 Sourcefire Inc., et al.\n"   
        "           Using PCRE version: %s\n"
        "\n"
        , VERSION, 
#ifdef SUP_IP6
          " IPv6",
#else
          "", 
#endif
#ifdef GRE
          " GRE",
#else
          "", 
#endif
        BUILD, 
#ifdef GIDS
        "inline", 
#else
        "",
#endif
        info,
        pcre_ver);

    return 0;
}

#endif

/****************************************************************************
 *
 * Function: ts_print(register const struct, char *)
 *
 * Purpose: Generate a time stamp and stuff it in a buffer.  This one has
 *          millisecond precision.  Oh yeah, I ripped this code off from
 *          TCPdump, props to those guys.
 *
 * Arguments: timeval => clock struct coming out of libpcap
 *            timebuf => buffer to stuff timestamp into
 *
 * Returns: void function
 *
 ****************************************************************************/
void ts_print(register const struct timeval *tvp, char *timebuf)
{
    register int s;
    int    localzone;
    time_t Time;
    struct timeval tv;
    struct timezone tz;
    struct tm *lt;    /* place to stick the adjusted clock data */

    /* if null was passed, we use current time */
    if(!tvp)
    {
        /* manual page (for linux) says tz is never used, so.. */
        bzero((char *) &tz, sizeof(tz));
        gettimeofday(&tv, &tz);
        tvp = &tv;
    }

    localzone = thiszone;
   
    /*
    **  If we're doing UTC, then make sure that the timezone is correct.
    */
    if(pv.use_utc)
        localzone = 0;
        
    s = (tvp->tv_sec + localzone) % 86400;
    Time = (tvp->tv_sec + localzone) - s;

    lt = gmtime(&Time);

    if(pv.include_year)
    {
        (void) SnortSnprintf(timebuf, TIMEBUF_SIZE, 
                        "%02d/%02d/%02d-%02d:%02d:%02d.%06u ", 
                        lt->tm_mon + 1, lt->tm_mday, lt->tm_year - 100, 
                        s / 3600, (s % 3600) / 60, s % 60, 
                        (u_int) tvp->tv_usec);
    } 
    else 
    {
        (void) SnortSnprintf(timebuf, TIMEBUF_SIZE,
                        "%02d/%02d-%02d:%02d:%02d.%06u ", lt->tm_mon + 1,
                        lt->tm_mday, s / 3600, (s % 3600) / 60, s % 60,
                        (u_int) tvp->tv_usec);
    }
}



/****************************************************************************
 *
 * Function: gmt2local(time_t)
 *
 * Purpose: Figures out how to adjust the current clock reading based on the
 *          timezone you're in.  Ripped off from TCPdump.
 *
 * Arguments: time_t => offset from GMT
 *
 * Returns: offset seconds from GMT
 *
 ****************************************************************************/
int gmt2local(time_t t)
{
    register int dt, dir;
    register struct tm *gmt, *loc;
    struct tm sgmt;

    if(t == 0)
        t = time(NULL);

    gmt = &sgmt;
    *gmt = *gmtime(&t);
    loc = localtime(&t);

    dt = (loc->tm_hour - gmt->tm_hour) * 60 * 60 +
        (loc->tm_min - gmt->tm_min) * 60;

    dir = loc->tm_year - gmt->tm_year;

    if(dir == 0)
        dir = loc->tm_yday - gmt->tm_yday;

    dt += dir * 24 * 60 * 60;

    return(dt);
}




/****************************************************************************
 *
 * Function: copy_argv(u_char **)
 *
 * Purpose: Copies a 2D array (like argv) into a flat string.  Stolen from
 *          TCPDump.
 *
 * Arguments: argv => 2D array to flatten
 *
 * Returns: Pointer to the flat string
 *
 ****************************************************************************/
char *copy_argv(char **argv)
{
    char **p;
    u_int len = 0;
    char *buf;
    char *src, *dst;
    //void ftlerr(char *,...);

    p = argv;
    if(*p == 0)
        return 0;

    while(*p)
        len += strlen(*p++) + 1;

    buf = (char *) calloc(1,len);

    if(buf == NULL)
    {
        FatalError("calloc() failed: %s\n", strerror(errno));
    }
    p = argv;
    dst = buf;

    while((src = *p++) != NULL)
    {
        while((*dst++ = *src++) != '\0');
        dst[-1] = ' ';
    }

    dst[-1] = '\0';

    return buf;
}


/****************************************************************************
 *
 * Function: strip(char *)
 *
 * Purpose: Strips a data buffer of CR/LF/TABs.  Replaces CR/LF's with
 *          NULL and TABs with spaces.
 *
 * Arguments: data => ptr to the data buf to be stripped
 *
 * Returns: void
 *
 * 3/7/07 - changed to return void - use strlen to get size of string
 *
 * Note that this function will turn all '\n' and '\r' into null chars
 * so, e.g. 'Hello\nWorld\n' => 'Hello\x00World\x00'
 * note that the string is now just 'Hello' and the length is shortened
 * by more than just an ending '\n' or '\r'
 ****************************************************************************/
void strip(char *data)
{
    int size;
    char *end;
    char *idx;

    idx = data;
    end = data + strlen(data);
    size = end - idx;

    while(idx != end)
    {
        if((*idx == '\n') ||
                (*idx == '\r'))
        {
            *idx = 0;
            size--;
        }
        if(*idx == '\t')
        {
            *idx = ' ';
        }
        idx++;
    }
}


/****************************************************************************
 *
 * Function: InitNetMasks()
 *
 * Purpose: Loads the netmask struct in network order.  Yes, I know I could
 *          just load the array when I define it, but this is what occurred
 *          to me when I wrote this at 3:00 AM.
 *
 * Arguments: None.
 *
 * Returns: void function
 *
 ****************************************************************************/
extern u_long netmasks[33]; /* defined in snort.c */

void InitNetmasks()
{
    netmasks[0] = 0x0;
    netmasks[1] = 0x80000000;
    netmasks[2] = 0xC0000000;
    netmasks[3] = 0xE0000000;
    netmasks[4] = 0xF0000000;
    netmasks[5] = 0xF8000000;
    netmasks[6] = 0xFC000000;
    netmasks[7] = 0xFE000000;
    netmasks[8] = 0xFF000000;
    netmasks[9] = 0xFF800000;
    netmasks[10] = 0xFFC00000;
    netmasks[11] = 0xFFE00000;
    netmasks[12] = 0xFFF00000;
    netmasks[13] = 0xFFF80000;
    netmasks[14] = 0xFFFC0000;
    netmasks[15] = 0xFFFE0000;
    netmasks[16] = 0xFFFF0000;
    netmasks[17] = 0xFFFF8000;
    netmasks[18] = 0xFFFFC000;
    netmasks[19] = 0xFFFFE000;
    netmasks[20] = 0xFFFFF000;
    netmasks[21] = 0xFFFFF800;
    netmasks[22] = 0xFFFFFC00;
    netmasks[23] = 0xFFFFFE00;
    netmasks[24] = 0xFFFFFF00;
    netmasks[25] = 0xFFFFFF80;
    netmasks[26] = 0xFFFFFFC0;
    netmasks[27] = 0xFFFFFFE0;
    netmasks[28] = 0xFFFFFFF0;
    netmasks[29] = 0xFFFFFFF8;
    netmasks[30] = 0xFFFFFFFC;
    netmasks[31] = 0xFFFFFFFE;
    netmasks[32] = 0xFFFFFFFF;
}


/*
 * Function: ErrorMessage(const char *, ...)
 *
 * Purpose: Print a message to stderr.
 *
 * Arguments: format => the formatted error string to print out
 *            ... => format commands/fillers
 *
 * Returns: void function
 */
void ErrorMessage(const char *format,...)
{
    char buf[STD_BUF+1];
    va_list ap;

    va_start(ap, format);

    if(pv.daemon_flag || pv.logtosyslog_flag)
    {
        vsnprintf(buf, STD_BUF, format, ap);
        buf[STD_BUF] = '\0';
        syslog(LOG_CONS | LOG_DAEMON | LOG_ERR, "%s", buf);
    }
    else
    {
        vfprintf(stderr, format, ap);
    }
    va_end(ap);
}

/*
 * Function: LogMessage(const char *, ...)
 *
 * Purpose: Print a message to stdout or with logfacility.
 *
 * Arguments: format => the formatted error string to print out
 *            ... => format commands/fillers
 *
 * Returns: void function
 */
void LogMessage(const char *format,...)
{
    char buf[STD_BUF+1];
    va_list ap;

    if(pv.quiet_flag && !pv.daemon_flag && !pv.logtosyslog_flag)
        return;

    va_start(ap, format);

    if(pv.daemon_flag || pv.logtosyslog_flag)
    {
        vsnprintf(buf, STD_BUF, format, ap);
        buf[STD_BUF] = '\0';
        syslog(LOG_DAEMON | LOG_NOTICE, "%s", buf);
    }
    else
    {
        vfprintf(stderr, format, ap);
    }
    va_end(ap);
}


/*
 * Function: CreateApplicationEventLogEntry(const char *)
 *
 * Purpose: Add an entry to the Win32 "Application" EventLog
 *
 * Arguments: szMessage => the formatted error string to print out
 *
 * Returns: void function
 */
#if defined(WIN32) && defined(ENABLE_WIN32_SERVICE)
void CreateApplicationEventLogEntry(const char *msg)
{
    HANDLE hEventLog; 
    char*  pEventSourceName = "SnortService";

    /* prepare to write to Application log on local host
      * with Event Source of SnortService
      */
    AddEventSource(pEventSourceName);
    hEventLog = RegisterEventSource(NULL, pEventSourceName);
    if (hEventLog == NULL)
    {
        /* Could not register the event source. */
        return;
    }
 
    if (!ReportEvent(hEventLog,   /* event log handle               */
            EVENTLOG_ERROR_TYPE,  /* event type                     */
            0,                    /* category zero                  */
            EVMSG_SIMPLE,         /* event identifier               */
            NULL,                 /* no user security identifier    */
            1,                    /* one substitution string        */
            0,                    /* no data                        */
            &msg,                 /* pointer to array of strings    */
            NULL))                /* pointer to data                */
    {
        /* Could not report the event. */
    }
 
    DeregisterEventSource(hEventLog); 
} 
#endif  /* WIN32 && ENABLE_WIN32_SERVICE */


/*
 * Function: FatalError(const char *, ...)
 *
 * Purpose: When a fatal error occurs, this function prints the error message
 *          and cleanly shuts down the program
 *
 * Arguments: format => the formatted error string to print out
 *            ... => format commands/fillers
 *
 * Returns: void function
 */
void FatalError(const char *format,...)
{
    char buf[STD_BUF+1];
    va_list ap;

    va_start(ap, format);

    vsnprintf(buf, STD_BUF, format, ap);
    buf[STD_BUF] = '\0';

    if(pv.daemon_flag || pv.logtosyslog_flag)
    {
        syslog(LOG_CONS | LOG_DAEMON | LOG_ERR, "FATAL ERROR: %s", buf);
    }
    else
    {
        fprintf(stderr, "ERROR: %s", buf);
        fprintf(stderr,"Fatal Error, Quitting..\n");
#if defined(WIN32) && defined(ENABLE_WIN32_SERVICE)
        CreateApplicationEventLogEntry(buf);
#endif
    }

    exit(1);
}


/****************************************************************************
 *
 * Function: CreatePidFile(char *)
 *
 * Purpose:  Creates a PID file
 *
 * Arguments: Interface opened.
 *
 * Returns: void function
 *
 ****************************************************************************/
static FILE *pid_lockfile = NULL;
static FILE *pid_file = NULL;
void CreatePidFile(char *intf)
{
    struct stat pt;
    int pid = (int) getpid();
#ifdef WIN32
    char dir[STD_BUF + 1];
#endif

    if (!pv.readmode_flag) 
    {
        if(!pv.quiet_flag)
        {
            LogMessage("Checking PID path...\n");
        }

        if (strlen(pv.pid_path) != 0)
        {
            if((stat(pv.pid_path, &pt) == -1) ||
                !S_ISDIR(pt.st_mode) || access(pv.pid_path, W_OK) == -1)
            {
#ifndef WIN32
                LogMessage("WARNING: %s is invalid, trying "
                           "/var/run...\n", pv.pid_path);
                if (errno)
                {
                    char errbuf[1024];
                    strerror_r(errno, errbuf, 1024);
                    LogMessage("Previous Error, errno=%d, (%s)\n", errno,
                        errbuf);
                }
#endif
                memset(pv.pid_path, '\0', STD_BUF);
            }
            else
            {
                LogMessage("PID path stat checked out ok, "
                           "PID path set to %s\n", pv.pid_path);
            }
        }

        if (strlen(pv.pid_path) == 0)
        {
#ifndef _PATH_VARRUN
#ifndef WIN32
            SnortStrncpy(_PATH_VARRUN, "/var/run/", sizeof(_PATH_VARRUN)); 
#else
            if (GetCurrentDirectory(sizeof(dir) - 1, dir))
                SnortStrncpy(_PATH_VARRUN, dir, sizeof(_PATH_VARRUN));
#endif  /* WIN32 */
#else
            if(!pv.quiet_flag)
            {
                LogMessage("PATH_VARRUN is set to %s on this operating "
                           "system\n", _PATH_VARRUN);
            }
#endif  /* _PATH_VARRUN */

            stat(_PATH_VARRUN, &pt);

            if(!S_ISDIR(pt.st_mode) || access(_PATH_VARRUN, W_OK) == -1)
            {
                LogMessage("WARNING: _PATH_VARRUN is invalid, trying "
                           "/var/log...\n");
                SnortStrncpy(pv.pid_path, "/var/log/", sizeof(pv.pid_path));
                stat(pv.pid_path, &pt);

                if(!S_ISDIR(pt.st_mode) || access(pv.pid_path, W_OK) == -1)
                {
                    LogMessage("WARNING: %s is invalid, logging Snort "
                               "PID path to log directory (%s)\n", pv.pid_path,
                               pv.log_dir);
                    CheckLogDir();
                    SnortSnprintf(pv.pid_path, STD_BUF, "%s/", pv.log_dir);
                }
            }
            else
            {
                LogMessage("PID path stat checked out ok, "
                           "PID path set to %s\n", _PATH_VARRUN);
                SnortStrncpy(pv.pid_path, _PATH_VARRUN, sizeof(pv.pid_path));
            }
        }
    }

    if(intf == NULL || strlen(pv.pid_path) == 0)
    {
        /* pv.pid_path should have some value by now
         * so let us just be sane.
         */
        FatalError("CreatePidFile() failed to lookup interface or pid_path is unknown!\n");
    }

    SnortSnprintf(pv.pid_filename, STD_BUF,  "%s/snort_%s%s.pid", pv.pid_path, intf,
                  pv.pidfile_suffix);

#ifndef WIN32
    if (!pv.nolock_pid_file)
    {
        char pid_lockfilename[STD_BUF+1];
        int lock_fd;

        /* First, lock the PID file */
        SnortSnprintf(pid_lockfilename, STD_BUF, "%s.lck", pv.pid_filename);
        pid_lockfile = fopen(pid_lockfilename, "w");

        if (pid_lockfile)
        {
            struct flock lock;
            lock_fd = fileno(pid_lockfile);

            lock.l_type = F_WRLCK;
            lock.l_whence = SEEK_SET;
            lock.l_start = 0;
            lock.l_len = 0;

            if (fcntl(lock_fd, F_SETLK, &lock) == -1)
            {
                ClosePidFile();
                FatalError("Failed to Lock PID File \"%s\" for PID \"%d\"\n", pv.pid_filename, pid);
            }
        }
    }
#endif
    /* Okay, were able to lock PID file, now open and write PID */
    pid_file = fopen(pv.pid_filename, "w");

    if(pid_file)
    {
        LogMessage("Writing PID \"%d\" to file \"%s\"\n", pid, pv.pid_filename);
        fprintf(pid_file, "%d\n", pid);
        fflush(pid_file);
    }
    else
    {
        ErrorMessage("Failed to create pid file %s", pv.pid_filename);
        pv.pid_filename[0] = 0;
    }
}

/****************************************************************************
 *
 * Function: ClosePidFile(char *)
 *
 * Purpose:  Releases lock on a PID file
 *
 * Arguments: None
 *
 * Returns: void function
 *
 ****************************************************************************/
void ClosePidFile()
{
    if (pid_file)
    {
        fclose(pid_file);
        pid_file = NULL;
    }
    if (pid_lockfile)
    {
        fclose(pid_lockfile);
        pid_lockfile = NULL;
    }
}

#if 0
/****************************************************************************
 *
 * Function: SetUidGid(char *)
 *
 * Purpose:  Sets safe UserID and GroupID if needed
 *
 * Arguments: none
 *
 * Returns: void function
 *
 ****************************************************************************/
void SetUidGid(void)
{
#ifndef WIN32

    if(groupname != NULL)
    {
        if (getgid() != groupid)
        {
            if(!InlineModeSetPrivsAllowed())
            {
                ErrorMessage("Cannot set uid and gid when running Snort in "
                             "inline mode.\n");

                return;
            }

            if(setgid(groupid) < 0)
                FatalError("Can not set gid: %lu\n", (u_long) groupid);

            DEBUG_WRAP(DebugMessage(DEBUG_INIT, "Set gid to %lu\n", groupid););
        }
    }

    if(username != NULL)
    {
        if (getuid() != userid)
        {
            if(!InlineModeSetPrivsAllowed())
            {
                ErrorMessage("Cannot set uid and gid when running Snort in "
                             "inline mode.\n");

                return;
            }

            if(getuid() == 0 && initgroups(username, groupid) < 0)
                FatalError("Can not initgroups(%s,%lu)",
                           username, (u_long) groupid);

            /** just to be on a safe side... **/
            endgrent();
            endpwent();

            if(setuid(userid) < 0)
                FatalError("Can not set uid: %lu\n", (u_long) userid);

            DEBUG_WRAP(DebugMessage(DEBUG_INIT, "Set gid to %lu\n", groupid););
        }
    }
#endif  /* WIN32 */

    return;
}
#endif
#ifdef TIMESTATS

static IntervalStats istats = {0};

void ResetTimeStats(void)
{
    memset(&istats, 0, sizeof(istats));
}

/* This function prints out stats based on a configurable time
 * interval.  It is an indication on how well snort is */
/* processing packets, including types, drops, etc */
void DropStatsPerTimeInterval()
{
    double per_sec, per_minute, per_hour;
    UINT64 recv, drop;
    UINT64 total = 0;

#ifdef PCAP_CLOSE
    if (UpdatePcapPktStats(0) != -1)
#else
    if (UpdatePcapPktStats() != -1)
#endif
    {
        recv = GetPcapPktStatsRecv();
        drop = GetPcapPktStatsDrop();

        istats.recv = recv - istats.recv_total;
        istats.recv_total = recv;

        istats.drop = drop - istats.drop_total;
        istats.drop_total = drop;

        /* calculate received packets by type */
        istats.tcp = pc.tcp - istats.tcp_total;
        istats.tcp_total = pc.tcp;

        istats.udp = pc.udp - istats.udp_total;
        istats.udp_total = pc.udp;

        istats.icmp = pc.icmp - istats.icmp_total;
        istats.icmp_total = pc.icmp;

        istats.arp = pc.arp - istats.arp_total;
        istats.arp_total = pc.arp;

#ifdef GRE
        istats.ip4ip4 = pc.ip4ip4 - istats.ip4ip4_total;
        istats.ip4ip4_total = pc.ip4ip4;

        istats.ip4ip6 = pc.ip4ip6 - istats.ip4ip6_total;
        istats.ip4ip6_total = pc.ip4ip6;

        istats.ip6ip4 = pc.ip6ip4 - istats.ip6ip4_total;
        istats.ip6ip4_total = pc.ip6ip4;

        istats.ip6ip6 = pc.ip6ip6 - istats.ip6ip6_total;
        istats.ip6ip6_total = pc.ip6ip6;

        istats.gre = pc.gre - istats.gre_total;
        istats.gre_total = pc.gre;

        istats.gre_ip = pc.gre_ip - istats.gre_ip_total;
        istats.gre_ip_total = pc.gre_ip;

        istats.gre_eth = pc.gre_eth - istats.gre_eth_total;
        istats.gre_eth_total = pc.gre_eth;

        istats.gre_arp = pc.gre_arp - istats.gre_arp_total;
        istats.gre_arp_total = pc.gre_arp;

        istats.gre_ipv6 = pc.gre_ipv6 - istats.gre_ipv6_total;
        istats.gre_ipv6_total = pc.gre_ipv6;

        istats.gre_ipx = pc.gre_ipx - istats.gre_ipx_total;
        istats.gre_ipx_total = pc.gre_ipx;

        istats.gre_loopback = pc.gre_loopback - istats.gre_loopback_total;
        istats.gre_loopback_total = pc.gre_loopback;

        istats.gre_vlan = pc.gre_vlan - istats.gre_vlan_total;
        istats.gre_vlan_total = pc.gre_vlan;

        istats.gre_ppp = pc.gre_ppp - istats.gre_ppp_total;
        istats.gre_ppp_total = pc.gre_ppp;
#endif

#ifdef DLT_IEEE802_11   /* if we are tracking wireless, add this to output */
        istats.wifi_mgmt = pc.wifi_mgmt - istats.wifi_mgmt_total;
        istats.wifi_mgmt_total = pc.wifi_mgmt;

        istats.wifi_control = pc.wifi_control - istats.wifi_control_total;
        istats.wifi_control_total = pc.wifi_control;

        istats.wifi_data = pc.wifi_data - istats.wifi_data_total;
        istats.wifi_data_total = pc.wifi_data;
#endif

        istats.ipx = pc.ipx - istats.ipx_total;
        istats.ipx_total = pc.ipx;

        istats.eapol = pc.eapol - istats.eapol_total;
        istats.eapol_total = pc.eapol;

        istats.ipv6 = pc.ipv6 - istats.ipv6_total;
        istats.ipv6_total = pc.ipv6;

        istats.ethloopback = pc.ethloopback - istats.ethloopback_total;
        istats.ethloopback_total = pc.ethloopback;

        istats.other = pc.other - istats.other_total;
        istats.other_total = pc.other;

        istats.discards = pc.discards - istats.discards_total;
        istats.discards_total = pc.discards;

        if (pc.frags > 0) /* do we have any fragmented packets being seen? */
        {
            istats.frags = pc.frags - istats.frags_total;
            istats.frags_total = pc.frags;

            istats.frag_trackers = pc.frag_trackers - istats.frag_trackers_total;
            istats.frag_trackers_total = pc.frag_trackers;

            istats.frag_rebuilt = pc.rebuilt_frags - istats.frag_rebuilt_total;
            istats.frag_rebuilt_total = pc.rebuilt_frags;

            istats.frag_element = pc.rebuild_element - istats.frag_element_total;
            istats.frag_element_total = pc.rebuild_element;

            istats.frag_incomp = pc.frag_incomp - istats.frag_incomp_total;
            istats.frag_incomp_total = pc.frag_incomp;

            istats.frag_timeout = pc.frag_timeout - istats.frag_timeout_total;
            istats.frag_timeout_total = pc.frag_timeout;

            istats.frag_mem_faults = pc.frag_mem_faults - istats.frag_mem_faults_total;
            istats.frag_mem_faults_total = pc.frag_mem_faults;
        }

        if (pc.tcp_stream_pkts > 0) /* do we have TCP stream re-assembly going on? */
        {
            istats.tcp_str_packets = pc.tcp_stream_pkts - istats.tcp_str_packets_total;
            istats.tcp_str_packets_total = pc.tcp_stream_pkts;

            istats.tcp_str_trackers = pc.tcp_streams - istats.tcp_str_trackers_total;
            istats.tcp_str_trackers_total = pc.tcp_streams;

            istats.tcp_str_flushes = pc.rebuilt_tcp - istats.tcp_str_flushes_total;
            istats.tcp_str_flushes_total = pc.rebuilt_tcp;

            istats.tcp_str_segs_used = pc.rebuilt_segs - istats.tcp_str_segs_used_total;
            istats.tcp_str_segs_used_total = pc.rebuilt_segs;

            istats.tcp_str_segs_queued = pc.queued_segs - istats.tcp_str_segs_queued_total;
            istats.tcp_str_segs_queued_total = pc.queued_segs;

            istats.tcp_str_mem_faults = pc.str_mem_faults - istats.tcp_str_mem_faults_total;
            istats.tcp_str_mem_faults_total = pc.str_mem_faults;
        }

        istats.processed = pc.total_processed - istats.processed_total;
        istats.processed_total = pc.total_processed;
        total = istats.processed;

        /* prepare packet type per time interval routine */
        LogMessage("================================================"
                   "===============================\n");

        LogMessage("\n");
        LogMessage("Statistics Report (last %d seconds)\n", pv.timestats_interval);
        LogMessage("\n");

        per_sec = (double)istats.recv / (double)pv.timestats_interval;

        LogMessage("Packet Wire Totals:\n");
        LogMessage("Packets received: " FMTu64("13") "\n", istats.recv);

        if (pv.timestats_interval >= SECONDS_PER_HOUR)
        {
            per_hour = (double)(istats.recv * SECONDS_PER_HOUR) / (double)pv.timestats_interval;
        LogMessage("        per hour: %13.2f\n", per_hour);
        }
        if (pv.timestats_interval >= SECONDS_PER_MIN)
        {
            per_minute = (double)(istats.recv * SECONDS_PER_MIN) / (double)pv.timestats_interval;
        LogMessage("      per minute: %13.2f\n", per_minute);
        }
        LogMessage("      per second: %13.2f\n", per_sec);
        LogMessage(" Packets dropped: " FMTu64("13") "\n", istats.drop);
        LogMessage("\n");
        LogMessage("Packet Breakdown by Protocol (includes rebuilt packets):\n");

        LogMessage("     TCP: " FMTu64("10") " (%.3f%%)\n",
                   istats.tcp, CalcPct(istats.tcp, total));
        LogMessage("     UDP: " FMTu64("10") " (%.3f%%)\n",
                   istats.udp, CalcPct(istats.udp, total));
        LogMessage("    ICMP: " FMTu64("10") " (%.3f%%)\n",
                   istats.icmp, CalcPct(istats.icmp, total));
        LogMessage("     ARP: " FMTu64("10") " (%.3f%%)\n",
                   istats.arp, CalcPct(istats.arp, total));
        LogMessage("   EAPOL: " FMTu64("10") " (%.3f%%)\n",
                   istats.eapol, CalcPct(istats.eapol, total));
        LogMessage("    IPv6: " FMTu64("10") " (%.3f%%)\n",
                   istats.ipv6, CalcPct(istats.ipv6, total));
        LogMessage(" ETHLOOP: " FMTu64("10") " (%.3f%%)\n",
                   istats.ethloopback, CalcPct(istats.ethloopback, total));
        LogMessage("     IPX: " FMTu64("10") " (%.3f%%)\n",
                   istats.ipx, CalcPct(istats.ipx, total));

#ifdef GRE
        LogMessage(" IP4/IP4: " FMTu64("-10") " (%.3f%%)\n", 
                   istats.ip4ip4, CalcPct(istats.ip4ip4, total));
        LogMessage(" IP4/IP6: " FMTu64("-10") " (%.3f%%)\n", 
                   istats.ip4ip6, CalcPct(istats.ip4ip6, total));
        LogMessage(" IP6/IP4: " FMTu64("-10") " (%.3f%%)\n", 
                   istats.ip6ip4, CalcPct(istats.ip6ip4, total));
        LogMessage(" IP6/IP6: " FMTu64("-10") " (%.3f%%)\n", 
                   istats.ip6ip6, CalcPct(istats.ip6ip6, total));
        LogMessage("     GRE: " FMTu64("10") " (%.3f%%)\n",
                   istats.gre, CalcPct(istats.gre, total));
        LogMessage(" GRE ETH: " FMTu64("-10") " (%.3f%%)\n", 
                   istats.gre_eth, CalcPct(istats.gre_eth, total));
        LogMessage("GRE VLAN: " FMTu64("-10") " (%.3f%%)\n", 
                   istats.gre_vlan, CalcPct(istats.gre_vlan, total));
        LogMessage("  GRE IP: " FMTu64("-10") " (%.3f%%)\n", 
                   istats.gre_ip, CalcPct(istats.gre_ip, total));
        LogMessage("GRE IPv6: " FMTu64("-10") " (%.3f%%)\n", 
                   istats.gre_ipv6, CalcPct(istats.gre_ipv6, total));
        LogMessage("GRE PPTP: " FMTu64("-10") " (%.3f%%)\n", 
                   istats.gre_ppp, CalcPct(istats.gre_ppp, total));
        LogMessage(" GRE ARP: " FMTu64("-10") " (%.3f%%)\n", 
                   istats.gre_arp, CalcPct(istats.gre_arp, total));
        LogMessage(" GRE IPX: " FMTu64("-10") " (%.3f%%)\n", 
                   istats.gre_ipx, CalcPct(istats.gre_ipx, total));
        LogMessage("GRE LOOP: " FMTu64("-10") " (%.3f%%)\n", 
                   istats.gre_loopback, CalcPct(istats.gre_loopback, total));
#endif

        LogMessage("    FRAG: " FMTu64("10") " (%.3f%%)\n",
                   istats.frags, CalcPct(istats.frags, total));
        LogMessage("   OTHER: " FMTu64("10") " (%.3f%%)\n",
                   istats.other, CalcPct(istats.other, total));
        LogMessage(" DISCARD: " FMTu64("10") " (%.3f%%)\n",
                   istats.discards, CalcPct(istats.discards, total));
        LogMessage("   Total: " FMTu64("10") "\n", total);

        LogMessage("\n");


        /*   handle case where wireless is enabled...	*/

#ifdef DLT_IEEE802_11
        if (datalink == DLT_IEEE802_11)
        {
            LogMessage("\n");
            LogMessage("Wireless Stats:\n\n");
            LogMessage("Management Packets: " FMTu64("10") " (%.3f%%)\n",
                       istats.wifi_mgmt, CalcPct(istats.wifi_mgmt, total));
            LogMessage("   Control Packets: " FMTu64("10") " (%.3f%%)\n",
                       istats.wifi_control, CalcPct(istats.wifi_control, total));
            LogMessage("      Data Packets: " FMTu64("10") " (%.3f%%)\n",
                       istats.wifi_data, CalcPct(istats.wifi_data, total));
            LogMessage("\n");
        }

#endif /* if wireless is enabled... */

        /*   handle case where we have snort seeing fragmented packets	*/

        if (pc.frags > 0) /* begin if (pc.frags > 0) */
        {
            LogMessage("\n");
            LogMessage("Fragmentation Stats:\n\n");
            LogMessage("Fragmented IP Packets: " FMTu64("10") "\n", istats.frags);
            LogMessage("    Fragment Trackers: " FMTu64("10") "\n", istats.frag_trackers);
            LogMessage("   Rebuilt IP Packets: " FMTu64("10") "\n", istats.frag_rebuilt);
            LogMessage("   Frag Elements Used: " FMTu64("10") "\n", istats.frag_element);
            LogMessage("Discarded(incomplete): " FMTu64("10") "\n", istats.frag_incomp);
            LogMessage("   Discarded(timeout): " FMTu64("10") "\n", istats.frag_timeout);
            LogMessage("  Frag2 memory faults: " FMTu64("10") "\n", istats.frag_mem_faults);
            LogMessage("\n");
        }   /* end if (pc.frags > 0) */

        /*   handle TCP stream re-assy stuff here */ 

        if (pc.tcp_stream_pkts > 0)
        {
            LogMessage("\n");
            LogMessage("TCP Stream Reassembly Stats:\n\n");
            LogMessage("      TCP Packets Used: " FMTu64("10") "\n", istats.tcp_str_packets);
            LogMessage("       Stream Trackers: " FMTu64("10") "\n", istats.tcp_str_trackers);
            LogMessage("        Stream Flushes: " FMTu64("10") "\n", istats.tcp_str_flushes);
            LogMessage("  Stream Segments Used: " FMTu64("10") "\n", istats.tcp_str_segs_used);
            LogMessage("Stream Segments Queued: " FMTu64("10") "\n", istats.tcp_str_segs_queued);
            LogMessage(" Stream4 Memory Faults: " FMTu64("10") "\n", istats.tcp_str_mem_faults);
            LogMessage("\n");
        }

        mpse_print_qinfo();

    }  /* end if pcap_stats(ps, &ps) */

    alarm(pv.timestats_interval);   /* reset the alarm to go off again */
}

/* print out stats on how long snort ran */
void TimeStats(void)
{

/*
 *  variable definitions for improved statistics handling
 *
 *  end_time = time which snort finished running (unix epoch)
 *  total_secs = total amount of time snort ran
 *  int_total_secs = used to eliminate casts from this function (temp. var)
 *  days = number of days snort ran
 *  hrs  = number of hrs snort ran
 *  mins = number of minutes snort ran
 *  secs = number of seconds snort ran
 *
 *  ival = temp. variable for integer/modulus math
 *  ppd  = packets per day processed
 *  pph  = packets per hour processed
 *  ppm  = packets per minute processed
 *  pps  = packets per second processed
 *
 *  hflag = used to flag when hrs = zero, but days > 0
 *  mflag = used to flag when min = zero, but hrs > 0
 *
 */

    time_t end_time, total_secs;
    u_int32_t days = 0, hrs = 0, mins = 0, secs = 0, tmp = 0;
    UINT64 pps = 0, ppm = 0, pph = 0, ppd = 0;
    u_int32_t int_total_secs = 0;
    char hflag = 0, mflag = 0;


    end_time = time(&end_time);         /* grab epoch for end time value (in seconds) */
    total_secs = end_time - start_time; /* total_secs is how many seconds snort ran for */

    tmp = (u_int32_t)total_secs;        
    int_total_secs = tmp;               /* used for cast elimination */

    days = tmp / SECONDS_PER_DAY;       /* 86400 is number of seconds in a day */
    tmp  = tmp % SECONDS_PER_DAY;       /* grab remainder to process hours */
    hrs  = tmp / SECONDS_PER_HOUR;      /* 3600 is number of seconds in a(n) hour */
    tmp  = tmp % SECONDS_PER_HOUR;      /* grab remainder to process minutes */
    mins = tmp / SECONDS_PER_MIN;       /* 60 is number of seconds in a minute */
    secs = tmp % SECONDS_PER_MIN;       /* grab remainder to process seconds */

    if (total_secs)
        pps = (pc.total_from_pcap / int_total_secs);
    else                                         
        pps = pc.total_from_pcap;     /* guard against division by zero */

    LogMessage("Snort ran for %u Days %u Hours %u Minutes %u Seconds\n", days, hrs, mins, secs);

    if (days > 0) {
        ppd = (pc.total_from_pcap / (int_total_secs / SECONDS_PER_DAY));
        LogMessage("Snort Analyzed " STDu64 " Packets Per Day\n", ppd);
        hflag = 1;
    }

    if (hrs > 0 || hflag == 1) {
        pph = (pc.total_from_pcap / (int_total_secs / SECONDS_PER_HOUR));
        LogMessage("Snort Analyzed " STDu64 " Packets Per Hour\n", pph);
        mflag = 1;
    }

    if (mins > 0 || mflag == 1) {
        ppm = (pc.total_from_pcap / (int_total_secs / SECONDS_PER_MIN));
        LogMessage("Snort Analyzed " STDu64 " Packets Per Minute\n", ppm);
    }

    LogMessage("Snort Analyzed " STDu64 " Packets Per Second\n", pps);
    LogMessage("\n");
}
#endif /* TIMESTATS */


#if 0
#ifdef PCAP_CLOSE
int UpdatePcapPktStats(int cacheReturn)
#else
int UpdatePcapPktStats(void)
#endif
{
    struct pcap_stat ps;
    u_int32_t recv, drop;
    static char not_initialized = 1;

#ifdef PCAP_CLOSE
    static int priorReturn = 0;
    static int returnWasCached = 0;

    if ( !cacheReturn && returnWasCached )
    {
        returnWasCached = 0;
        return priorReturn;
    }
    priorReturn = -1;
    returnWasCached = cacheReturn;
#endif

    if (not_initialized)
    {
        memset(&pkt_stats, 0, sizeof(PcapPktStats));
        not_initialized = 0;
    }
    
    if ((pd == NULL) || pv.readmode_flag)
        return -1;

    if (pcap_stats(pd, &ps) == -1)
    {
        pcap_perror(pd, "pcap_stats");
        return -1;
    }

    recv = (u_int32_t)ps.ps_recv;
    drop = (u_int32_t)ps.ps_drop;

#ifdef LINUX_LIBPCAP_DOUBLES_STATS
    recv /= 2;
    drop /= 2;
#endif

#ifdef LIBPCAP_ACCUMULATES
    /* pcap recv wrapped */
    if (recv < pkt_stats.wrap_recv)
        pkt_stats.recv += (UINT64)UINT32_MAX;

    /* pcap drop wrapped */
    if (drop < pkt_stats.wrap_drop)
        pkt_stats.drop += (UINT64)UINT32_MAX;

    pkt_stats.wrap_recv = recv;
    pkt_stats.wrap_drop = drop;
#else
    pkt_stats.recv += (UINT64)recv;
    pkt_stats.drop += (UINT64)drop;
#endif  /* LIBPCAP_ACCUMULATES */

#ifdef PCAP_CLOSE
    priorReturn = 0;
#endif
    return 0;
}
#endif

UINT64 GetPcapPktStatsRecv(void)
{
    return pkt_stats.recv + (UINT64)pkt_stats.wrap_recv;
}

UINT64 GetPcapPktStatsDrop(void)
{
    return pkt_stats.drop + (UINT64)pkt_stats.wrap_drop;
}

#if 0
#ifdef PCAP_CLOSE
/* exiting should be 0 for if not exiting, 1 if restarting, and 2 if exiting */
#else
/* exiting should be 0 for if not exiting and 1 if exiting */
#endif
void DropStats(int exiting)
{
    PreprocessStatsList *idx;
    UINT64 total = 0;
    UINT64 pkts_recv;
    UINT64 pkts_drop;

    total = pc.total_processed;

#ifndef TIMESTATS
    if(pv.quiet_flag)
        return;
#endif

#ifdef PPM_MGR
    PPM_PRINT_SUMMARY();
#endif

    LogMessage("================================================"
               "===============================\n");
    /*
     * you will hardly run snort in daemon mode and read from file i that is
     * why no `LogMessage()' here
     */
    if(pv.readmode_flag || InlineMode())
    {
        printf("Snort processed " STDu64 " packets.\n", total);
    }
    else
    {
#ifdef PCAP_CLOSE
        if (exiting < 2 && !pd)
#else
        if (!pd)
#endif
        {
            LogMessage("Snort received 0 packets\n");
        }
        else
        {
#ifdef PCAP_CLOSE
            if (UpdatePcapPktStats(0) != -1)
#else
            if (UpdatePcapPktStats() != -1)
#endif
            {
#ifdef TIMESTATS
                {
                    int oldQFlag = pv.quiet_flag;
                    pv.quiet_flag = 0;
                    TimeStats();     /* how long did snort run? */
                    pv.quiet_flag = oldQFlag;
                }
#endif
                pkts_recv = GetPcapPktStatsRecv();
                pkts_drop = GetPcapPktStatsDrop();

                LogMessage("Packet Wire Totals:\n");
                LogMessage("   Received: " FMTu64("12") "\n", pkts_recv);
                LogMessage("   Analyzed: " FMTu64("12") " (%.3f%%)\n", pc.total_from_pcap,
                           CalcPct(pc.total_from_pcap, pkts_recv));
                LogMessage("    Dropped: " FMTu64("12") " (%.3f%%)\n", pkts_drop,
                           CalcPct(pkts_drop, pkts_recv));
                LogMessage("Outstanding: " FMTu64("12") " (%.3f%%)\n",
                           pkts_recv - pkts_drop - pc.total_from_pcap,
                           CalcPct((pkts_recv - pkts_drop - pc.total_from_pcap), pkts_recv));
            }
        }
    }

    LogMessage("================================================"
               "===============================\n");

    LogMessage("Breakdown by protocol (includes rebuilt packets):\n");

    LogMessage("      ETH: " FMTu64("-10") " (%.3f%%)\n", 
               pc.eth, CalcPct(pc.eth, total));
    LogMessage("  ETHdisc: " FMTu64("-10") " (%.3f%%)\n", 
               pc.ethdisc, CalcPct(pc.ethdisc, total));
#ifdef GIDS
#ifndef IPFW
    LogMessage(" IPTables: " FMTu64("-10") " (%.3f%%)\n", 
               pc.iptables, CalcPct(pc.iptables, total));
#else
    LogMessage("     IPFW: " FMTu64("-10") " (%.3f%%)\n", 
               pc.ipfw, CalcPct(pc.ipfw, total));
#endif  /* IPFW */
#endif  /* GIDS */
    LogMessage("     VLAN: " FMTu64("-10") " (%.3f%%)\n", 
               pc.vlan, CalcPct(pc.vlan, total));

    LogMessage("     IPV6: " FMTu64("-10") " (%.3f%%)\n", 
               pc.ipv6, CalcPct(pc.ipv6, total));
    LogMessage("  IP6 EXT: " FMTu64("-10") " (%.3f%%)\n", 
               pc.ip6ext, CalcPct(pc.ip6ext, total));
    LogMessage("  IP6opts: " FMTu64("-10") " (%.3f%%)\n", 
               pc.ipv6opts, CalcPct(pc.ipv6opts, total));
    LogMessage("  IP6disc: " FMTu64("-10") " (%.3f%%)\n", 
               pc.ipv6disc, CalcPct(pc.ipv6disc, total));

    LogMessage("      IP4: " FMTu64("-10") " (%.3f%%)\n", 
               pc.ip, CalcPct(pc.ip, total));
    LogMessage("  IP4disc: " FMTu64("-10") " (%.3f%%)\n", 
               pc.ipdisc, CalcPct(pc.ipdisc, total));

    LogMessage("    TCP 6: " FMTu64("-10") " (%.3f%%)\n", 
               pc.tcp6, CalcPct(pc.tcp6, total));
    LogMessage("    UDP 6: " FMTu64("-10") " (%.3f%%)\n", 
               pc.udp6, CalcPct(pc.udp6, total));
    LogMessage("    ICMP6: " FMTu64("-10") " (%.3f%%)\n", 
               pc.icmp6, CalcPct(pc.icmp6, total));
    LogMessage("  ICMP-IP: " FMTu64("-10") " (%.3f%%)\n", 
               pc.embdip, CalcPct(pc.embdip, total));

    LogMessage("      TCP: " FMTu64("-10") " (%.3f%%)\n", 
               pc.tcp, CalcPct(pc.tcp, total));
    LogMessage("      UDP: " FMTu64("-10") " (%.3f%%)\n", 
               pc.udp, CalcPct(pc.udp, total));
    LogMessage("     ICMP: " FMTu64("-10") " (%.3f%%)\n", 
               pc.icmp, CalcPct(pc.icmp, total));

    LogMessage("  TCPdisc: " FMTu64("-10") " (%.3f%%)\n", 
               pc.tdisc, CalcPct(pc.tdisc, total));
    LogMessage("  UDPdisc: " FMTu64("-10") " (%.3f%%)\n", 
               pc.udisc, CalcPct(pc.udisc, total));
    LogMessage("  ICMPdis: " FMTu64("-10") " (%.3f%%)\n", 
               pc.icmpdisc, CalcPct(pc.icmpdisc, total));

    LogMessage("     FRAG: " FMTu64("-10") " (%.3f%%)\n", 
               pc.frags, CalcPct(pc.frags, total));
    LogMessage("   FRAG 6: " FMTu64("-10") " (%.3f%%)\n", 
               pc.frag6, CalcPct(pc.frag6, total));

    LogMessage("      ARP: " FMTu64("-10") " (%.3f%%)\n", 
               pc.arp, CalcPct(pc.arp, total));
    LogMessage("    EAPOL: " FMTu64("-10") " (%.3f%%)\n", 
               pc.eapol, CalcPct(pc.eapol, total));
    LogMessage("  ETHLOOP: " FMTu64("-10") " (%.3f%%)\n", 
               pc.ethloopback, CalcPct(pc.ethloopback, total));
    LogMessage("      IPX: " FMTu64("-10") " (%.3f%%)\n", 
               pc.ipx, CalcPct(pc.ipx, total));
#ifdef GRE
    LogMessage("IPv4/IPv4: " FMTu64("-10") " (%.3f%%)\n", 
               pc.ip4ip4, CalcPct(pc.ip4ip4, total));
    LogMessage("IPv4/IPv6: " FMTu64("-10") " (%.3f%%)\n", 
               pc.ip4ip6, CalcPct(pc.ip4ip6, total));
    LogMessage("IPv6/IPv4: " FMTu64("-10") " (%.3f%%)\n", 
               pc.ip6ip4, CalcPct(pc.ip6ip4, total));
    LogMessage("IPv6/IPv6: " FMTu64("-10") " (%.3f%%)\n", 
               pc.ip6ip6, CalcPct(pc.ip6ip6, total));
    LogMessage("      GRE: " FMTu64("-10") " (%.3f%%)\n", 
               pc.gre, CalcPct(pc.gre, total));
    LogMessage("  GRE ETH: " FMTu64("-10") " (%.3f%%)\n", 
               pc.gre_eth, CalcPct(pc.gre_eth, total));
    LogMessage(" GRE VLAN: " FMTu64("-10") " (%.3f%%)\n", 
               pc.gre_vlan, CalcPct(pc.gre_vlan, total));
    LogMessage(" GRE IPv4: " FMTu64("-10") " (%.3f%%)\n", 
               pc.gre_ip, CalcPct(pc.gre_ip, total));
    LogMessage(" GRE IPv6: " FMTu64("-10") " (%.3f%%)\n", 
               pc.gre_ipv6, CalcPct(pc.gre_ipv6, total));
    LogMessage("GRE IP6 E: " FMTu64("-10") " (%.3f%%)\n", 
               pc.gre_ipv6ext, CalcPct(pc.gre_ipv6ext, total));
    LogMessage(" GRE PPTP: " FMTu64("-10") " (%.3f%%)\n", 
               pc.gre_ppp, CalcPct(pc.gre_ppp, total));
    LogMessage("  GRE ARP: " FMTu64("-10") " (%.3f%%)\n", 
               pc.gre_arp, CalcPct(pc.gre_arp, total));
    LogMessage("  GRE IPX: " FMTu64("-10") " (%.3f%%)\n", 
               pc.gre_ipx, CalcPct(pc.gre_ipx, total));
    LogMessage(" GRE LOOP: " FMTu64("-10") " (%.3f%%)\n", 
               pc.gre_loopback, CalcPct(pc.gre_loopback, total));
#endif  /* GRE */
    LogMessage("    OTHER: " FMTu64("-10") " (%.3f%%)\n", 
               pc.other, CalcPct(pc.other, total));
    LogMessage("  DISCARD: " FMTu64("-10") " (%.3f%%)\n", 
               pc.discards, CalcPct(pc.discards, total));
    LogMessage("InvChkSum: " FMTu64("-10") " (%.3f%%)\n", 
               pc.invalid_checksums, CalcPct(pc.invalid_checksums, total));

    LogMessage("   S5 G 1: " FMTu64("-10") " (%.3f%%)\n", 
               pc.s5tcp1, CalcPct(pc.s5tcp1, total));
    LogMessage("   S5 G 2: " FMTu64("-10") " (%.3f%%)\n", 
               pc.s5tcp2, CalcPct(pc.s5tcp2, total));

    LogMessage("    Total: " FMTu64("-10") "\n", total);

    LogMessage("================================================"
               "===============================\n");

    LogMessage("Action Stats:\n");
    LogMessage("ALERTS: " STDu64 "\n", pc.alert_pkts);
    LogMessage("LOGGED: " STDu64 "\n", pc.log_pkts);
    LogMessage("PASSED: " STDu64 "\n", pc.pass_pkts);

#ifdef TARGET_BASED
    LogMessage("================================================"
               "===============================\n");
    LogMessage("Attribute Table Stats:\n");
    LogMessage("    Number Entries: %u\n", SFAT_NumberOfHosts());
    LogMessage("    Table Reloaded: " STDu64 "\n", pc.attribute_table_reloads);
#endif  /* TARGET_BASED */

    mpse_print_qinfo();

#ifdef DLT_IEEE802_11
    if(datalink == DLT_IEEE802_11)
    {
        LogMessage("================================================"
                   "===============================\n");
        LogMessage("Wireless Stats:\n");
        LogMessage("Breakdown by type:\n");
        LogMessage("    Management Packets: " FMTu64("-10") " (%.3f%%)\n", 
                   pc.wifi_mgmt, CalcPct(pc.wifi_mgmt, total));
        LogMessage("    Control Packets:    " FMTu64("-10") " (%.3f%%)\n", 
                   pc.wifi_control, CalcPct(pc.wifi_control, total));
        LogMessage("    Data Packets:       " FMTu64("-10") " (%.3f%%)\n", 
                   pc.wifi_data, CalcPct(pc.wifi_data, total));
    }
#endif  /* DLT_IEEE802_11 */

    for (idx = PreprocessStats; idx != NULL; idx = idx->next)
    {
        LogMessage("=============================================="
                   "=================================\n");

#ifdef PCAP_CLOSE
        idx->entry.func(exiting ? 1 : 0);
#else
        idx->entry.func(exiting);
#endif
    }

    LogMessage("=============================================="
               "=================================\n");

    return;
}

#endif
/****************************************************************************
 *
 * Function: InitProtoNames()
 *
 * Purpose: Initializes the protocol names
 *
 * Arguments: None.
 *
 * Returns: void function
 *
 ****************************************************************************/
void InitProtoNames()
{
    int i;
    struct protoent *pt;
    u_char *tmp;
    u_char protoname[12];

    for(i = 0; i < 256; i++)
    {
        pt = getprotobynumber(i);

        if(pt)
        {
            protocol_names[i] = SnortStrdup(pt->p_name);

            tmp = (u_char*)protocol_names[i];

            for(tmp = (u_char*)protocol_names[i]; *tmp != 0; tmp++)
                *tmp = (u_char) toupper(*tmp);
        }
        else
        {
            SnortSnprintf((char*)protoname, 11, "PROTO:%03d", i);
            protocol_names[i] = SnortStrdup((char*)protoname);
        }
    }
}

/****************************************************************************
 *
 * Function: CleanupProtoNames()
 *
 * Purpose: Frees the protocol names
 *
 * Arguments: None.
 *
 * Returns: void function
 *
 ****************************************************************************/
void CleanupProtoNames()
{
    int i;

    for(i = 0; i < 256; i++)
    {
        if( protocol_names[i] != NULL )
        {
            free( protocol_names[i] );
            protocol_names[i] = NULL;
        }
    }
}

/****************************************************************************
 *
 * Function: read_infile(char *)
 *
 * Purpose: Reads the BPF filters in from a file.  Ripped from tcpdump.
 *
 * Arguments: fname => the name of the file containing the BPF filters
 *
 * Returns: the processed BPF string
 *
 ****************************************************************************/
char *read_infile(char *fname)
{
    register int fd, cc;
    register char *cp, *cmt;
    struct stat buf;

    fd = open(fname, O_RDONLY);

    if(fd < 0){
        //FatalError("can't open %s: %s\n", fname, pcap_strerror(errno));
	}

    if(fstat(fd, &buf) < 0){
        //FatalError("can't stat %s: %s\n", fname, pcap_strerror(errno));
	}

    cp = (char *)SnortAlloc(((u_int)buf.st_size + 1) * sizeof(char));

    cc = read(fd, cp, (int) buf.st_size);

    if(cc < 0){
        //FatalError("read %s: %s\n", fname, pcap_strerror(errno));
	}

    if(cc != buf.st_size)
        FatalError("short read %s (%d != %d)\n", fname, cc, (int) buf.st_size);

    cp[(int) buf.st_size] = '\0';

    close(fd);

    /* Treat everything upto the end of the line as a space
     *  so that we can put comments in our BPF filters
     */
    
    while((cmt = strchr(cp, '#')) != NULL)
    {
        while (*cmt != '\r' && *cmt != '\n' && *cmt != '\0')
        {
            *cmt++ = ' ';
        }
    }

    /** LogMessage("BPF filter file: %s\n", fname); **/
    
    return(cp);
}


 /****************************************************************************
  *
  * Function: CheckLogDir()
  *
  * Purpose: CyberPsychotic sez: basically we only check if logdir exist and
  *          writable, since it might screw the whole thing in the middle. Any
  *          other checks could be performed here as well.
  *
  * Arguments: None.
  *
  * Returns: void function
  *
  ****************************************************************************/
void CheckLogDir(void)
{
    struct stat st;
    char log_dir[STD_BUF];

    SnortSnprintf(log_dir, STD_BUF, "%s", pv.log_dir);
    stat(log_dir, &st);

    if(!S_ISDIR(st.st_mode) || access(log_dir, W_OK) == -1)
    {
        FatalError("\n[!] ERROR: "
                "Can not get write access to logging directory \"%s\".\n"
                "(directory doesn't exist or permissions are set incorrectly\n"
                /*
                 * let us add this comment. Too many people seem to
                 * confuse it otherwise :-)
                 */
            "or it is not a directory at all)\n\n",
        log_dir);
    }
}

/* Signal handler for child process signaling the parent
 * that is is ready */
static int parent_wait = 1;
static void SigChildReadyHandler(int signal)
{
#ifdef DEBUG
    LogMessage("Received Signal from Child\n");
#endif
    parent_wait = 0;
}

/****************************************************************************
 *
 * Function: GoDaemon()
 *
 * Purpose: Puts the program into daemon mode, nice and quiet like....
 *
 * Arguments: None.
 *
 * Returns: void function
 *
 ****************************************************************************/
void GoDaemon(void)
{
#ifndef WIN32
    int exit_val = 0;
    pid_t fs;

    LogMessage("Initializing daemon mode\n");

    if (pv.daemon_restart_flag)
        return;

    /* Don't daemonize if we've already daemonized and
     * received a SIGHUP. */
    if(getppid() != 1)
    {
        /* Register signal handler that parent can trap signal */
        signal(SIGNAL_SNORT_CHILD_READY, SigChildReadyHandler);
        if (errno != 0) errno=0;

        /* now fork the child */
        fs = fork();

        if(fs > 0)
        {
            /* Parent */

            /* Don't exit quite yet.  Wait for the child
             * to signal that is there and created the PID
             * file.
             */
            while (parent_wait)
            {
                /* Continue waiting until receiving signal from child */
                int status;
                if (waitpid(fs, &status, WNOHANG) == fs)
                {
                    /* If the child is gone, parent should go away, too */
                    if (WIFEXITED(status))
                    {
                        LogMessage("Child exited unexpectedly\n");
                        exit_val = -1;
                        break;
                    }
                    if (WIFSIGNALED(status))
                    {
                        LogMessage("Child terminated unexpectedly\n");
                        exit_val = -2;
                        break;
                    }
                }

#ifdef DEBUG
                LogMessage("Parent waiting for child...\n");
#endif

                sleep(1);
            }

            LogMessage("Daemon parent exiting\n");

            exit(exit_val);                /* parent */
        }

        if(fs < 0)
        {
            /* Daemonizing failed... */
            perror("fork");
            exit(1);
        }

        /* Child */
        setsid();
    }
    /* redirect stdin/stdout/stderr to /dev/null */
    close(0);
    close(1);
    close(2);

#ifdef DEBUG
    open("/tmp/snort.debug", O_CREAT | O_RDWR);
#else
    open("/dev/null", O_RDWR);
#endif

    dup(0);
    dup(0);
#endif /* ! WIN32 */
    return;
}

/* Signal the parent that child is ready */
void SignalWaitingParent(void)
{
#ifndef WIN32
    pid_t parentpid = getppid();
#ifdef DEBUG
    LogMessage("Signaling parent %d from child %d\n", parentpid, getpid());
#endif

    if (kill(parentpid, SIGNAL_SNORT_CHILD_READY))
    {
        LogMessage("Daemon initialized, failed to signal parent pid: %d, failure: %d, %s\n", parentpid, errno, strerror(errno));
    }
    else
    {
        LogMessage("Daemon initialized, signaled parent pid: %d\n", parentpid);
    }
#endif
}

/* This function has been moved into mstring.c, since that
*  is where the allocation actually occurs.  It has been
*  renamed to mSplitFree().
*
void FreeToks(char **toks, int num_toks)
{
    if (toks)
    {
        if (num_toks > 0)
        {
            do
            {
                num_toks--;
                free(toks[num_toks]);
            } while(num_toks);
        }
        free(toks);
    }
}
*/


/* Self preserving memory allocator */
void *SPAlloc(unsigned long size, struct _SPMemControl *spmc)
{
    void *tmp;

    spmc->mem_usage += size;

    if(spmc->mem_usage > spmc->memcap)
    {
        spmc->sp_func(spmc);
    }

    tmp = (void *) calloc(size, sizeof(char));

    if(tmp == NULL)
    {
        FatalError("Unable to allocate memory!  (%lu requested, %lu in use)\n",
                size, spmc->mem_usage);
    }

    return tmp;
}

/* Guaranteed to be '\0' terminated even if truncation occurs.
 *
 * returns  SNORT_SNPRINTF_SUCCESS if successful
 * returns  SNORT_SNPRINTF_TRUNCATION on truncation
 * returns  SNORT_SNPRINTF_ERROR on error
 */
int SnortSnprintf(char *buf, size_t buf_size, const char *format, ...)
{
    va_list ap;
    int ret;

    if (buf == NULL || buf_size <= 0 || format == NULL)
        return SNORT_SNPRINTF_ERROR;

    /* zero first byte in case an error occurs with
     * vsnprintf, so buffer is null terminated with
     * zero length */
    buf[0] = '\0';
    buf[buf_size - 1] = '\0';

    va_start(ap, format);

    ret = vsnprintf(buf, buf_size, format, ap);

    va_end(ap);

    if (ret < 0)
        return SNORT_SNPRINTF_ERROR;

    if (buf[buf_size - 1] != '\0' || (size_t)ret >= buf_size)
    {
        /* result was truncated */
        buf[buf_size - 1] = '\0';
        return SNORT_SNPRINTF_TRUNCATION;
    }

    return SNORT_SNPRINTF_SUCCESS;
}

/* Appends to a given string
 * Guaranteed to be '\0' terminated even if truncation occurs.
 * 
 * returns SNORT_SNPRINTF_SUCCESS if successful
 * returns SNORT_SNPRINTF_TRUNCATION on truncation
 * returns SNORT_SNPRINTF_ERROR on error
 */
int SnortSnprintfAppend(char *buf, size_t buf_size, const char *format, ...)
{
    int str_len;
    int ret;
    va_list ap;

    if (buf == NULL || buf_size <= 0 || format == NULL)
        return SNORT_SNPRINTF_ERROR;

    str_len = SnortStrnlen(buf, buf_size);

    /* since we've already checked buf and buf_size an error
     * indicates no null termination, so just start at
     * beginning of buffer */
    if (str_len == SNORT_STRNLEN_ERROR)
    {
        buf[0] = '\0';
        str_len = 0;
    }

    buf[buf_size - 1] = '\0';

    va_start(ap, format);

    ret = vsnprintf(buf + str_len, buf_size - (size_t)str_len, format, ap);

    va_end(ap);

    if (ret < 0)
        return SNORT_SNPRINTF_ERROR;

    if (buf[buf_size - 1] != '\0' || (size_t)ret >= buf_size)
    {
        /* truncation occured */
        buf[buf_size - 1] = '\0';
        return SNORT_SNPRINTF_TRUNCATION;
    }

    return SNORT_SNPRINTF_SUCCESS;
}

/* Guaranteed to be '\0' terminated even if truncation occurs.
 *
 * Arguments:  dst - the string to contain the copy
 *             src - the string to copy from
 *             dst_size - the size of the destination buffer
 *                        including the null byte.
 *
 * returns SNORT_STRNCPY_SUCCESS if successful
 * returns SNORT_STRNCPY_TRUNCATION on truncation
 * returns SNORT_STRNCPY_ERROR on error
 *
 * Note: Do not set dst[0] = '\0' on error since it's possible that
 * dst and src are the same pointer - it will at least be null
 * terminated in any case
 */
int SnortStrncpy(char *dst, const char *src, size_t dst_size)
{
    char *ret = NULL;

    if (dst == NULL || src == NULL || dst_size <= 0)
        return SNORT_STRNCPY_ERROR;

    dst[dst_size - 1] = '\0';

    ret = strncpy(dst, src, dst_size);

    /* Not sure if this ever happens but might as
     * well be on the safe side */
    if (ret == NULL)
        return SNORT_STRNCPY_ERROR;

    if (dst[dst_size - 1] != '\0')
    {
        /* result was truncated */
        dst[dst_size - 1] = '\0';
        return SNORT_STRNCPY_TRUNCATION;
    }

    return SNORT_STRNCPY_SUCCESS;
}

char *SnortStrndup(const char *src, size_t dst_size)
{
	char *ret = SnortAlloc(dst_size + 1);
    int ret_val;

	ret_val = SnortStrncpy(ret, src, dst_size + 1);

    if(ret_val == SNORT_STRNCPY_ERROR) 
	{
		free(ret);
		return NULL;
	}

	return ret;
}

/* Determines whether a buffer is '\0' terminated and returns the
 * string length if so
 *
 * returns the string length if '\0' terminated
 * returns SNORT_STRNLEN_ERROR if not '\0' terminated
 */
int SnortStrnlen(const char *buf, int buf_size)
{
    int i = 0;

    if (buf == NULL || buf_size <= 0)
        return SNORT_STRNLEN_ERROR;

    for (i = 0; i < buf_size; i++)
    {
        if (buf[i] == '\0')
            break;
    }

    if (i == buf_size)
        return SNORT_STRNLEN_ERROR;

    return i;
}

char * SnortStrdup(const char *str)
{
    char *copy = NULL;

    if (!str)
    {
        FatalError("Unable to duplicate string: NULL!\n");
    }

    copy = strdup(str);

    if (copy == NULL)
    {
        FatalError("Unable to duplicate string: %s!\n", str);
    }

    return copy;
}

/*
 * Find first occurrence of substring in s, ignore case.
*/
const char *SnortStrcasestr(const char *s, const char *substr)
{
    char ch, nc;
    int len;

    if (!s || !*s || !substr)
        return NULL;

    if ((ch = *substr++) != 0)
    {
        ch = tolower((char)ch);
        len = strlen(substr);
        do
        {
            do
            {
                if ((nc = *s++) == 0)
                {
                    return NULL;
                }
            } while ((char)tolower((u_int8_t)nc) != ch);
        } while (strncasecmp(s, substr, len) != 0);
        s--;
    }
    return s;
}

void *SnortAlloc(unsigned long size)
{
    void *tmp;

    tmp = (void *) calloc(size, sizeof(char));

    if(tmp == NULL)
    {
        FatalError("Unable to allocate memory!  (%lu requested)\n", size);
    }

    return tmp;
}

void * SnortAlloc2(size_t size, const char *format, ...)
{
    void *tmp;

    tmp = (void *)calloc(size, sizeof(char));

    if(tmp == NULL)
    {
        va_list ap;
        char buf[STD_BUF];

        buf[STD_BUF - 1] = '\0';

        va_start(ap, format);

        vsnprintf(buf, STD_BUF - 1, format, ap);

        va_end(ap);

        FatalError("%s", buf);
    }

    return tmp;
}

/** 
 * Chroot and adjust the pv.log_dir reference 
 * 
 * @param directory directory to chroot to
 * @param logdir ptr to pv.log_dir
 */
void SetChroot(char *directory, char **logstore)
{
#ifdef WIN32
    FatalError("SetChroot() should not be called under Win32!\n");
#else
    char *absdir;
    int abslen;
    char *logdir;
    
    if(!directory || !logstore)
    {
        FatalError("Null parameter passed\n");
    }

    logdir = *logstore;

    if(logdir == NULL || *logdir == '\0')
    {
        FatalError("Null log directory\n");
    }    

    DEBUG_WRAP(DebugMessage(DEBUG_INIT,"SetChroot: %s\n",
                                       CurrentWorkingDir()););
    
    logdir = GetAbsolutePath(logdir);

    DEBUG_WRAP(DebugMessage(DEBUG_INIT, "SetChroot: %s\n",
                                       CurrentWorkingDir()));
    
    logdir = SnortStrdup(logdir);

    /* change to the directory */
    if(chdir(directory) != 0)
    {
        FatalError("SetChroot: Can not chdir to \"%s\": %s\n", directory, 
                   strerror(errno));
    }

    /* always returns an absolute pathname */
    absdir = CurrentWorkingDir();

    if(absdir == NULL)                          
    {
        FatalError("NULL Chroot found\n");
    }
    
    abslen = strlen(absdir);

    DEBUG_WRAP(DebugMessage(DEBUG_INIT, "ABS: %s %d\n", absdir, abslen););
    
    /* make the chroot call */
    if(chroot(absdir) < 0)
    {
        FatalError("Can not chroot to \"%s\": absolute: %s: %s\n",
                   directory, absdir, strerror(errno));
    }

    DEBUG_WRAP(DebugMessage(DEBUG_INIT,"chroot success (%s ->", absdir););
    DEBUG_WRAP(DebugMessage(DEBUG_INIT,"%s)\n ", CurrentWorkingDir()););
    
    /* change to "/" in the new directory */
    if(chdir("/") < 0)
    {
        FatalError("Can not chdir to \"/\" after chroot: %s\n", 
                   strerror(errno));
    }    

    DEBUG_WRAP(DebugMessage(DEBUG_INIT,"chdir success (%s)\n",
                            CurrentWorkingDir()););


    if(strncmp(absdir, logdir, strlen(absdir)))
    {
        FatalError("Absdir is not a subset of the logdir");
    }
    
    if(abslen >= strlen(logdir))
    {
        *logstore = "/";
    }
    else
    {
        *logstore = logdir + abslen;
    }

    DEBUG_WRAP(DebugMessage(DEBUG_INIT,"new logdir from %s to %s\n",
                            logdir, *logstore));

    /* install the I can't do this signal handler */
    //signal(SIGHUP, SigCantHupHandler);
#endif /* !WIN32 */
}


/**
 * Return a ptr to the absolute pathname of snort.  This memory must
 * be copied to another region if you wish to save it for later use.
 */
char *CurrentWorkingDir(void)
{
    static char buf[PATH_MAX_UTIL + 1];
    
    if(getcwd((char *) buf, PATH_MAX_UTIL) == NULL)
    {
        return NULL;
    }

    buf[PATH_MAX_UTIL] = '\0';

    return (char *) buf;
}

/**
 * Given a directory name, return a ptr to a static 
 */
char *GetAbsolutePath(char *dir)
{
    char *savedir, *dirp;
    static char buf[PATH_MAX_UTIL + 1];

    if(dir == NULL)
    {
        return NULL;
    }

    savedir = strdup(CurrentWorkingDir());

    if(savedir == NULL)
    {
        return NULL;
    }

    if(chdir(dir) < 0)
    {
        LogMessage("Can't change to directory: %s\n", dir);
        free(savedir);
        return NULL;
    }

    dirp = CurrentWorkingDir();

    if(dirp == NULL)
    {
        LogMessage("Unable to access current directory\n");
        free(savedir);
        return NULL;
    }
    else
    {
        strncpy(buf, dirp, PATH_MAX_UTIL);
        buf[PATH_MAX_UTIL] = '\0';
    }

    if(chdir(savedir) < 0)
    {
        LogMessage("Can't change back to directory: %s\n", dir);
        free(savedir);                
        return NULL;
    }

    free(savedir);
    return (char *) buf;
}


#ifndef WIN32
/* very slow sort - do not use at runtime! */
SF_LIST * SortDirectory(const char *path)
{
    SF_LIST *dir_entries;
    DIR *dir;
    struct dirent *direntry;
    int ret = 0;

    if (path == NULL)
        return NULL;

    dir_entries = sflist_new();
    if (dir_entries == NULL)
    {
        LogMessage("Could not allocate new list for directory entries\n");
        return NULL;
    }

    dir = opendir(path);
    if (dir == NULL)
    {
        LogMessage("Error opening directory: %s: %s\n",
                   path, strerror(errno));
        sflist_free_all(dir_entries, free);
        return NULL;
    }

    while ((direntry = readdir(dir)) != NULL)
    {
        char *node_entry_name, *dir_entry_name;
        SF_LNODE *node;

        dir_entry_name = SnortStrdup(direntry->d_name);

        for (node = sflist_first_node(dir_entries);
             node != NULL;
             node = sflist_next_node(dir_entries))
        {
            node_entry_name = (char *)node->ndata;
            if (strcmp(dir_entry_name, node_entry_name) < 0)
                break;
        }

        if (node == NULL)
            ret = sflist_add_tail(dir_entries, (NODE_DATA)dir_entry_name);
        else
            ret = sflist_add_before(dir_entries, node, (NODE_DATA)dir_entry_name);

        if (ret == -1)
        {
            LogMessage("Error adding directory entry to list\n");
            sflist_free_all(dir_entries, free);
            closedir(dir);
            return NULL;
        }
    }

    if (errno != 0)
    {
        LogMessage("Error reading directory: %s: %s\n",
                   path, strerror(errno));
        sflist_free_all(dir_entries, free);
        closedir(dir);
        return NULL;
    }

    closedir(dir);

    return dir_entries;
}

int GetFilesUnderDir(const char *path, SF_QUEUE *dir_queue, const char *filter)
{
    SF_LIST *dir_entries;
    char *direntry;
    int ret = 0;
    int num_files = 0;

    if ((path == NULL) || (dir_queue == NULL))
        return -1;

    dir_entries = SortDirectory(path);
    if (dir_entries == NULL)
    {
        LogMessage("Error sorting entries in directory: %s\n", path);
        return -1;
    }

    for (direntry = (char *)sflist_first(dir_entries);
         direntry != NULL;
         direntry = (char *)sflist_next(dir_entries))
    {
        char path_buf[PATH_MAX];
        struct stat file_stat;

        /* Don't look at dot files */
        if (strncmp(".", direntry, 1) == 0)
            continue;
            
        ret = SnortSnprintf(path_buf, PATH_MAX, "%s%s%s",
                            path, path[strlen(path) - 1] == '/' ? "" : "/", direntry);
        if (ret == SNORT_SNPRINTF_TRUNCATION)
        {
            LogMessage("Error copying file to buffer: Path too long\n");
            sflist_free_all(dir_entries, free);
            return -1;
        }
        else if (ret != SNORT_SNPRINTF_SUCCESS)
        {
            LogMessage("Error copying file to buffer\n");
            sflist_free_all(dir_entries, free);
            return -1;
        }

        ret = stat(path_buf, &file_stat);
        if (ret == -1)
        {
            LogMessage("Could not stat file: %s: %s\n",
                       path_buf, strerror(errno));
            sflist_free_all(dir_entries, free);
            return -1;
        }

        if (file_stat.st_mode & S_IFDIR)
        {
            ret = GetFilesUnderDir(path_buf, dir_queue, filter);
            if (ret == -1)
            {
                sflist_free_all(dir_entries, free);
                return -1;
            }

            num_files += ret;
        }
        else if (file_stat.st_mode & S_IFREG)
        {
            if ((filter == NULL) || (fnmatch(filter, direntry, 0) == 0))
            {
                char *file = SnortStrdup(path_buf);

                ret = sfqueue_add(dir_queue, (NODE_DATA)file);
                if (ret == -1)
                {
                    LogMessage("Could not append item to list: %s\n", file);
                    free(file);
                    sflist_free_all(dir_entries, free);
                    return -1;
                }

                num_files++;
            }
        }
    }

    sflist_free_all(dir_entries, free);

    return num_files;
}
#endif

/* $Id$ */
/*
** Copyright (C) 2000,2001 Christopher Cramer <cec@ee.duke.edu>
** Snort is Copyright (C) 1998-2002 Martin Roesch <roesch@sourcefire.com>
**
** Copyright (C) 2002-2008 Sourcefire, Inc.
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
**
** 7/2002 Marc Norton - added inline/optimized checksum routines
**                      these handle all hi/low endian issues
** 8/2002 Marc Norton - removed old checksum code and prototype
**
*/


/* define checksum error flags */
#define CSE_IP    0x01
#define CSE_TCP   0x02
#define CSE_UDP   0x04
#define CSE_ICMP  0x08
#define CSE_IGMP  0x10

/*
*  checksum IP  - header=20+ bytes
*
*  w - short words of data
*  blen - byte length
* 
*/
inline unsigned short in_chksum_ip(  unsigned short * w, int blen )
{
   unsigned int cksum;

   /* IP must be >= 20 bytes */
   cksum  = w[0];
   cksum += w[1];
   cksum += w[2];
   cksum += w[3];
   cksum += w[4];
   cksum += w[5];
   cksum += w[6];
   cksum += w[7];
   cksum += w[8];
   cksum += w[9];

   blen  -= 20;
   w     += 10;

   while( blen ) /* IP-hdr must be an integral number of 4 byte words */
   {
     cksum += w[0];
     cksum += w[1];
     w     += 2;
     blen  -= 4;
   }

   cksum  = (cksum >> 16) + (cksum & 0x0000ffff);
   cksum += (cksum >> 16);
 
   return (unsigned short) (~cksum);
}

/*
*  checksum tcp
*
*  h    - pseudo header - 12 bytes
*  d    - tcp hdr + payload
*  dlen - length of tcp hdr + payload in bytes
*
*/
inline unsigned short in_chksum_tcp(  unsigned short *h, unsigned short * d, int dlen )
{
   unsigned int cksum;
   unsigned short answer=0;

   /* PseudoHeader must have 12 bytes */
   cksum  = h[0];
   cksum += h[1];
   cksum += h[2];
   cksum += h[3];
   cksum += h[4];
   cksum += h[5];

   /* TCP hdr must have 20 hdr bytes */
   cksum += d[0];
   cksum += d[1];
   cksum += d[2];
   cksum += d[3];
   cksum += d[4];
   cksum += d[5];
   cksum += d[6];
   cksum += d[7];
   cksum += d[8];
   cksum += d[9];

   dlen  -= 20; /* bytes   */
   d     += 10; /* short's */ 

   while(dlen >=32)
   {
     cksum += d[0];
     cksum += d[1];
     cksum += d[2];
     cksum += d[3];
     cksum += d[4];
     cksum += d[5];
     cksum += d[6];
     cksum += d[7];
     cksum += d[8];
     cksum += d[9];
     cksum += d[10];
     cksum += d[11];
     cksum += d[12];
     cksum += d[13];
     cksum += d[14];
     cksum += d[15];
     d     += 16;
     dlen  -= 32;
   }

   while(dlen >=8)  
   {
     cksum += d[0];
     cksum += d[1];
     cksum += d[2];
     cksum += d[3];
     d     += 4;   
     dlen  -= 8;
   }

   while(dlen > 1)
   {
     cksum += *d++;
     dlen  -= 2;
   }

   if( dlen == 1 ) 
   { 
    /* printf("new checksum odd byte-packet\n"); */
    *(unsigned char*)(&answer) = (*(unsigned char*)d);

    /* cksum += (u_int16_t) (*(u_int8_t*)d); */
     
     cksum += answer;
   }
   
   cksum  = (cksum >> 16) + (cksum & 0x0000ffff);
   cksum += (cksum >> 16);
 
   return (unsigned short)(~cksum);
}
/*
*  checksum tcp for IPv6.
*
*  h    - pseudo header - 12 bytes
*  d    - tcp hdr + payload
*  dlen - length of tcp hdr + payload in bytes
*
*/
inline unsigned short in_chksum_tcp6(  unsigned short *h, unsigned short * d, int dlen )
{
   unsigned int cksum;
   unsigned short answer=0;

   /* PseudoHeader must have 36 bytes */
   cksum  = h[0];
   cksum += h[1];
   cksum += h[2];
   cksum += h[3];
   cksum += h[4];
   cksum += h[5];
   cksum += h[6];
   cksum += h[7];
   cksum += h[8];
   cksum += h[9];
   cksum += h[10];
   cksum += h[11];
   cksum += h[12];
   cksum += h[13];
   cksum += h[14];
   cksum += h[15];
   cksum += h[16];
   cksum += h[17];

   /* TCP hdr must have 20 hdr bytes */
   cksum += d[0];
   cksum += d[1];
   cksum += d[2];
   cksum += d[3];
   cksum += d[4];
   cksum += d[5];
   cksum += d[6];
   cksum += d[7];
   cksum += d[8];
   cksum += d[9];

   dlen  -= 20; /* bytes   */
   d     += 10; /* short's */ 

   while(dlen >=32)
   {
     cksum += d[0];
     cksum += d[1];
     cksum += d[2];
     cksum += d[3];
     cksum += d[4];
     cksum += d[5];
     cksum += d[6];
     cksum += d[7];
     cksum += d[8];
     cksum += d[9];
     cksum += d[10];
     cksum += d[11];
     cksum += d[12];
     cksum += d[13];
     cksum += d[14];
     cksum += d[15];
     d     += 16;
     dlen  -= 32;
   }

   while(dlen >=8)  
   {
     cksum += d[0];
     cksum += d[1];
     cksum += d[2];
     cksum += d[3];
     d     += 4;   
     dlen  -= 8;
   }

   while(dlen > 1)
   {
     cksum += *d++;
     dlen  -= 2;
   }

   if( dlen == 1 ) 
   { 
    /* printf("new checksum odd byte-packet\n"); */
    *(unsigned char*)(&answer) = (*(unsigned char*)d);

    /* cksum += (u_int16_t) (*(u_int8_t*)d); */
     
     cksum += answer;
   }
   
   cksum  = (cksum >> 16) + (cksum & 0x0000ffff);
   cksum += (cksum >> 16);
 
   return (unsigned short)(~cksum);
}

/*
*  checksum udp
*
*  h    - pseudo header - 12 bytes
*  d    - udp hdr + payload
*  dlen - length of payload in bytes
*
*/
inline unsigned short in_chksum_udp6(  unsigned short *h, unsigned short * d, int dlen )
{
   unsigned int cksum;
   unsigned short answer=0;

   /* PseudoHeader must have  12 bytes */
   cksum  = h[0];
   cksum += h[1];
   cksum += h[2];
   cksum += h[3];
   cksum += h[4];
   cksum += h[5];
   cksum += h[6];
   cksum += h[7];
   cksum += h[8];
   cksum += h[9];
   cksum += h[10];
   cksum += h[11];
   cksum += h[12];
   cksum += h[13];
   cksum += h[14];
   cksum += h[15];
   cksum += h[16];
   cksum += h[17];

   /* UDP must have 8 hdr bytes */
   cksum += d[0];
   cksum += d[1];
   cksum += d[2];
   cksum += d[3];

   dlen  -= 8; /* bytes   */
   d     += 4; /* short's */ 

   while(dlen >=32) 
   {
     cksum += d[0];
     cksum += d[1];
     cksum += d[2];
     cksum += d[3];
     cksum += d[4];
     cksum += d[5];
     cksum += d[6];
     cksum += d[7];
     cksum += d[8];
     cksum += d[9];
     cksum += d[10];
     cksum += d[11];
     cksum += d[12];
     cksum += d[13];
     cksum += d[14];
     cksum += d[15];
     d     += 16;
     dlen  -= 32;
   }

   while(dlen >=8)
   {
     cksum += d[0];
     cksum += d[1];
     cksum += d[2];
     cksum += d[3];
     d     += 4;   
     dlen  -= 8;
   }

   while(dlen > 1) 
   {
     cksum += *d++;
     dlen  -= 2;
   }

   if( dlen == 1 ) 
   { 
     *(unsigned char*)(&answer) = (*(unsigned char*)d);
     cksum += answer;
   }
   
   cksum  = (cksum >> 16) + (cksum & 0x0000ffff);
   cksum += (cksum >> 16);
 
   return (unsigned short)(~cksum);
}



 inline unsigned short in_chksum_udp(  unsigned short *h, unsigned short * d, int dlen )
{
   unsigned int cksum;
   unsigned short answer=0;

   /* PseudoHeader must have 36 bytes */
   cksum  = h[0];
   cksum += h[1];
   cksum += h[2];
   cksum += h[3];
   cksum += h[4];
   cksum += h[5];

   /* UDP must have 8 hdr bytes */
   cksum += d[0];
   cksum += d[1];
   cksum += d[2];
   cksum += d[3];

   dlen  -= 8; /* bytes   */
   d     += 4; /* short's */ 

   while(dlen >=32) 
   {
     cksum += d[0];
     cksum += d[1];
     cksum += d[2];
     cksum += d[3];
     cksum += d[4];
     cksum += d[5];
     cksum += d[6];
     cksum += d[7];
     cksum += d[8];
     cksum += d[9];
     cksum += d[10];
     cksum += d[11];
     cksum += d[12];
     cksum += d[13];
     cksum += d[14];
     cksum += d[15];
     d     += 16;
     dlen  -= 32;
   }

   while(dlen >=8)
   {
     cksum += d[0];
     cksum += d[1];
     cksum += d[2];
     cksum += d[3];
     d     += 4;   
     dlen  -= 8;
   }

   while(dlen > 1) 
   {
     cksum += *d++;
     dlen  -= 2;
   }

   if( dlen == 1 ) 
   { 
     *(unsigned char*)(&answer) = (*(unsigned char*)d);
     cksum += answer;
   }
   
   cksum  = (cksum >> 16) + (cksum & 0x0000ffff);
   cksum += (cksum >> 16);
 
   return (unsigned short)(~cksum);
}

/*
*  checksum icmp
*/
inline unsigned short in_chksum_icmp( unsigned short * w, int blen )
{
  unsigned  short answer=0;
  unsigned int cksum = 0;

  while(blen >=32) 
  {
     cksum += w[0];
     cksum += w[1];
     cksum += w[2];
     cksum += w[3];
     cksum += w[4];
     cksum += w[5];
     cksum += w[6];
     cksum += w[7];
     cksum += w[8];
     cksum += w[9];
     cksum += w[10];
     cksum += w[11];
     cksum += w[12];
     cksum += w[13];
     cksum += w[14];
     cksum += w[15];
     w     += 16;
     blen  -= 32;
  }

  while(blen >=8) 
  {
     cksum += w[0];
     cksum += w[1];
     cksum += w[2];
     cksum += w[3];
     w     += 4;
     blen  -= 8;
  }

  while(blen > 1) 
  {
     cksum += *w++;
     blen  -= 2;
  }

  if( blen == 1 ) 
  {
    *(unsigned char*)(&answer) = (*(unsigned char*)w);
    cksum += answer;
  }

  cksum  = (cksum >> 16) + (cksum & 0x0000ffff);
  cksum += (cksum >> 16);


  return (unsigned short)(~cksum);
}

/*
*  checksum icmp6
*/
inline unsigned short in_chksum_icmp6( unsigned short * w, int blen )
{
// XXX ICMP6 CHECKSUM NOT YET IMPLEMENTED
  return 0;
#if 0
  unsigned  short answer=0;
  unsigned int cksum = 0;

  while(blen >=32) 
  {
     cksum += w[0];
     cksum += w[1];
     cksum += w[2];
     cksum += w[3];
     cksum += w[4];
     cksum += w[5];
     cksum += w[6];
     cksum += w[7];
     cksum += w[8];
     cksum += w[9];
     cksum += w[10];
     cksum += w[11];
     cksum += w[12];
     cksum += w[13];
     cksum += w[14];
     cksum += w[15];
     w     += 16;
     blen  -= 32;
  }

  while(blen >=8) 
  {
     cksum += w[0];
     cksum += w[1];
     cksum += w[2];
     cksum += w[3];
     w     += 4;
     blen  -= 8;
  }

  while(blen > 1) 
  {
     cksum += *w++;
     blen  -= 2;
  }

  if( blen == 1 ) 
  {
    *(unsigned char*)(&answer) = (*(unsigned char*)w);
    cksum += answer;
  }

  cksum  = (cksum >> 16) + (cksum & 0x0000ffff);
  cksum += (cksum >> 16);


  return (unsigned short)(~cksum);
#endif
}


/* $Id$ */
/*
 * Copyright (C) 2002-2008 Sourcefire, Inc.
 * 
 * Author(s):  Andrew R. Baker <andrewb@snort.org>
 *             Martin Roesch   <roesch@sourcefire.com>
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

/* includes */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifndef WIN32
#include <netdb.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include "util.h"
//#include "parser.h"
#include "debug.h"

#ifdef SUP_IP6
#include "ipv6_port.h"
#else


u_long netmasks[33]; 
extern char *file_name;     /* current rules file being processed */
extern int line_num;        /* current rules file line */


IpAddrSet *IpAddrSetCreate()
{
    IpAddrSet *tmp;

    tmp = (IpAddrSet *) SnortAlloc(sizeof(IpAddrSet));

    return tmp;
}


void IpAddrSetDestroy(IpAddrSet *ipAddrSet)
{
    IpAddrNode *node, *tmp;

    if(!ipAddrSet) 
        return;

    node = ipAddrSet->iplist;

    while(node)
    {
        tmp = node;
        node = node->next;
        free(tmp);
    }

    node = ipAddrSet->neg_iplist;

    while(node)
    {
        tmp = node;
        node = node->next;
        free(tmp);
    }
}

static char buffer[1024];

void IpAddrSetPrint(char *prefix, IpAddrSet *ipAddrSet)
{
    IpAddrNode *iplist, *neglist;
    struct in_addr in;
    int ret;

    if(!ipAddrSet) return;

    iplist = ipAddrSet->iplist;
    neglist = ipAddrSet->neg_iplist;

    while(iplist) 
    {
        buffer[0] = '\0';

        in.s_addr = iplist->ip_addr;
        ret = SnortSnprintfAppend(buffer, sizeof(buffer), "%s/", inet_ntoa(in));
        if (ret != SNORT_SNPRINTF_SUCCESS)
            return;

        in.s_addr = iplist->netmask;
        ret = SnortSnprintfAppend(buffer, sizeof(buffer), "%s", inet_ntoa(in));
        if (ret != SNORT_SNPRINTF_SUCCESS)
            return;

        if (prefix)
            LogMessage("%s%s\n", prefix, buffer);
        else
            LogMessage("%s\n", buffer);

        iplist = iplist->next;
       
    }

    while(neglist) 
    {
        buffer[0] = '\0';

        in.s_addr = neglist->ip_addr;
        ret = SnortSnprintfAppend(buffer, sizeof(buffer), "NOT %s/", inet_ntoa(in));
        if (ret != SNORT_SNPRINTF_SUCCESS)
            return;

        in.s_addr = neglist->netmask;
        ret = SnortSnprintfAppend(buffer, sizeof(buffer), "%s", inet_ntoa(in));
        if (ret != SNORT_SNPRINTF_SUCCESS)
            return;

        if (prefix)
            LogMessage("%s%s\n", prefix, buffer);
        else
            LogMessage("%s\n", buffer);

        neglist = neglist->next;
    }
}

IpAddrSet *IpAddrSetCopy(IpAddrSet *ipAddrSet)
{
    IpAddrSet *newIpAddrSet;
    IpAddrNode *current;
    IpAddrNode *iplist, *neglist;
    IpAddrNode *prev = NULL;

    if(!ipAddrSet) return NULL;

    newIpAddrSet = (IpAddrSet *)calloc(sizeof(IpAddrSet), 1);
    if(!newIpAddrSet) 
    {
        goto failed;
    }

    iplist = ipAddrSet->iplist;
    neglist = ipAddrSet->neg_iplist;

    while(iplist)
    {
        current = (IpAddrNode *)malloc(sizeof(IpAddrNode));
        if (!current)
        {
            goto failed;
        }

        if(!newIpAddrSet->iplist)
            newIpAddrSet->iplist = current;
        
        current->ip_addr = iplist->ip_addr;
        current->netmask = iplist->netmask;
        current->addr_flags = iplist->addr_flags;
        current->next = NULL;

        if(prev)
            prev->next = current;

        prev = current;

        iplist = iplist->next;
    }

    while(neglist)
    {
        current = (IpAddrNode *)malloc(sizeof(IpAddrNode));
        if (!current)
        {
            goto failed;
        }
        
        if(!newIpAddrSet->neg_iplist)
            newIpAddrSet->neg_iplist = current;

        current->ip_addr = neglist->ip_addr;
        current->netmask = neglist->netmask;
        current->addr_flags = neglist->addr_flags;
        current->next = NULL;

        if(prev)
            prev->next = current;

        prev = current;

        neglist = neglist->next;
    }

    return newIpAddrSet;

failed:
    if(newIpAddrSet)
        IpAddrSetDestroy(newIpAddrSet);
    return NULL; /* XXX ENOMEM */
}


/* XXX: legacy support function */
/*
 * Function: ParseIP(char *, IpAddrSet *)
 *
 * Purpose: Convert a supplied IP address to it's network order 32-bit long
 *          value.  Also convert the CIDR block notation into a real
 *          netmask.
 *
 * Arguments: char *addr  => address string to convert
 *            IpAddrSet * =>
 *            
 *
 * Returns: 0 for normal addresses, 1 for an "any" address
 */
int ParseIP(char *paddr, IpAddrSet *ias, int negate) //, IpAddrNode *node)
{
    char **toks;        /* token dbl buffer */
    int num_toks;       /* number of tokens found by mSplit() */
    int cidr = 1;       /* is network expressed in CIDR format */
    int nmask = -1;     /* netmask temporary storage */
    char *addr;         /* string to parse, eventually a
                         * variable-contents */
    struct hostent *host_info;  /* various struct pointers for stuff */
    struct sockaddr_in sin; /* addr struct */
    char broadcast_addr_set = 0;

    IpAddrNode *address_data = (IpAddrNode*)SnortAlloc(sizeof(IpAddrNode));

    if(!paddr || !ias) 
        return 1;

    addr = paddr;

    if(*addr == '!')
    {
        negate = !negate;
//        address_data->addr_flags |= EXCEPT_IP;

        addr++;  /* inc past the '!' */
    }

    /* check for wildcards */
    if(!strcasecmp(addr, "any"))
    {
        if(negate) 
        {
            //FatalError("%s(%d) => !any is not allowed\n", file_name, file_line);
        }
    
        /* Make first node 0, which matches anything */
        if(!ias->iplist) 
        {
            ias->iplist = (IpAddrNode*)SnortAlloc(sizeof(IpAddrNode));
        }
        ias->iplist->ip_addr = 0;
        ias->iplist->netmask = 0;

        free(address_data);

        return 1;
    }
    /* break out the CIDR notation from the IP address */
    toks = mSplit(addr, "/", 2, &num_toks, 0);

    /* "/" was not used as a delimeter, try ":" */
    if(num_toks == 1)
    {
        mSplitFree(&toks, num_toks);
        toks = mSplit(addr, ":", 2, &num_toks, 0);
    }

    /*
     * if we have a mask spec and it is more than two characters long, assume
     * it is netmask format
     */
    if((num_toks > 1) && strlen(toks[1]) > 2)
    {
        cidr = 0;
    }

    switch(num_toks)
    {
        case 1:
            address_data->netmask = netmasks[32];
            break;

        case 2:
            if(cidr)
            {
                /* convert the CIDR notation into a real live netmask */
                nmask = atoi(toks[1]);

                /* it's pain to differ whether toks[1] is correct if netmask */
                /* is /0, so we deploy some sort of evil hack with isdigit */

                if(!isdigit((int) toks[1][0]))
                    nmask = -1;

                /* if second char is != '\0', it must be a digit
                 * by Daniel B. Cid, dcid@sourcefire.com
                 */ 
                if((toks[1][1] != '\0')&&(!isdigit((int) toks[1][1]) ))
                    nmask = -1;
                
                if((nmask > -1) && (nmask < 33))
                {
                    address_data->netmask = netmasks[nmask];
                }
                else
                {
                    //FatalError("ERROR %s(%d): Invalid CIDR block for IP addr " "%s\n", file_name, file_line, addr);
                           
                }
            }
            else
            {
                /* convert the netmask into its 32-bit value */

                /* broadcast address fix from 
                 * Steve Beaty <beaty@emess.mscd.edu> 
                 */

                /*
                 * if the address is the (v4) broadcast address, inet_addr *
                 * returns -1 which usually signifies an error, but in the *
                 * broadcast address case, is correct.  we'd use inet_aton() *
                 * here, but it's less portable.
                 */
                if(!strncmp(toks[1], "255.255.255.255", 15))
                {
                    address_data->netmask = INADDR_BROADCAST;
                }
                else if((address_data->netmask = inet_addr(toks[1])) == -1)
                {
                    //FatalError("ERROR %s(%d): Unable to parse rule netmask " "(%s)\n", file_name, file_line, toks[1]);
                }
                /* Set nmask so we don't try to do a host lookup below.
                 * The value of 0 is irrelevant. */
                nmask = 0;
            }
            break;

        default:
            //FatalError("ERROR %s(%d) => Unrecognized IP address/netmask %s\n", file_name, file_line, addr);
            break;
    }
    sin.sin_addr.s_addr = inet_addr(toks[0]);

#ifndef WORDS_BIGENDIAN
    /*
     * since PC's store things the "wrong" way, shuffle the bytes into the
     * right order.  Non-CIDR netmasks are already correct.
     */
    if(cidr)
    {
        address_data->netmask = htonl(address_data->netmask);
    }
#endif
    /* broadcast address fix from Steve Beaty <beaty@emess.mscd.edu> */
    /* Changed location */
    if(!strncmp(toks[0], "255.255.255.255", 15))
    {
        address_data->ip_addr = INADDR_BROADCAST;
        broadcast_addr_set = 1;
    }
    else if (nmask == -1)
    {
        /* Try to do a host lookup if the address didn't
         * convert to a valid IP and there were not any
         * mask bits specified (CIDR or dot notation). */
        if(sin.sin_addr.s_addr == INADDR_NONE)
        {
            /* get the hostname and fill in the host_info struct */
            host_info = gethostbyname(toks[0]);
            if (host_info)
            {
                /* protecting against malicious DNS servers */
                if(host_info->h_length <= sizeof(sin.sin_addr))
                {
                    bcopy(host_info->h_addr, (char *) &sin.sin_addr, host_info->h_length);
                }
                else
                {
                    bcopy(host_info->h_addr, (char *) &sin.sin_addr, sizeof(sin.sin_addr));
                }
            }
            /* Using h_errno */
            else if(h_errno == HOST_NOT_FOUND)
            /*else if((sin.sin_addr.s_addr = inet_addr(toks[0])) == INADDR_NONE)*/
            {
                //FatalError("ERROR %s(%d): Couldn't resolve hostname %s\n", file_name, file_line, toks[0]);
            }
        }
        else
        {
            /* It was a valid IP address with no netmask specified. */
            /* Noop */
        }
    }
    else
    {
        if(sin.sin_addr.s_addr == INADDR_NONE)
        {
            /* It was not a valid IP address but had a valid netmask. */
            //FatalError("ERROR %s(%d): Rule IP addr (%s) didn't translate\n", file_name, file_line, toks[0]);
        }
    }

    /* Only set this if we haven't set it above as 255.255.255.255 */
    if (!broadcast_addr_set)
    {
        address_data->ip_addr = ((u_long) (sin.sin_addr.s_addr) &
            (address_data->netmask));
    }
    mSplitFree(&toks, num_toks);

    /* Add new IP address to address set */
    if(!negate) 
    {
        IpAddrNode *idx;

        if(!ias->iplist) 
        {
            ias->iplist = address_data;
        }
        else 
        {
            /* Get to the end of the list */
            for(idx = ias->iplist; idx->next; idx=idx->next) ;

            idx->next = address_data;
        }
    }
    else
    {
        IpAddrNode *idx;

        if(!ias->neg_iplist) 
        {
            ias->neg_iplist = address_data;
        }
        else 
        {
            /* Get to the end of the list */
            for(idx = ias->neg_iplist; idx->next; idx=idx->next) ;

            idx->next = address_data;
        }

        address_data->addr_flags |= EXCEPT_IP;
    }
    
    return 0;
} 


void IpAddrSetBuild(char *addr, IpAddrSet *ret, int neg_list) 
{
    char *tok, *end, *tmp=NULL;
    int neg_ip;

    while(*addr) 
    {
        /* Skip whitespace and leading commas */
        for(; *addr && (isspace((int)*addr) || *addr == ','); addr++) ;

        /* Handle multiple negations (such as if someone negates variable that
         * contains a negated IP */
        neg_ip = 0;
        for(; *addr == '!'; addr++) 
             neg_ip = !neg_ip;

        /* Find end of this token */
        for(end = addr+1; 
           *end && !isspace((int)*end) && *end != ']' && *end != ',';
            end++) ;

        tok = SnortStrndup(addr, end - addr);

        if(!tok)    
        {
            //FatalError("%s(%d) => Failed to allocate memory for parsing '%s'\n", file_name, file_line, addr);
        }

        if(*addr == '[') 
        {
            int brack_count = 0;
            char *list_tok;
    
            /* Find corresponding ending bracket */
            for(end = addr; *end; end++) 
            {
                if(*end == '[') 
                    brack_count++;
                else if(*end == ']')
                    brack_count--;
    
                if(!brack_count)
                    break;
            }
    
            if(!*end) 
            {
                //FatalError("%s(%d) => Unterminated IP List '%s'\n", file_name, file_line, addr);
            }
        
            addr++;

            list_tok = SnortStrndup(addr, end - addr);

            if(!list_tok)    
            {
                //FatalError("%s(%d) => Failed to allocate memory for parsing '%s'\n", file_name, file_line, addr);
            }

            IpAddrSetBuild(list_tok, ret, neg_ip ^ neg_list);
            free(list_tok);
        }
        else if(*addr == '$') 
        {
            //if((tmp = VarGet(tok + 1)) == NULL) wxh
            {
               // FatalError("%s(%d) => Undefined variable %s\n", file_name, file_line, addr);
            }
            
            IpAddrSetBuild(tmp, ret, neg_list ^ neg_ip); 
        }
        else if(*addr == ']')
        {
            if(!(*(addr+1))) 
            {
                /* Succesfully reached the end of this list */
                free(tok);
                return;
            }

            //FatalError("%s(%d) => Mismatched bracket in '%s'\n", file_name, file_line, addr);
        }
        else 
        {
            /* Skip leading commas */
            for(; *addr && (*addr == ',' || isspace((int)*addr)); addr++) ;

            ParseIP(tok, ret, neg_list ^ neg_ip);

           // if(ret->iplist && !ret->iplist->ip_addr && !ret->iplist->netmask) 
            //     ret->iplist->addr_flags |= ANY_SRC_IP; wxh
                
            /* Note: the neg_iplist is not checked for '!any' here since
             * ParseIP should have already FatalError'ed on it. */
        }
        
        free(tok);

        if(*end)
            addr = end + 1;   
        else break;
    }

    return;
}
#endif


IpAddrSet *IpAddrSetParse(char *addr) 
{
    IpAddrSet *ret;
#ifdef SUP_IP6
    int ret_code;
#endif

    DEBUG_WRAP(DebugMessage(DEBUG_CONFIGRULES,"Got address string: %s\n", 
                addr););

    ret = (IpAddrSet*)SnortAlloc(sizeof(IpAddrSet));

#ifdef SUP_IP6 
    if((ret_code = sfvt_add_to_var(vartable, ret, addr)) != SFIP_SUCCESS) 
    {
        if(ret_code == SFIP_LOOKUP_FAILURE)
            FatalError("%s(%d) => Undefined variable in the string: %s\n",
                file_name, file_line, addr);
        else if(ret_code == SFIP_CONFLICT)
            FatalError("%s(%d) => Negated IP ranges that equal to or are"
                " more-specific than non-negated ranges are not allowed."
                " Consider inverting the logic: %s.\n", 
                file_name, file_line, addr);
        else
            FatalError("%s(%d) => Unable to process the IP address: %s\n",
                file_name, file_line, addr);
    }
#else

    IpAddrSetBuild(addr, ret, 0);

#endif

    return ret;
}

#ifndef SUP_IP6
int IpAddrSetContains(IpAddrSet *ias, struct in_addr test_addr)
{
    IpAddrNode *index;
    u_int32_t raw_addr = test_addr.s_addr;
    int match = 0;

    if(!ias)
    {
        DEBUG_WRAP(DebugMessage(DEBUG_ALL,"Null IP address set!\n"););
        return 0;
    }
    if(!ias->iplist) 
        match = 1;

    for(index = ias->iplist; index != NULL; index = index->next)
    {
        if(index->ip_addr == (raw_addr & index->netmask)) 
        {
            match = 1;
            break;
        }
    }   

    if(!match) 
        return 0;

    if(!ias->neg_iplist) 
        return 1;

    for(index = ias->neg_iplist; index != NULL; index = index->next)
    {
        if(index->ip_addr == (raw_addr & index->netmask)) 
            return 0;
    }

    return 1;
}
#endif // SUP_IP6
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
 * sf_ip.c
 * 11/17/06
 *
 * Library for managing IP addresses of either v6 or v4 families.  
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h> /* For ceil */

#ifdef TESTER
#define FatalError printf
#endif

/* Support function .. but could see some external uses */
static INLINE int sfip_length(sfip_t *ip) {
    ARG_CHECK1(ip, 0);

    if(sfip_family(ip) == AF_INET) return 4;
    return 16;
}

/* Support function */
static INLINE int sfip_str_to_fam(char *str) {
    ARG_CHECK1(str, 0);

    if(strchr(str,(int)':')) return AF_INET6;
    if(strchr(str,(int)'.')) return AF_INET;
    return AF_UNSPEC;

}

/* Place-holder allocation incase we want to do something more indepth later */
static INLINE sfip_t *_sfip_alloc() {
    /* Note: using calloc here instead of SnortAlloc since the dynamic libs 
     * can't presently resolve SnortAlloc */
    return (sfip_t*)calloc(sizeof(sfip_t), 1); 
}

/* Masks off 'val' bits from the IP contained within 'ip' */
static INLINE int sfip_cidr_mask(sfip_t *ip, int val) {
    int i;
    unsigned int mask = 0; 
    unsigned int *p;
    int index = (int)ceil(val / 32.0) - 1;
   
    ARG_CHECK1(ip, SFIP_ARG_ERR);

    p = ip->ip32;

    if( val < 0 ||
        ((sfip_family(ip) == AF_INET6) && val > 128) ||
        ((sfip_family(ip) == AF_INET) && val > 32) ) {
        return SFIP_ARG_ERR;
    }
    
    /* Build the netmask by converting "val" into 
     * the corresponding number of bits that are set */
    for(i = 0; i < 32- (val - (index * 32)); i++)
        mask = (mask<<1) + 1;

    p[index] = htonl((ntohl(p[index]) & ~mask));

    index++;

    /* 0 off the rest of the IP */
    for( ; index<4; index++) p[index] = 0;

    return SFIP_SUCCESS;
}

/* Allocate IP address from a character array describing the IP */
sfip_t *sfip_alloc(char *ip, SFIP_RET *status) {
    int tmp;
    sfip_t *ret;
   
    if(!ip) {
        if(status)
            *status = SFIP_ARG_ERR;
        return NULL;
    }

    if((ret = _sfip_alloc()) == NULL) {
        if(status) 
            *status = SFIP_ALLOC_ERR;
        return NULL;
    }
    
    if( (tmp = sfip_pton(ip, ret)) != SFIP_SUCCESS) {
        if(status) 
            *status = tmp;

        sfip_free(ret);
        return NULL;
    }

    if(status) 
        *status = SFIP_SUCCESS;

    return ret;
}

/* Allocate IP address from an array of 8 byte integers */
sfip_t *sfip_alloc_raw(void *ip, int family, SFIP_RET *status) {
    sfip_t *ret;

    if(!ip) {
        if(status)
            *status = SFIP_ARG_ERR;
        return NULL;
    }

    if((ret = _sfip_alloc()) == NULL) {
        if(status)
            *status = SFIP_ALLOC_ERR;
        return NULL;
    }

    ret->bits = (family==AF_INET?32:128);
    ret->family = family;
    /* XXX Replace with appropriate "high speed" copy */
    memcpy(ret->ip8, ip, ret->bits/8);

    if(status)
        *status = SFIP_SUCCESS;

    return ret;
}

/* Support function for _netmask_str_to_bit_count */
static INLINE int _count_bits(unsigned int val) {
    unsigned int count; 

    for (count = 0; val; count++) {
        val &= val - 1;
    }

    return count;
}

/* Support function for sfip_pton.  Used for converting a netmask string
 * into a number of bits to mask off */
static INLINE int _netmask_str_to_bit_count(char *mask, int family) {
    u_int32_t buf[4];
    int bits;

    /* XXX 
     * Mask not validated.  
     * Only sfip_pton should be using this function, and using it safely. 
     * XXX */

    if(inet_pton(family, mask, buf) < 1)
        return -1;

    if(family == AF_INET)
        return _count_bits(buf[0]);

     bits =  _count_bits(buf[0]);
     bits += _count_bits(buf[1]);
     bits += _count_bits(buf[2]);
     bits += _count_bits(buf[3]);
    
     return bits;
}

/* Parses "src" and stores results in "dst" */
SFIP_RET sfip_pton(char *src, sfip_t *dst) {
    char *mask;
    char *sfip_buf;
    char *ip;
    int bits;

    if(!dst || !src) 
        return SFIP_ARG_ERR;
            
    if((sfip_buf = strdup(src)) == NULL) 
        return SFIP_ALLOC_ERR;

    ip = sfip_buf;
    dst->family = sfip_str_to_fam(src);

    /* skip whitespace or opening bracket */
    while(isspace((int)*ip) || *ip == '[')
        ip++;

    /* check for and extract a mask in CIDR form */
    if( (mask = strchr(ip, (int)'/')) != NULL ) {
        /* NULL out this character so inet_pton will see the 
         * correct ending to the IP string */
        *mask = 0;
        mask++;

        /* verify a leading digit */
        if(((dst->family == AF_INET6) && !isxdigit((int)*mask)) ||
           ((dst->family == AF_INET) && !isdigit((int)*mask))) {
            free(sfip_buf);                          
            return SFIP_CIDR_ERR;
        }

        /* Check if there's a netmask here instead of the number of bits */
        if(strchr(mask, (int)'.') || strchr(mask, (int)':')) 
            bits = _netmask_str_to_bit_count(mask, sfip_str_to_fam(mask));
        else
            bits = atoi(mask);
    }
    /* We've already skipped the leading whitespace, if there is more 
     * whitespace, then there's probably a netmask specified after it. */
    else if( (mask = strchr(ip, (int)' ')) != NULL ||
            /* If this is IPv4, ia ':' may used specified to indicate a netmask */
             (dst->family == AF_INET && (mask = strchr(ip, (int)':')) != NULL) ) {

        *mask = 0;  /* Now the IP will end at this point */

        /* skip whitespace */
        do { 
            mask++;
        } while(isspace((int)*mask));

        /* Make sure we're either looking at a valid digit, or a leading
         * colon, such as can be the case with IPv6 */
        if((dst->family == AF_INET && isdigit((int)*mask)) ||
           (dst->family == AF_INET6 && (isxdigit((int)*mask) || *mask == ':'))) { 
            bits = _netmask_str_to_bit_count(mask, sfip_str_to_fam(mask));
        } 
        /* No netmask */
        else { 
            if(dst->family == AF_INET) bits = 32;
            else bits = 128;        
        }
    }
    /* No netmask */
    else {
        if(dst->family == AF_INET) bits = 32;
        else bits = 128;        
    }

    if(inet_pton(dst->family, ip, dst->ip8) < 1) {
        free(sfip_buf);                          
        return SFIP_INET_PARSE_ERR;
    }

    /* Store mask */
    dst->bits = bits;

    /* Apply mask */
    if(sfip_cidr_mask(dst, bits) != SFIP_SUCCESS) {
        free(sfip_buf);
        return SFIP_INVALID_MASK;
    }
    
    free(sfip_buf);
    return SFIP_SUCCESS;
}

/* Sets existing IP, "dst", to be source IP, "src" */
SFIP_RET sfip_set_raw(sfip_t *dst, void *src, int family) {
    
    ARG_CHECK3(dst, src, dst->ip32, SFIP_ARG_ERR);

    dst->family = family;

    if(family == AF_INET) {
        dst->ip32[0] = *(u_int32_t*)src;
        memset(&dst->ip32[1], 0, 12);
        dst->bits = 32;
    } else if(family == AF_INET6) {
        memcpy(dst->ip8, src, 16);
        dst->bits = 128;
    } else {
        return SFIP_ARG_ERR;
    }
    
    return SFIP_SUCCESS;
}

/* Sets existing IP, "dst", to be source IP, "src" */
SFIP_RET sfip_set_ip(sfip_t *dst, sfip_t *src) {
    ARG_CHECK2(dst, src, SFIP_ARG_ERR);

    dst->family = src->family;
    dst->bits = src->bits;
    dst->ip32[0] = src->ip32[0];
    dst->ip32[1] = src->ip32[1];
    dst->ip32[2] = src->ip32[2];
    dst->ip32[3] = src->ip32[3];

    return SFIP_SUCCESS;
}

/* Obfuscates an IP
 * Makes 'ip': ob | (ip & mask) */
void sfip_obfuscate(sfip_t *ob, sfip_t *ip) {
    unsigned int *ob_p, *ip_p;
    int index, i;
    unsigned int mask = 0;

    if(!ob || !ip)
        return;

    ob_p = ob->ip32;
    ip_p = ip->ip32;

    /* Build the netmask by converting "val" into 
     * the corresponding number of bits that are set */
    index = (int)ceil(ob->bits / 32.0) - 1;

    for(i = 0; i < 32- (ob->bits - (index * 32)); i++)
        mask = (mask<<1) + 1;

    /* Note: The old-Snort obfuscation code uses !mask for masking.
     * hence, this code uses the same algorithm as sfip_cidr_mask
     * except the mask below is not negated. */
    ip_p[index] = htonl((ntohl(ip_p[index]) & mask));

    index++;

    /* 0 off the rest of the IP */
    for( ; index<4; index++) ip_p[index] = 0;

    /* OR remaining pieces */
    ip_p[0] |= ob_p[0];
    ip_p[1] |= ob_p[1];
    ip_p[2] |= ob_p[2];
    ip_p[3] |= ob_p[3];
}


/* Check if ip is contained within the network specified by net */ 
/* Returns SFIP_EQUAL if so.  
 * XXX sfip_contains assumes that "ip" is 
 *      not less-specific than "net" XXX
*/
SFIP_RET sfip_contains(sfip_t *net, sfip_t *ip) {
    unsigned int bits, mask, temp, i;
    int net_fam, ip_fam;
    unsigned int *p1, *p2;

    /* SFIP_CONTAINS is returned here due to how IpAddrSetContains 
     * handles zero'ed IPs" */
    ARG_CHECK2(net, ip, SFIP_CONTAINS);

    bits = sfip_bits(net);
    net_fam = sfip_family(net);
    ip_fam = sfip_family(ip);

    /* If the families are mismatched, check if we're really comparing
     * an IPv4 with a mapped IPv4 (in IPv6) address. */
    if(net_fam != ip_fam) {
        if(net_fam != AF_INET && !sfip_ismapped(ip))
            return SFIP_NOT_CONTAINS;

        /* Both are really IPv4.  Only compare last 4 bytes of 'ip'*/
        p1 = net->ip32;
        p2 = &ip->ip32[3];
        
        /* Mask off bits */
        bits = 32 - bits;
        temp = (ntohl(*p2) >> bits) << bits;

        if(ntohl(*p1) == temp) return SFIP_CONTAINS;

        return SFIP_NOT_CONTAINS;
    }

    p1 = net->ip32;
    p2 = ip->ip32;

    /* Iterate over each 32 bit segment */
    for(i=0; i < bits/32 && i < 3; i++, p1++, p2++) {
        if(*p1 != *p2) 
            return SFIP_NOT_CONTAINS;
    }

    /* At this point, there are some number of remaining bits to check.
     * Mask the bits we don't care about off of "ip" so we can compare
     * the ints directly */
    temp = ntohl(*p2);
    mask = 32 - (bits - 32*i);
    temp = (temp >> mask) << mask;

    /* If p1 was setup correctly through this library, there is no need to 
     * mask off any bits of its own. */
    if(ntohl(*p1) == temp) 
        return SFIP_CONTAINS;

    return SFIP_NOT_CONTAINS;

}

void sfip_raw_ntop(int family, const void *ip_raw, char *buf, int bufsize) {
    int i;

    if(!ip_raw || !buf || !bufsize || 
       (family != AF_INET && family != AF_INET6) || 
       /* Make sure if it's IPv6 that the buf is large enough. */
       /* Need atleast a max of 8 fields of 4 bytes plus 7 for colons in 
        * between.  Need 1 more byte for null. */
       (family == AF_INET6 && bufsize < 8*4 + 7 + 1) ||
       /* Make sure if it's IPv4 that the buf is large enough. */
       /* 4 fields of 3 numbers, plus 3 dots and a null byte */
       (family == AF_INET && bufsize < 3*4 + 4) )
    {
        if(buf && bufsize > 0) buf[0] = 0;
        return;
    }

    /* 4 fields of at most 3 characters each */
    if(family == AF_INET) {
        u_int8_t *p = (u_int8_t*)ip_raw;

        for(i=0; p < ((u_int8_t*)ip_raw) + 4; p++) {
            i += sprintf(&buf[i], "%d", *p);

            /* If this is the last iteration, this could technically cause one
             *  extra byte to be written past the end. */
            if(i < bufsize && ((p + 1) < ((u_int8_t*)ip_raw+4)))
                buf[i] = '.';

            i++;
        }

    /* Check if this is really just an IPv4 address represented as 6, 
     * in compatible format */
#if 0
    } 
    else if(!field[0] && !field[1] && !field[2]) {
        unsigned char *p = (unsigned char *)(&ip->ip[12]);

        for(i=0; p < &ip->ip[16]; p++) 
             i += sprintf(&buf[i], "%d.", *p);
#endif
    } 
    else {
        u_int16_t *p = (u_int16_t*)ip_raw;

        for(i=0; p < ((u_int16_t*)ip_raw) + 8; p++) {
            i += sprintf(&buf[i], "%04x", ntohs(*p));

            /* If this is the last iteration, this could technically cause one
             *  extra byte to be written past the end. */
            if(i < bufsize && ((p + 1) < ((u_int16_t*)ip_raw) + 8))
                buf[i] = ':';

            i++;
        }
    }
}

/* Uses a static buffer to return a string representation of the IP */
char *sfip_to_str(sfip_t *ip) {
    /* IPv6 addresses will be at most 8 fields, of 4 characters each, 
     * with 7 colons inbetween, one NULL, and one fudge byte for sloppy use
     * in sfip_to_strbuf */
    static char buf[8*4 + 7 + 1 + 1];

    if(!ip)
         return NULL;

    sfip_raw_ntop(sfip_family(ip), ip->ip32, buf, sizeof(buf));
    
    return buf;
}

void sfip_free(sfip_t *ip) {
    if(ip) free(ip);
}

/* Returns 1 if the IP is non-zero. 0 otherwise */
int sfip_is_loopback(sfip_t *ip) {
    unsigned int *p;

    ARG_CHECK1(ip, 0);

    if(sfip_family(ip) == AF_INET) {
        return (ip->ip8[0] == 0x7f);
    }

    p = ip->ip32;

    /* Check the first two ints in an IPv6 address,
     * and verify they're NULL.  If not, it's not a loopback */
    if(p[0] || p[1]) return 0;

    /* Check if the 3rd int is a mapped IPv4 address or NULL */
    /* If it's 0xffff, then we might be carrying an IPv4 loopback address */
    if(p[2] != 0xffff && p[2] != 0) return 0;

    if(p[3] == 1 ||       /* IPv6 loopback */
       ip->ip8[12] == 0x7f /* IPv4 loopback over compatible or mapped IPv6 */
      ) return 1;

    return 0;
}

int sfip_ismapped(sfip_t *ip) {
    unsigned int *p;

    ARG_CHECK1(ip, 0);

    if(sfip_family(ip) == AF_INET) 
        return 0;
       
    p = ip->ip32;

    if(p[0] || p[1] || (p[2] != 0xffff && p[2] != 0)) return 0;

    return 1;
}

#ifndef strndup 
char *strndup(const char *s, size_t n) {
    char *ret; 
    size_t len = strlen(s);

    if(len < n) {
        n = len;
    }
 
    ret = (char*)malloc(n+1);

    if(!ret) 
        return NULL;

    strncpy(ret, s, n);
    ret[n] = 0;
    return ret;
}
#endif


#ifdef TESTER
#include <stdio.h>
#include <string.h>

#define PASS 1
#define FAIL 0

int sf_ip_failures = 0;
/* By using a macro, __LINE__  will be right */
#define test(msg, result) { \
    if(result == FAIL) { printf("\tFAILED:\t%s\tline %d\n", msg, __LINE__); sf_ip_failures++; } \
    else printf("\tPassed:\t%s\n", msg);\
}

int test_str(sfip_t *ip, char *str) {
    char *s = sfip_to_str(ip);
    if(!strcmp( s, str) ) return PASS;

    printf("\tShould have seen: \"%s\"\n", str);
    printf("\tInstead saw:      \"%s\"\n\t", s);
    return FAIL;
}

int sf_ip_unittest() {
    unsigned int i = 0xffffffff;
    sfip_t *ip[9];
    sfip_t conv;
   
    /* Verify the simplest allocation method */
    puts("*********************************************************************");
    puts("Testing raw allocation:");
    ip[0] = sfip_alloc_raw(&i, AF_INET);
    test("255.255.255.255", test_str(ip[0], "255.255.255.255"));



    /* The following lines verify parsing via sfip_alloc */
    /* sfip_alloc should be able to recognize IPv4 and IPv6 addresses, 
     * and extract and apply netmasks.  IPv6 address can be specified in 
     * any valid IPv6 notation as recognized by inet_pton.  Netmasks can 
     * either be specified in IP form or by using CIDR notation */
    puts("");
    puts("*********************************************************************");
    puts("Testing parsing:");
    ip[1] = sfip_alloc("192.168.0.1");
    test("192.168.0.1", test_str(ip[1], "192.168.0.1"));
    
    ip[2] = sfip_alloc("255.255.255.255/21");
    test("255.255.255.255/21", test_str(ip[2], "255.255.248.0"));
    
    ip[3] = sfip_alloc("1.1.255.255      255.255.248.0");
    test("1.1.255.255      255.255.248.0", test_str(ip[3], "1.1.248.0"));
    
    ip[4] = sfip_alloc(" 2001:0db8:0000:0000:0000:0000:1428:57ab   ");
    test(" 2001:0db8:0000:0000:0000:0000:1428:57ab   ", test_str(ip[4], "2001:0db8:0000:0000:0000:0000:1428:57ab"));
    
    ip[5] = sfip_alloc("ffff:ffff::1");
    test("ffff:ffff::1", test_str(ip[5], "ffff:ffff:0000:0000:0000:0000:0000:0001"));
    
    ip[6] = sfip_alloc("fFfF::FfFf:FFFF/127");
    test("fFfF::FfFf:FFFF/127", test_str(ip[6], "ffff:0000:0000:0000:0000:0000:ffff:fffe"));
    
    ip[7] = sfip_alloc("ffff::ffff:1/8");
    test("ffff::ffff:1/8", test_str(ip[7], "ff00:0000:0000:0000:0000:0000:0000:0000"));

    ip[8] = sfip_alloc("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff ::3");
    test("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff ::3", test_str(ip[8], "c000:0000:0000:0000:0000:0000:0000:0000"));



    /* Free everything and reallocate it. */
    /* This will atleast /imply/ memory is being handled somewhat properly. */
    puts("");
    puts("*********************************************************************");
    puts("Verifying deletes:");
    /* Make sure we can free: */
    for(i=0; i<9; i++) { sfip_free(ip[i]); }
    i = 0xffffffff;
    /* Reallocate */
    ip[0] = sfip_alloc_raw(&i, AF_INET);
    ip[1] = sfip_alloc("192.168.0.1");
    ip[2] = sfip_alloc("255.255.255.255/21");
    ip[3] = sfip_alloc("1.1.255.255      255.255.248.0");
    ip[4] = sfip_alloc(" 2001:0db8:0000:0000:0000:0000:1428:57ab   ");
    ip[5] = sfip_alloc("ffff:ffff::1");
    ip[6] = sfip_alloc("ffff::ffff:FFFF/127");
    ip[7] = sfip_alloc("ffff::ffff:1/8");
    ip[8] = sfip_alloc("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff ::3");
    printf("\tPassed (as best I can tell, since there was no seg fault)\n");


    /* The following lines verify that IPs can be converted to different families. */
    puts("");
    puts("*********************************************************************");
    puts("Verifying IPv4<->IPv6 conversions:");
    conv = sfip_4to6(ip[0]);
    test("ipv4 -> ipv6", test_str(&conv, "0000:0000:0000:0000:0000:ffff:ffff:ffff"));
    conv = sfip_6to4(&conv);
    /* Converting an IP from v4 to v6 and back to v4 should yield the same IP. */
    test("ipv6 -> ipv4", test_str(&conv, "255.255.255.255"));
    conv = sfip_6to4(ip[4]);
    test("ipv6 -> ipv4", test_str(&conv, "20.40.87.171"));
    

    /* Comparisons can be byte-by-byte, effectively treating an IP that has 
     * been masked as an unmasked IP (treats the masked bits as zeroed) */
    puts("");
    puts("*********************************************************************");
    puts("Testing byte-by-byte comparisons:");
    test("ip[0] > ip[1]", (sfip_compare(ip[0], ip[1]) == SFIP_GREATER) );
    test("ip[1] < ip[2]", (sfip_compare(ip[1], ip[2]) == SFIP_LESSER) );
    test("ip[5] > ip[8]", (sfip_compare(ip[5], ip[8]) == SFIP_GREATER) );
    
    /* Comparisons can also check if an IP is contained within a given network */
    puts("");
    puts("*********************************************************************");
    puts("Testing network containment comparisons:");
    test("ip[0] does not contain ip[1]", (sfip_contains(ip[0], ip[1]) == SFIP_NOT_CONTAINS) );
    test("ip[1] does not contain ip[2]", (sfip_contains(ip[1], ip[2]) == SFIP_NOT_CONTAINS) );
    test("ip[5] does not contain ip[7]", (sfip_contains(ip[5], ip[7]) == SFIP_NOT_CONTAINS) );
    test("ip[7] does contain ip[5]", (sfip_contains(ip[7], ip[5]) == SFIP_CONTAINS) );
    test("ip[2] does contain ip[0]", (sfip_contains(ip[2], ip[0]) == SFIP_CONTAINS) );
    test("ip[0] does not contain ip[2]", (sfip_contains(ip[0], ip[2]) == SFIP_NOT_CONTAINS) );
    test("ip[0] can't be compared to ip[4]", (sfip_contains(ip[0], ip[4]) == SFIP_ARG_ERR) );
    
    puts("*********************************************************************");
    puts(" ... Cleaning up");
    for(i=0; i<9; i++) { sfip_free(ip[i]); }

    printf("\n\tTotal failures: %d\n\n", sf_ip_failures);
    return 1;
}

#endif
/* $Id$ */
/****************************************************************************
 *
 * Copyright (C) 2007-2008 Sourcefire, Inc.
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

#include <string.h>
#include "decode.h" 

#ifdef SUP_IP6

#define FAILURE -1
#define SUCCESS 0
#define IP6_HEADER_LEN 40

#define IP6_VER(x) (x & 0xf000) >> 24

/* The 'Packet' structure is almost always allocated on the stack.
 * Likewise, return buffers will almost always be aswell. 
 * So, for performance reasons, argument validation can be disabled
 * and removed from the code at compile time to prevent unecessary extra 
 * conditionals from being checked at run-time. */
#if ERR_CHK_LVL == 2
#define VALIDATE(x,y) if(!x || !y) return FAILURE;
#elif ERR_CHK_LVL == 1
#define VALIDATE(x,y) if(!y) return FAILURE;
#else
#define VALIDATE(x,y)
#endif

sfip_t *ip6_ret_src(Packet *p) {
    VALIDATE(p, 1);

    return &p->ip6h.ip_src;
}

sfip_t *orig_ip6_ret_src(Packet *p) {
    VALIDATE(p, 1);

    return &p->orig_ip6h.ip_src;
}

sfip_t *ip6_ret_dst(Packet *p) {
    VALIDATE(p, 1);

    return &p->ip6h.ip_dst;
}


sfip_t *orig_ip6_ret_dst(Packet *p) {
    VALIDATE(p, 1);

    return &p->orig_ip6h.ip_dst;
}

u_int16_t ip6_ret_toc(Packet *p) {
    VALIDATE(p,1);

// XXX-IPv6 "NOT YET IMPLEMENTED - ip6_ret_toc"    
    return 0;
}

u_int16_t orig_ip6_ret_toc(Packet *p) {
    VALIDATE(p,1);

// XXX-IPv6 "NOT YET IMPLEMENTED - ip6_ret_toc"    
    return 0;
}

u_int8_t ip6_ret_hops(Packet *p) {
//    VALIDATE(p,1);

    return p->ip6h.hop_lmt;
}
u_int8_t orig_ip6_ret_hops(Packet *p) {
//    VALIDATE(p,1);

    return p->orig_ip6h.hop_lmt;
}

u_int16_t ip6_ret_len(Packet *p) {
    VALIDATE(p,1);

    /* The length field does not include the header in IPv6, but does in IPv4.
     * To make this analogous to IPv4, for Snort's purposes, we need to tack
     * on the difference. */
    return p->ip6h.len;
}

u_int16_t orig_ip6_ret_len(Packet *p) {
    VALIDATE(p,1);

    return p->orig_ip6h.len;
}

u_int16_t ip6_ret_id(Packet *p) {
// XXX-IPv6 "NOT YET IMPLEMENTED - IP6 identification"
    return 0;
}

u_int16_t orig_ip6_ret_id(Packet *p) {
    return 0;
}

u_int8_t ip6_ret_next(Packet *p) {
    VALIDATE(p,1);
    return p->ip6h.next;
}

u_int8_t orig_ip6_ret_next(Packet *p) {
    VALIDATE(p,1);
    return p->orig_ip6h.next;
}

u_int16_t ip6_ret_off(Packet *p) {
// XXX-IPv6 "NOT YET IMPLEMENTED - IP6 frag offset"
    return 0;
}

u_int16_t orig_ip6_ret_off(Packet *p) {
// XXX-IPv6 "NOT YET IMPLEMENTED - IP6 frag offset"
    return 0;
}

u_int8_t ip6_ret_ver(Packet *p) {
    return IP6_VER(p->ip6h.vcl); 
}

u_int8_t orig_ip6_ret_ver(Packet *p) {
    return IP6_VER(p->orig_ip6h.vcl); 
}

sfip_t *ip4_ret_dst(Packet *p) {
    VALIDATE(p,1);
    return &p->ip4h.ip_dst;
}

sfip_t *orig_ip4_ret_dst(Packet *p) {
    VALIDATE(p,1);
    return &p->orig_ip4h.ip_dst;
}

sfip_t *ip4_ret_src(Packet *p) {
    VALIDATE(p,1);
    return &p->ip4h.ip_src;
}

sfip_t *orig_ip4_ret_src(Packet *p) {
    VALIDATE(p,1);
    return &p->orig_ip4h.ip_src;
}

u_int16_t ip4_ret_tos(Packet *p) {
   VALIDATE(p,1);

   return p->ip4h.ip_tos;
}

u_int16_t orig_ip4_ret_tos(Packet *p) {
   VALIDATE(p,1);

   return p->orig_ip4h.ip_tos;
}

u_int8_t ip4_ret_ttl(Packet *p) {
    VALIDATE(p,1);

    return p->ip4h.ip_ttl;
}

u_int8_t orig_ip4_ret_ttl(Packet *p) {
    VALIDATE(p,1);

    return p->orig_ip4h.ip_ttl;
}

u_int16_t ip4_ret_len(Packet *p) {
    VALIDATE(p,1);

    return p->ip4h.ip_len;
}

u_int16_t orig_ip4_ret_len(Packet *p) {
    VALIDATE(p,1);

    return p->orig_ip4h.ip_len;
}

u_int16_t ip4_ret_id(Packet *p) {
    VALIDATE(p,1);
    
    return p->ip4h.ip_id;
}

u_int16_t orig_ip4_ret_id(Packet *p) {
    VALIDATE(p,1);
    
    return p->orig_ip4h.ip_id;
}

u_int8_t ip4_ret_proto(Packet *p) {
    // VALIDATION()
    
    return p->ip4h.ip_proto;
}

u_int8_t orig_ip4_ret_proto(Packet *p) {
    // VALIDATION()
    
    return p->orig_ip4h.ip_proto;
}

u_int16_t ip4_ret_off(Packet *p) {
    return p->ip4h.ip_off;
}

u_int16_t orig_ip4_ret_off(Packet *p) {
    return p->orig_ip4h.ip_off;
}

u_int8_t ip4_ret_ver(Packet *p) {
    return IP_VER(p->iph); 
}

u_int8_t orig_ip4_ret_ver(Packet *p) {
    return IP_VER(p->orig_iph);
}

u_int8_t ip4_ret_hlen(Packet *p) {
    return IP_HLEN(p->iph);
}

u_int8_t orig_ip4_ret_hlen(Packet *p) {
    return IP_HLEN(p->orig_iph);
}

u_int8_t ip6_ret_hlen(Packet *p) {
    /* Snort is expecting this number to be in terms of 32 bit words */
    return IP6_HDR_LEN / 4 ;
}

u_int8_t orig_ip6_ret_hlen(Packet *p) {
    return IP6_HDR_LEN / 4;
}

void sfiph_build(Packet *p, const void *hdr, int family) {
    IP6RawHdr *hdr6;
    IPHdr *hdr4;

    if(!p || !hdr)
        return;

    set_callbacks(p, family);

    if(family == AF_INET) {
        hdr4 = (IPHdr*)hdr;

        /* The struct Snort uses is identical to the actual IP6 struct, 
         * with the exception of the IP addresses. Copy over everything but
         * the IPs */
        memcpy(&p->ip4h, hdr4, sizeof(IPHdr) - 8);
        sfip_set_raw(&p->ip4h.ip_src, &hdr4->ip_src, p->family);
        sfip_set_raw(&p->ip4h.ip_dst, &hdr4->ip_dst, p->family);
        p->actual_ip_len = ntohs(p->ip4h.ip_len);
    } else {
        hdr6 = (IP6RawHdr*)hdr;
           
        /* The struct Snort uses is identical to the actual IP6 struct, 
         * with the exception of the IP addresses. Copy over everything but
         * the IPs*/
        memcpy(&p->ip6h, hdr6, sizeof(IP6RawHdr) - 32);
        sfip_set_raw(&p->ip6h.ip_src, &hdr6->ip6_src, p->family);
        sfip_set_raw(&p->ip6h.ip_dst, &hdr6->ip6_dst, p->family);
        p->actual_ip_len = ntohs(p->ip6h.len) + IP6_HDR_LEN;
    }
}

void sfiph_orig_build(Packet *p, const void *hdr, int family) {
    IP6RawHdr *hdr6;
    IPHdr *hdr4;

    if(!p || !hdr)
        return;

    set_callbacks(p, family);

    if(family == AF_INET) {
        hdr4 = (IPHdr*)hdr;

        /* The struct Snort uses is identical to the actual IP6 struct, 
         * with the exception of the IP addresses. Copy over everything but
         * the IPs */
        memcpy(&p->orig_ip4h, hdr4, sizeof(IPHdr) - 8);
        sfip_set_raw(&p->orig_ip4h.ip_src, &hdr4->ip_src, p->family);
        sfip_set_raw(&p->orig_ip4h.ip_dst, &hdr4->ip_dst, p->family);
        p->actual_ip_len = ntohs(p->orig_ip4h.ip_len);
    } else {
        hdr6 = (IP6RawHdr*)hdr;
           
        /* The struct Snort uses is identical to the actual IP6 struct, 
         * with the exception of the IP addresses. Copy over everything but
         * the IPs*/
        memcpy(&p->orig_ip6h, hdr6, sizeof(IP6RawHdr) - 32);
        sfip_set_raw(&p->orig_ip6h.ip_src, &hdr6->ip6_src, p->family);
        sfip_set_raw(&p->orig_ip6h.ip_dst, &hdr6->ip6_dst, p->family);
        p->actual_ip_len = ntohs(p->orig_ip6h.len) + IP6_HDR_LEN;
    }
}

#ifdef TESTER
int main() {
    Packet p;
    IP4Hdr i4;
    IP6Hdr i6;

    /* This test assumes we get an IPv4 packet and verifies
     * that the correct callbacks are setup, and they return
     * the correct values. */

    set_callbacks(AF_INET);

    /* Same test as above, but with IPv6 */
    set_callbacks(AF_INET6);

    return 0;
}
#endif
#endif /* #ifdef SUP_IP6 */
/* $Id$ */
/*
** Copyright (C) 2002-2008 Sourcefire, Inc.
** Copyright (C) 1998-2002 Martin Roesch <roesch@sourcefire.com>

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

/***************************************************************************
 *
 * File: MSTRING.C
 *
 * Purpose: Provide a variety of string functions not included in libc.  Makes
 *          up for the fact that the libstdc++ is hard to get reference
 *          material on and I don't want to write any more non-portable c++
 *          code until I have solid references and libraries to use.
 *
 * History:
 *
 * Date:      Author:  Notes:
 * ---------- ------- ----------------------------------------------
 *  08/19/98    MFR    Initial coding begun
 *  03/06/99    MFR    Added Boyer-Moore pattern match routine, don't use
 *                     mContainsSubstr() any more if you don't have to
 *  12/31/99	JGW    Added a full Boyer-Moore implementation to increase
 *                     performance. Added a case insensitive version of mSearch
 *  07/24/01    MFR    Fixed Regex pattern matcher introduced by Fyodor
 *
 **************************************************************************/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>

#include "debug.h"
#include "plugbase.h" /* needed for fasthex() */
#include "util.h"

#ifdef GIDS
extern int detect_depth;
#endif /* GIDS */


const u_int8_t *doe_ptr;

#ifdef TEST_MSTRING

int main()
{
    char test[] = "\0\0\0\0\0\0\0\0\0CKAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\0\0";
    char find[] = "CKAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\0\0";

/*   char test[] = "\x90\x90\x90\x90\x90\x90\xe8\xc0\xff\xff\xff/bin/sh\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90";
     char find[] = "\xe8\xc0\xff\xff\xff/bin/sh";  */
    int i;
    int toks;
    int *shift;
    int *skip;

/*   shift=make_shift(find,sizeof(find)-1);
     skip=make_skip(find,sizeof(find)-1); */

    DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH,"%d\n",
			    mSearch(test, sizeof(test) - 1, find,
				    sizeof(find) - 1, shift, skip)););

    return 0;
}

#endif

/****************************************************************
 *
 *  Function: mSplit()
 *
 *  Purpose: Splits a string into tokens non-destructively.
 *
 *  Parameters:
 *      char *str => the string to be split
 *      char *sep => a string of token seperaters
 *      int max_strs => how many tokens should be returned
 *      int *toks => place to store the number of tokens found in str
 *      char meta => the "escape metacharacter", treat the character
 *                   after this character as a literal and "escape" a
 *                   seperator
 *
 *  Returns:
 *      2D char array with one token per "row" of the returned
 *      array.
 *
 ****************************************************************/
char **mSplit(char *str, const char *sep, int max_strs, int *toks, char meta)
{
    char **retstr;      /* 2D array which is returned to caller */
    char *idx;          /* index pointer into str */
    char *end;          /* ptr to end of str */
    const char *sep_end;/* ptr to end of seperator string */
    const char *sep_idx;/* index ptr into seperator string */
    int len = 0;        /* length of current token string */
    int curr_str = 0;       /* current index into the 2D return array */
    unsigned char last_char = 0xFF;  /* initialize to something that won't be in meta */

    if(!toks) return NULL;

    *toks = 0;

    if (!str || !*str) return NULL;

    DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH, 
                "[*] Splitting string: %s\n", str);
            DebugMessage(DEBUG_PATTERN_MATCH, "curr_str = %d\n", curr_str););

    /*
     * find the ends of the respective passed strings so our while() loops
     * know where to stop
     */
    sep_end = sep + strlen(sep);
    end = str + strlen(str);

    /* remove trailing whitespace */
    while(isspace((int) *(end - 1)) && ((end - 1) >= str))
        *(--end) = '\0';    /* -1 because of NULL */

    /* set our indexing pointers */
    sep_idx = sep;
    idx = str;

    /*
     * alloc space for the return string, this is where the pointers to the
     * tokens will be stored
     */
    retstr = (char **) SnortAlloc( sizeof(char **) * max_strs );

    max_strs--;

    DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH, 
                "max_strs = %d  curr_str = %d\n", 
                max_strs, curr_str););

    /* loop thru each letter in the string being tokenized */
    while(idx < end)
    {
        /* loop thru each seperator string char */
        while(sep_idx < sep_end)
        {
            /*
             * if the current string-indexed char matches the current
             * seperator char...
             */
            if((*idx == *sep_idx) && (last_char != meta))
            {
                /* if there's something to store... */
                if(len > 0)
                {
                    DEBUG_WRAP(
                            DebugMessage(DEBUG_PATTERN_MATCH, 
                                "Allocating %d bytes for token ", len + 1););
                    if(curr_str <= max_strs)
                    {
                        /* allocate space for the new token */
                        retstr[curr_str] = (char *)SnortAlloc((len + 1) * sizeof(char));

                        /* copy the token into the return string array */
                        memcpy(retstr[curr_str], (idx - len), len);
                        retstr[curr_str][len] = 0;
                        DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH, 
                                    "tok[%d]: %s\n", curr_str, 
                                    retstr[curr_str]););

                        /* twiddle the necessary pointers and vars */
                        len = 0;
                        curr_str++;
                        DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH, 
                                    "curr_str = %d\n", curr_str);
                                DebugMessage(DEBUG_PATTERN_MATCH, 
                                    "max_strs = %d  curr_str = %d\n", 
                                    max_strs, curr_str););

                        last_char = *idx;
                        idx++;
                    }

                    DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH, 
                                "Checking if curr_str (%d) >= max_strs (%d)\n",
                               curr_str, max_strs););

                    /*
                     * if we've gotten all the tokens requested, return the
                     * list
                     */
                    if(curr_str >= max_strs)
                    {
                        while(isspace((int) *idx))
                            idx++;

                        len = end - idx;
                        DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH, 
                                    "Finishing up...\n");
                                DebugMessage(DEBUG_PATTERN_MATCH, 
                                    "Allocating %d bytes "
                                    "for last token ", len + 1););
                        fflush(stdout);

                        retstr[curr_str] = (char *)SnortAlloc((len + 1) * sizeof(char));

                        memcpy(retstr[curr_str], idx, len);
                        retstr[curr_str][len] = 0;

                        DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH, 
                                    "tok[%d]: %s\n", curr_str, 
                                    retstr[curr_str]););

                        *toks = curr_str + 1;
                        DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH, 
                                    "max_strs = %d  curr_str = %d\n", 
                                    max_strs, curr_str);
                                DebugMessage(DEBUG_PATTERN_MATCH, 
                                    "mSplit got %d tokens!\n", *toks););

                        return retstr;
                    }
                }
                else
                    /*
                     * otherwise, the previous char was a seperator as well,
                     * and we should just continue
                     */
                {
                    last_char = *idx;
                    idx++;
                    /* make sure to reset this so we test all the sep. chars */
                    sep_idx = sep;
                    len = 0;
                }
            }
            else
            {
                /* go to the next seperator */
                sep_idx++;
            }
        }

        sep_idx = sep;
        len++;
        last_char = *idx;
        idx++;
    }

    /* put the last string into the list */

    if(len > 0)
    {
        DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH, 
                    "Allocating %d bytes for last token ", len + 1););

        retstr[curr_str] = (char *)SnortAlloc((len + 1) * sizeof(char));

        memcpy(retstr[curr_str], (idx - len), len);
        retstr[curr_str][len] = 0;

        DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH,"tok[%d]: %s\n", curr_str, 
                    retstr[curr_str]););
        *toks = curr_str + 1;
    }

    DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH, 
                "mSplit got %d tokens!\n", *toks););

    /* return the token list */
    return retstr;
}




/****************************************************************
 *
 * Free the buffer allocated by mSplit().
 *
 * char** toks = NULL;
 * int num_toks = 0;
 * toks = (str, " ", 2, &num_toks, 0);
 * mSplitFree(&toks, num_toks);
 *
 * At this point, toks is again NULL.
 *
 ****************************************************************/
void mSplitFree(char ***pbuf, int num_toks)
{
    int i;
    char** buf;  /* array of string pointers */

    if( pbuf==NULL || *pbuf==NULL )
    {
        return;
    }

    buf = *pbuf;

    for( i=0; i<num_toks; i++ )
    {
        if( buf[i] != NULL )
        {
            free( buf[i] );
            buf[i] = NULL;
        }
    }

    free(buf);
    *pbuf = NULL;
}




/****************************************************************
 *
 *  Function: mContainsSubstr(char *, int, char *, int)
 *
 *  Purpose: Determines if a string contains a (non-regex)
 *           substring.
 *
 *  Parameters:
 *      buf => data buffer we want to find the data in
 *      b_len => data buffer length
 *      pat => pattern to find
 *      p_len => length of the data in the pattern buffer
 *
 *  Returns:
 *      Integer value, 1 on success (str constains substr), 0 on
 *      failure (substr not in str)
 *
 ****************************************************************/
int mContainsSubstr(const char *buf, int b_len, const char *pat, int p_len)
{
    const char *b_idx;  /* index ptr into the data buffer */
    const char *p_idx;  /* index ptr into the pattern buffer */
    const char *b_end;  /* ptr to the end of the data buffer */
    int m_cnt = 0;      /* number of pattern matches so far... */
#ifdef DEBUG
    unsigned long loopcnt = 0;
#endif

    /* mark the end of the strs */
    b_end = (char *) (buf + b_len);

    /* init the index ptrs */
    b_idx = buf;
    p_idx = pat;

    do
    {
#ifdef DEBUG
        loopcnt++;
#endif

        if(*p_idx == *b_idx)
        {

            if(m_cnt == (p_len - 1))
            {
		DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH,
					"\n%ld compares for match\n", loopcnt););
                return 1;
            }
            m_cnt++;
            b_idx++;
            p_idx++;
        }
        else
        {
            if(m_cnt == 0)
            {
                b_idx++;
            }
            else
            {
                b_idx = b_idx - (m_cnt - 1);
            }

            p_idx = pat;

            m_cnt = 0;
        }

    } while(b_idx < b_end);


    /* if we make it here we didn't find what we were looking for */
    return 0;
}




/****************************************************************
 *
 *  Function: make_skip(char *, int)
 *
 *  Purpose: Create a Boyer-Moore skip table for a given pattern
 *
 *  Parameters:
 *      ptrn => pattern
 *      plen => length of the data in the pattern buffer
 *
 *  Returns:
 *      int * - the skip table
 *
 ****************************************************************/
int *make_skip(char *ptrn, int plen)
{
    int  i;
    int *skip = (int *) SnortAlloc(256* sizeof(int));

    for ( i = 0; i < 256; i++ )
        skip[i] = plen + 1;

    while(plen != 0)
        skip[(unsigned char) *ptrn++] = plen--;

    return skip;
}



/****************************************************************
 *
 *  Function: make_shift(char *, int)
 *
 *  Purpose: Create a Boyer-Moore shift table for a given pattern
 *
 *  Parameters:
 *      ptrn => pattern
 *      plen => length of the data in the pattern buffer
 *
 *  Returns:
 *      int * - the shift table
 *
 ****************************************************************/
int *make_shift(char *ptrn, int plen)
{
    int *shift = (int *) SnortAlloc(plen * sizeof(int));
    int *sptr = shift + plen - 1;
    char *pptr = ptrn + plen - 1;
    char c;

     c = ptrn[plen - 1];

    *sptr = 1;

    while(sptr-- != shift)
    {
        char *p1 = ptrn + plen - 2, *p2, *p3;

        do
        {
            while(p1 >= ptrn && *p1-- != c);

            p2 = ptrn + plen - 2;
            p3 = p1;

            while(p3 >= ptrn && *p3-- == *p2-- && p2 >= pptr);
        }
        while(p3 >= ptrn && p2 >= pptr);

        *sptr = shift + plen - sptr + p2 - p3;

        pptr--;
    }

    return shift;
}



/****************************************************************
 *
 *  Function: mSearch(char *, int, char *, int)
 *
 *  Purpose: Determines if a string contains a (non-regex)
 *           substring.
 *
 *  Parameters:
 *      buf => data buffer we want to find the data in
 *      blen => data buffer length
 *      ptrn => pattern to find
 *      plen => length of the data in the pattern buffer
 *      skip => the B-M skip array
 *      shift => the B-M shift array
 *
 *  Returns:
 *      Integer value, 1 on success (str constains substr), 0 on
 *      failure (substr not in str)
 *
 ****************************************************************/
int mSearch(const char *buf, int blen, const char *ptrn, int plen, int *skip, int *shift)
{
    int b_idx = plen;

#ifdef DEBUG
    char *hexbuf;
    int cmpcnt = 0;
#endif

    DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH,"buf: %p  blen: %d  ptrn: %p  "
                "plen: %d\n", buf, blen, ptrn, plen););

#ifdef DEBUG
    hexbuf = fasthex((const u_char *)buf, blen);
    DebugMessage(DEBUG_PATTERN_MATCH,"buf: %s\n", hexbuf);
    free(hexbuf);
    hexbuf = fasthex((const u_char *)ptrn, plen);
    DebugMessage(DEBUG_PATTERN_MATCH,"ptrn: %s\n", hexbuf);
    free(hexbuf);
    DebugMessage(DEBUG_PATTERN_MATCH,"buf: %p  blen: %d  ptrn: %p  "
                 "plen: %d\n", buf, blen, ptrn, plen);
#endif /* DEBUG */
    if(plen == 0)
        return 1;

    while(b_idx <= blen)
    {
        int p_idx = plen, skip_stride, shift_stride;

        while(buf[--b_idx] == ptrn[--p_idx])
        {
#ifdef DEBUG
            cmpcnt++;
#endif
            if(b_idx < 0)
                return 0;

            if(p_idx == 0)
            {
                DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH, 
                            "match: compares = %d.\n", cmpcnt););

                doe_ptr = (const u_int8_t *)&(buf[b_idx]) + plen;

#ifdef GIDS
                detect_depth = b_idx;
#endif /* GIDS */

                return 1;
            }
        }

        skip_stride = skip[(unsigned char) buf[b_idx]];
        shift_stride = shift[p_idx];

        b_idx += (skip_stride > shift_stride) ? skip_stride : shift_stride;
    }

    DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH,
                "no match: compares = %d.\n", cmpcnt););

    return 0;
}



/****************************************************************
 *
 *  Function: mSearchCI(char *, int, char *, int)
 *
 *  Purpose: Determines if a string contains a (non-regex)
 *           substring matching is case insensitive
 *
 *  Parameters:
 *      buf => data buffer we want to find the data in
 *      blen => data buffer length
 *      ptrn => pattern to find
 *      plen => length of the data in the pattern buffer
 *      skip => the B-M skip array
 *      shift => the B-M shift array
 *
 *  Returns:
 *      Integer value, 1 on success (str constains substr), 0 on
 *      failure (substr not in str)
 *
 ****************************************************************/
int mSearchCI(const char *buf, int blen, const char *ptrn, int plen, int *skip, int *shift)
{
    int b_idx = plen;
#ifdef DEBUG
    int cmpcnt = 0;
#endif

    if(plen == 0)
        return 1;

    while(b_idx <= blen)
    {
        int p_idx = plen, skip_stride, shift_stride;

        while((unsigned char) ptrn[--p_idx] == 
                toupper((unsigned char) buf[--b_idx]))
        {
#ifdef DEBUG
            cmpcnt++;
#endif
            if(p_idx == 0)
            {
                DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH, 
                            "match: compares = %d.\n", 
                            cmpcnt););
                doe_ptr = (const u_int8_t *)&(buf[b_idx]) + plen;
#ifdef GIDS
                detect_depth = b_idx;
#endif /* GIDS */
                return 1;
            }
        }

        skip_stride = skip[toupper((unsigned char) buf[b_idx])];
        shift_stride = shift[p_idx];

        b_idx += (skip_stride > shift_stride) ? skip_stride : shift_stride;
    }

    DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH, "no match: compares = %d.\n", cmpcnt););

    return 0;
}


/****************************************************************
 *
 *  Function: mSearchREG(char *, int, char *, int)
 *
 *  Purpose: Determines if a string contains a (regex)
 *           substring.
 *
 *  Parameters:
 *      buf => data buffer we want to find the data in
 *      blen => data buffer length
 *      ptrn => pattern to find
 *      plen => length of the data in the pattern buffer
 *      skip => the B-M skip array
 *      shift => the B-M shift array
 *
 *  Returns:
 *      Integer value, 1 on success (str constains substr), 0 on
 *      failure (substr not in str)
 *
 ****************************************************************/
int mSearchREG(const char *buf, int blen, const char *ptrn, int plen, int *skip, int *shift)
{
    int b_idx = plen;
    int literal = 0;
    int regexcomp = 0;
#ifdef DEBUG
    int cmpcnt = 0;
#endif /*DEBUG*/
    
    DEBUG_WRAP(
	       DebugMessage(DEBUG_PATTERN_MATCH, "buf: %p  blen: %d  ptrn: %p "
			    " plen: %d b_idx: %d\n", buf, blen, ptrn, plen, b_idx);
	       DebugMessage(DEBUG_PATTERN_MATCH, "packet data: \"%s\"\n", buf);
	       DebugMessage(DEBUG_PATTERN_MATCH, "matching for \"%s\"\n", ptrn);
	       );
	       
    if(plen == 0)
        return 1;

    while(b_idx <= blen)
    {
        int p_idx = plen, skip_stride, shift_stride;

	DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH, "Looping... "
				"([%d]0x%X (%c) -> [%d]0x%X(%c))\n",
				b_idx, buf[b_idx-1], 
				buf[b_idx-1], 
				p_idx, ptrn[p_idx-1], ptrn[p_idx-1]););

        while(buf[--b_idx] == ptrn[--p_idx]
              || (ptrn[p_idx] == '?' && !literal)
              || (ptrn[p_idx] == '*' && !literal)
              || (ptrn[p_idx] == '\\' && !literal))
        {
	    DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH, "comparing: b:%c -> p:%c\n", 
				    buf[b_idx], ptrn[p_idx]););
#ifdef DEBUG
            cmpcnt++;
#endif

            if(literal)
                literal = 0;
            if(!literal && ptrn[p_idx] == '\\')
                literal = 1;
            if(ptrn[p_idx] == '*')
            {
		DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH,"Checking wildcard matching...\n"););
                while(p_idx != 0 && ptrn[--p_idx] == '*'); /* fool-proof */

                while(buf[--b_idx] != ptrn[p_idx])
                {
		    DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH, "comparing: b[%d]:%c -> p[%d]:%c\n",
					    b_idx, buf[b_idx], p_idx, ptrn[p_idx]););

                   regexcomp++;
                    if(b_idx == 0)
                    {
			DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH,
						"b_idx went to 0, returning 0\n");)
                        return 0;
                    }
                }

		DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH, "got wildcard final char match! (b[%d]: %c -> p[%d]: %c\n", b_idx, buf[b_idx], p_idx, ptrn[p_idx]););
            }

            if(p_idx == 0)
            {
		DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH, "match: compares = %d.\n", 
					cmpcnt););
                return 1;
            }

            if(b_idx == 0)
                break;
        }

	DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH, "skip-shifting...\n"););
	skip_stride = skip[(unsigned char) buf[b_idx]];
	shift_stride = shift[p_idx];
	
	b_idx += (skip_stride > shift_stride) ? skip_stride : shift_stride;
	DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH, "b_idx skip-shifted to %d\n", b_idx););
	b_idx += regexcomp;
	DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH,
				"b_idx regex compensated %d steps, to %d\n", regexcomp, b_idx););
	regexcomp = 0;
    }

    DEBUG_WRAP(DebugMessage(DEBUG_PATTERN_MATCH, "no match: compares = %d, b_idx = %d, "
			    "blen = %d\n", cmpcnt, b_idx, blen););

    return 0;
}
