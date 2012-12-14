/*
 *  the kernel packet generator module, it can generator TCP/UDP packet
 *
 *  write by Forrest.zhang
 */

#define  _KDEBUG_               1

#include <linux/module.h>
#include <linux/init.h>
#include <kdebug.h>

/* packet type only TCP/UDP */
#define PKT_TYPE_UDP                1
#define PKT_TYPE_TCP                2
#define PKT_CMD_START               1
#define PKT_CMD_STOP                0
#define PKT_PROC_STATUS_BUFLEN      2048             /* store modules status */
#define PKT_PROC_STATS_BUFLEN       1024             /* store statistic data */

/* the device used to send packet */
struct pkt_dev {
    __u32       size;
    __u32       count;
    int         used;
    char        name[DEV_NAMEMAX];
};

/* the kernel thread used to send packet */
struct pkt_thread {
    __u32       size;
    __u32       count;
    int         cpu;
    int         used;
}

/*
 *  the main loop to send packet.
 *  @data         type is struct pkt_thread;
 */
static void send_loop(void *data)
{

}


/*
 *  create seven proc entry on /proc
 *  /proc/ksend_status     readonly    
 *                         the ksend packet send status: 
 *                         protocol                TCP packet or UDP packet
 *                         time                    start n seconds then stop
 *                         packet size             each packet size
 *                         throughout              XXMbps when send
 *
 *  /proc/ksend_stats      readonly
 *                         the ksend statistic of packet send out
 *                         cpu                     which CPU send
 *                         type                    TCP packet or UDP packet
 *                         time                    spend n seconds send all packet
 *                         packet size             each packet size
 *                         bytes                   how many bytes send
 *                         count                   how many packet send
 *                         througout               XXMbps when send
 *
 *  /proc/ksend_cmd        writeonly
 *                         status                  write start or stop to start or stop send
 *
 *  /proc/ksend_time       writeonly
 *                         XXX                     send XXX second
 *
 *  /proc/ksend_pktsize    writeonly
 *                         XXX                     the packet size
 *
 *  /proc/ksend_pkttype    writeonly
 *                         XXX                     the packet type TCP or UDP
 *
 *  /proc/ksend_througout  writeonly
 *                         XXX                     the throughout of packet
 */
static int __init create_ksend_proc(void)
{
    
}

static void __exit destroy_ksend_proc(void)
{
    
}

static int __init ksend_init(void)
{
    DBG("ksend_init success!\n");
    return 0;
}

static void __exit ksend_exit(void)
{
    DBG("ksend_exit success!\n");
}

module_init(ksend_init);
module_exit(ksend_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Forrest.zhang");



