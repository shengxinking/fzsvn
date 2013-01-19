/**
 *	file	packet.h
 *	brief	Declare a set of function to decode raw packet get from PCAP.
 *
 *	date	2008-07-18
 */

#ifndef FZ_PACKET_H
#define FZ_PACKET_H

#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <stdint.h>
#include <sys/types.h>
#include <pcap.h>

/* Ethernet type */
#define ETHERNET_MTU                  1500
#define ETHERNET_TYPE_IP              0x0800
#define ETHERNET_TYPE_ARP             0x0806
#define ETHERNET_TYPE_REVARP          0x8035
#define ETHERNET_TYPE_IPV6            0x86dd
#define ETHERNET_TYPE_LOOP            0x9000
#define ETHERNET_HEADER_LEN             14

/* protocol header length */
#define IP_HEADER_LEN           20
#define TCP_HEADER_LEN          20

/* packet length */
#define IP_MAXPACKET    65535        /* maximum packet size */

/* TCP flags */
#define TH_FIN  0x01
#define TH_SYN  0x02
#define TH_RST  0x04
#define TH_PUSH 0x08
#define TH_ACK  0x10
#define TH_URG  0x20
#define TH_RES2 0x40
#define TH_RES1 0x80
#define TH_NORESERVED (TH_FIN|TH_SYN|TH_RST|TH_PUSH|TH_ACK|TH_URG)

#ifndef TCP_MSS
#define    TCP_MSS      512
#endif

#ifndef TCP_MAXWIN
#define    TCP_MAXWIN   65535    /* largest value for (unscaled) window */
#endif

#ifndef TCP_MAX_WINSHIFT 
#define TCP_MAX_WINSHIFT    14    /* maximum window shift */
#endif

/*
 * User-settable options (used with setsockopt).
 */
#ifndef TCP_NODELAY
#define    TCP_NODELAY   0x01    /* don't delay send to coalesce packets */
#endif

#ifndef TCP_MAXSEG
#define    TCP_MAXSEG    0x02    /* set maximum segment size */
#endif

#define SOL_TCP        6    /* TCP level */


/* PCAP parameter */
#define SNAPLEN         1514
#define MIN_SNAPLEN         68
#define PROMISC             1
#define READ_TIMEOUT        500

/* packet status flags */
#define PKT_REBUILT_FRAG     0x00000001  /* is a rebuilt fragment */
#define PKT_REBUILT_STREAM   0x00000002  /* is a rebuilt stream */
#define PKT_STREAM_UNEST_UNI 0x00000004  /* is from an unestablished stream and
                                          * we've only seen traffic in one
                                          * direction
                                          */
#define PKT_STREAM_UNEST_BI  0x00000008  /* is from an unestablished stream and
                                          * we've seen traffic in both 
                                          * directions
                                          */
#define PKT_STREAM_EST       0x00000010  /* is from an established stream */
#define PKT_FROM_SERVER      0x00000040  /* this packet came from the server
                                            side of a connection (TCP) */
#define PKT_FROM_CLIENT      0x00000080  /* this packet came from the client
                                            side of a connection (TCP) */
#define PKT_FRAG_ALERTED     0x00000200  /* this packet has been alerted by 
                                            defrag */
#define PKT_STREAM_INSERT    0x00000400  /* this packet has been inserted into stream4 */
#define PKT_ALT_DECODE       0x00000800  /* this packet has been normalized by telnet
                                             (only set when we must look at an alernative buffer)
                                         */
#define PKT_STREAM_TWH       0x00001000
#define PKT_IGNORE_PORT      0x00002000  /* this packet should be ignored, based on port */
#define PKT_PASS_RULE        0x00004000  /* this packet has matched a pass rule */
#define PKT_NO_DETECT        0x00008000  /* this packet should not be preprocessed */
#define PKT_PREPROC_RPKT     0x00010000  /* set in original packet to indicate a preprocessor
                                          * has a reassembled packet */
#define PKT_DCE_RPKT         0x00020000  /* this packet is a DCE/RPC reassembled one */
#define PKT_IP_RULE          0x00040000  /* this packet is being evaluated against an IP rule */
#define PKT_IP_RULE_2ND      0x00080000  /* this packet is being evaluated against an IP rule */
#define PKT_STATELESS        0x10000000  /* Packet has matched a stateless rule */
#define PKT_INLINE_DROP      0x20000000
#define PKT_OBFUSCATED       0x40000000  /* this packet has been obfuscated */
#define PKT_LOGGED           0x80000000  /* this packet has been logged */

/* 
 *	Ethernet header
 */
typedef struct _EtherHdr {
	u_int8_t ether_dst[6];
	u_int8_t ether_src[6];
	u_int16_t ether_type;
} EtherHdr;


/* Get/Set IP version and Header length */
#define IP_VER(iph)    (((iph)->ip_verhl & 0xf0) >> 4)
#define IP_HLEN(iph)   ((iph)->ip_verhl & 0x0f)
#define SET_IP_VER(iph, value)  ((iph)->ip_verhl = (unsigned char)(((iph)->ip_verhl & 0x0f) | (value << 4)))
#define SET_IP_HLEN(iph, value)  ((iph)->ip_verhl = (unsigned char)(((iph)->ip_verhl & 0xf0) | (value & 0x0f)))

/**
 *	IP header
 */
typedef struct _IPHdr
{
    u_int8_t ip_verhl;      /* version & header length */
    u_int8_t ip_tos;        /* type of service */
    u_int16_t ip_len;       /* datagram length */
    u_int16_t ip_id;        /* identification  */
    u_int16_t ip_off;       /* fragment offset */
    u_int8_t ip_ttl;        /* time to live field */
    u_int8_t ip_proto;      /* datagram protocol */
    u_int16_t ip_csum;      /* checksum */
    struct in_addr ip_src;  /* source IP */
    struct in_addr ip_dst;  /* dest IP */
} IPHdr;

#define iph_is_valid(p) (p->family != NO_IP)
#define NO_IP 0
#define IP_CHECKSUMS 0

/* more macros for TCP offset */
#define TCP_OFFSET(tcph)        (((tcph)->th_offx2 & 0xf0) >> 4)
#define TCP_X2(tcph)            ((tcph)->th_offx2 & 0x0f)

#define TCP_ISFLAGSET(tcph, flags) (((tcph)->th_flags & (flags)) == (flags))

/* we need to change them as well as get them */
#define SET_TCP_OFFSET(tcph, value)  ((tcph)->th_offx2 = (unsigned char)(((tcph)->th_offx2 & 0x0f) | (value << 4)))
#define SET_TCP_X2(tcph, value)  ((tcph)->th_offx2 = (unsigned char)(((tcph)->th_offx2 & 0xf0) | (value & 0x0f)))

#define TCP_CHECKSUMS 0

/**
 *	TCP header
 */
typedef struct _TCPHdr {
	u_int16_t th_sport;     /* source port */
	u_int16_t th_dport;     /* destination port */
	u_int32_t th_seq;       /* sequence number */
	u_int32_t th_ack;       /* acknowledgement number */
	u_int8_t th_offx2;      /* offset and reserved */
	u_int8_t th_flags;
	u_int16_t th_win;       /* window */
	u_int16_t th_sum;       /* checksum */
	u_int16_t th_urp;       /* urgent pointer */
} TCPHdr;



/**
 *	The Packet struct is used for IP-defrag and TCP-reasm.
 */
typedef struct _Packet {
	const struct pcap_pkthdr *pkth;   /* BPF data */
	const u_int8_t *pkt;              /* base pointer to the raw packet data */

	const EtherHdr *eh;         /* standard TCP/IP/Ethernet/ARP headers */

	const IPHdr *iph;
	u_int32_t ip_options_len;
	const u_int8_t *ip_options_data;

	const TCPHdr *tcph;
	u_int32_t tcp_options_len;
	const u_int8_t *tcp_options_data;

	const u_int8_t *data;   /* packet payload pointer */
	u_int16_t dsize;        /* packet payload size */
	u_int16_t alt_dsize;    /* the dsize of a packet before munging (used for log)*/

	/* IP fragment */
	u_int8_t frag_flag;     /* flag to indicate a fragmented packet */
	u_int16_t frag_offset;  /* fragment offset number */
	u_int8_t mf;            /* more fragments flag */
	u_int8_t df;            /* don't fragment flag */
	u_int8_t rf;            /* IP reserved bit */

	u_int16_t sp;           /* source port (TCP/UDP) */
	u_int16_t dp;           /* dest port (TCP/UDP) */
	u_int32_t caplen;

	void *ssnptr;           /* for tcp session tracking info... */
	void *fragtracker;      /* for ip fragmentation tracking info... */
	void *flow;             /* for flow info */
	void *streamptr;        /* for tcp pkt dump */
    
	u_int8_t csum_flags;        /* checksum flags */
	u_int32_t packet_flags;     /* special flags for the packet */

	const u_int8_t *ip_data;   /* IP payload pointer */
	u_int16_t ip_dsize;        /* IP payload size */

} Packet;

typedef struct s_pseudoheader
{
	u_int32_t sip, dip; 
	u_int8_t  zero;     
	u_int8_t  protocol; 
	u_int16_t len; 
} PSEUDO_HDR;

extern int DecodeEthPkt(Packet *, const struct pcap_pkthdr *, const u_int8_t *);
extern int DecodeIP(const u_int8_t *, const u_int32_t, Packet *);
extern int DecodeTCP(const u_int8_t *, const u_int32_t, Packet *);

#endif /* FZ_PACKET_H */
