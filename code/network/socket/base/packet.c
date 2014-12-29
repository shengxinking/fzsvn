/*
 *  APIs for create PF_PACKET socket and send/recv funtion using PF_PACKET socket
 *
 *  write by Forrest.zhang
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>

#include <netpacket/packet.h>
#include <netinet/ether.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/if_ether.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

#include "packet.h"

#define PACKET_ERROR     snprintf(_err_buf, ERRBUFSIZE, "%s: %s", \
                                  __FUNCTION__, strerror(errno))
#define ERRBUFSIZE       128


struct psehdr {
	u_int32_t            saddr;
	u_int32_t            daddr;
	u_int8_t             zero;
	u_int8_t             protocol;
	u_int16_t            len;
};

static char _err_buf[ERRBUFSIZE + 1] = {0};

/*
 *   return the error message if packet_xxx failed
 */
const char *packet_error(void)
{
	return _err_buf;
}

/*
 *   the checksum algorithm for IP packet or UDP/TCP packet
 */
u_int16_t packet_checksum(const u_int16_t *ptr, int nbytes)
{
	register long           sum;
	register u_int16_t      answer;
	u_int16_t               oddbyte;

	sum = 0;
	while (nbytes > 1)  {
		sum += *ptr++;
		nbytes -= 2;
	}

	if (nbytes == 1) {
		oddbyte = 0;
		*((u_char *) &oddbyte) = *(u_char *)ptr;
		sum += oddbyte;
	}

	sum  = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	answer = ~sum;

	return(answer);
}

u_int32_t packet_aton(const char *address)
{
	u_int32_t     addr;

	if (!address) {
		memset(_err_buf, 0, sizeof(_err_buf));
		snprintf(_err_buf, ERRBUFSIZE, "%s: invalid parameter\n", __FUNCTION__);
		return -1;
	}

	if (inet_pton(AF_INET, address, &addr) <= 0) {
		memset(_err_buf, 0, sizeof(_err_buf));
		PACKET_ERROR;
		return -1;
	}

	return addr;
}

/*
 *   create a PF_PACKET socket, @type is SOCK_DGRAM or SOCK_RAW, protocol
 *   using ETH_P_XXX(see linux/if_ether.h>)
 * 
 *   return socket fd if OK, -1 on error
 */
int packet_socket(int type, int protocol)
{
	int            fd = -1;

	fd = socket(PF_PACKET, type, htons(protocol));
	if (fd < 0) {
		memset(_err_buf, 0, sizeof(_err_buf));
		PACKET_ERROR;
		return -1;
	}

	return fd;
}


/*
 *   send a SOCK_RAW packet though @fd, outgoing interface is @ifname, destination
 *   ethernet address is @hwaddr, data is store in @buf.
 *   the @fd type is SOCK_RAW
 *
 *   return 0 if send OK, -1 on error
 */
int packet_send_raw(
    int                      fd,
    u_int8_t                *buf,
    int                      buf_len,
    int                      ifindex,
    const char               *dmac)
{
    struct sockaddr_ll       addr;
    struct ether_addr        hwaddr;

    if (fd < 0 || !buf || !dmac) {
	memset(_err_buf, 0, sizeof(_err_buf));
	snprintf(_err_buf, ERRBUFSIZE, "%s: invalid parameter", __FUNCTION__);
	return -1;
    }
    
    if (buf_len < ETH_ZLEN) {
	memset(_err_buf, 0, sizeof(_err_buf));
	snprintf(_err_buf, ERRBUFSIZE, "%s: packet size too small", __FUNCTION__);
	return -1;
    }

    if (ether_aton_r(dmac, &hwaddr) == NULL) {
	memset(_err_buf, 0, sizeof(_err_buf));
	PACKET_ERROR;
	return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sll_family = AF_PACKET;
    memcpy(addr.sll_addr, &hwaddr, ETH_ALEN);
    addr.sll_halen = ETH_ALEN;
    addr.sll_ifindex = ifindex;
    
    if (sendto(fd, buf, buf_len, 0, (struct sockaddr*)&addr,
	       sizeof(addr)) != buf_len) {
	memset(_err_buf, 0, sizeof(_err_buf));
	PACKET_ERROR;
	return -1;
    }

    return 0;
}

/*
 *   send a SOCK_DGRAM packet though @fd, outgoing interface is @ifname, destination
 *   ethernet address is @hwaddr, data is store in @buf.
 *   the @fd type is SOCK_DGRAM
 *
 *   return 0 if send OK, -1 on error
 */
int packet_send_datagram(
    int                      fd,
    u_int8_t                 *buf,
    int                      buf_len,
    int                      ifindex,
    const char               *dmac,
    int                      type)
{
    struct sockaddr_ll       addr;
    struct ether_addr        hwaddr;

    if (fd < 0 || !buf || !dmac) {
	memset(_err_buf, 0, sizeof(_err_buf));
	snprintf(_err_buf, ERRBUFSIZE, "%s: invalid parameter", __FUNCTION__);
	return -1;
    }
    
    if (buf_len < ETH_ZLEN - ETH_HLEN) {
	memset(_err_buf, 0, sizeof(_err_buf));
	snprintf(_err_buf, ERRBUFSIZE, "%s: packet size too small", __FUNCTION__);
	return -1;
    }

    if (ether_aton_r(dmac, &hwaddr) == NULL) {
	memset(_err_buf, 0, sizeof(_err_buf));
	PACKET_ERROR;
	return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sll_family = AF_PACKET;
    memcpy(addr.sll_addr, &hwaddr, ETH_ALEN);
    addr.sll_halen = ETH_ALEN;
    addr.sll_ifindex = ifindex;
    addr.sll_protocol = htons(type);
    
    if (sendto(fd, buf, buf_len, 0, (struct sockaddr*)&addr,
	       sizeof(addr)) != buf_len) {
	memset(_err_buf, 0, sizeof(_err_buf));
	PACKET_ERROR;
	return -1;
    }

    return 0;
}


/*
 *   create a Ethernet packet using <@dmac, @smac, @proto>, the Ethernet data is store in @buf
 *   all data
 *
 *   return the IP packet size if OK, -1 on error
 */
int packet_ether(
    const u_int8_t           *buf, 
    int                      buf_len, 
    u_int8_t                 *pkt,
    int                      pkt_len,
    const char               *smac,
    const char               *dmac,
    int                      proto)
{
    struct ether_header      *eth;
    struct ether_addr        saddr, daddr;

    if (!buf || !pkt || !smac || !dmac) {
	memset(_err_buf, 0, sizeof(_err_buf));
	snprintf(_err_buf, ERRBUFSIZE, "packet_ether error: invalid parameter\n");
	return -1;
    }

    if (pkt_len < buf_len + sizeof(struct ether_header)) {
	memset(_err_buf, 0, sizeof(_err_buf));
	snprintf(_err_buf, ERRBUFSIZE, "packet_ether error: not enough room\n");
	return -1;
    }

    if (pkt_len < ETH_ZLEN) {
	memset(_err_buf, 0, sizeof(_err_buf));
	snprintf(_err_buf, ERRBUFSIZE, 
		 "packet_ether error: ether packet must great than or equal 60 bytes\n");
	return -1;
    }

    if (ether_aton_r(smac, &saddr) == NULL) {
	memset(_err_buf, 0, sizeof(_err_buf));
	PACKET_ERROR;
	return -1;
    }

    if (ether_aton_r(dmac, &daddr) == NULL) {
	memset(_err_buf, 0, sizeof(_err_buf));
	PACKET_ERROR;
	return -1;
    }

    /* copy buf to pkt + ether header len, use memmove */
    memmove(pkt + sizeof(struct ether_header), buf, buf_len);

    /* set ether header */
    memset(pkt, 0, sizeof(struct ether_header));
    eth = (struct ether_header *)pkt;
    memcpy(eth->ether_dhost, &daddr, ETH_ALEN);
    memcpy(eth->ether_shost, &saddr, ETH_ALEN);
    eth->ether_type = htons(proto);

    return (buf_len + sizeof(struct ether_header));
}

/*
 *   create a IP packet using @saddr and @daddr, the IP data is store in @buf
 *
 *   return the IP packet size if OK, -1 on error
 */
int packet_ip(
    const u_int8_t           *buf, 
    int                      buf_len, 
    u_int8_t                 *pkt,
    int                      pkt_len,
    u_int32_t                sip,
    u_int32_t                dip,
    u_int8_t                 protocol)
{
    struct iphdr             *iphdr = NULL;

    if (!buf || !pkt) {
	memset(_err_buf, 0, sizeof(_err_buf));
	snprintf(_err_buf, ERRBUFSIZE, "%s: invalid parameter\n", __FUNCTION__);
	return -1;
    }

    if (pkt_len < buf_len + sizeof(struct iphdr)) {
	memset(_err_buf, 0, sizeof(_err_buf));
	snprintf(_err_buf, ERRBUFSIZE, "%s: not enough room\n", __FUNCTION__);
	return -1;
    }

    /* copy buf to pkt + iphdr len, use memmove */
    memmove(pkt + sizeof(struct iphdr), buf, buf_len);

    /* set ip header */
    memset(pkt, 0, sizeof(struct iphdr));
    iphdr = (struct iphdr*)pkt;
    iphdr->version  = IPVERSION;
    iphdr->ihl      = sizeof(struct iphdr) / 4;
    iphdr->tos      = 0;
    iphdr->tot_len  = htons(buf_len + sizeof(struct iphdr));
    iphdr->id       = htons(188);
    iphdr->frag_off = htons(0);
    iphdr->ttl      = IPDEFTTL;
    iphdr->protocol = protocol;
    iphdr->saddr    = sip;
    iphdr->daddr    = dip;
    iphdr->check    = packet_checksum((u_int16_t*)iphdr, sizeof(struct iphdr));
    

    return (buf_len + sizeof(struct iphdr));
}


/*
 *   create a ICMP echo message using @buf as data. store ICMP message in @pkt
 *   
 *   @type                ICMP type must be ICMP_ECHO or ICMP_ECHOREPLY
 *   @id                  the ICMP identifier
 *   @sequence            the ICMP sequence
 */
int packet_icmp_echo(
    const u_int8_t *buf,
    int            buf_len,
    u_int8_t       *pkt,
    int            pkt_len,
    u_int8_t       type,
    u_int16_t      id,
    u_int16_t      sequence)
{
    struct icmphdr           *icmp = NULL;

    /* verify parameter */
    if (!buf || !pkt || buf_len <= 0 || pkt_len <= 0) {
	memset(_err_buf, 0, sizeof(_err_buf));
	snprintf(_err_buf, ERRBUFSIZE, "%s: invalid parameter\n", __FUNCTION__);
	return -1;
    }

    /* type must be ICMP_ECHOREPLY or ICMP_ECHO */
    if (type != ICMP_ECHOREPLY && type != ICMP_ECHO) {
	memset(_err_buf, 0, sizeof(_err_buf));
	snprintf(_err_buf, ERRBUFSIZE, "%s: invalid parameter\n", __FUNCTION__);
	return -1;
    }

    /* verify pkt have enough room */
    if (pkt_len < buf_len + sizeof(struct icmphdr)) {
	memset(_err_buf, 0, sizeof(_err_buf));
	snprintf(_err_buf, ERRBUFSIZE, "%s: not enough room\n", __FUNCTION__);
	return -1;
    }
    
    /* copy buf to pkt after icmphdr, use memmove not memcpy, see man */
    memmove(pkt + sizeof(struct icmphdr), buf, buf_len);

    memset(pkt, 0, sizeof(struct icmphdr));
    icmp = (struct icmphdr *)pkt;
    icmp->type             = type;
    icmp->code             = 0;
    icmp->un.echo.id       = htons(id);
    icmp->un.echo.sequence = htons(sequence);
    icmp->checksum         = packet_checksum((u_int16_t *)pkt, 
					buf_len + sizeof(struct icmphdr));

    return (buf_len + sizeof(struct icmphdr));
}


/*
 *   create a TCP packet using <@saddr, @sport> and <@daddr, @dport>, the
 *   TCP packet data is in @buf, TCP packet stored is in @packet
 *
 *   return the TCP packet size if OK, -1 on error
 */
int packet_tcp(
    const u_int8_t           *buf, 
    int                      buf_len,
    u_int8_t                 *pkt,
    int                      pkt_len,
    u_int16_t                sport,
    u_int16_t                dport,
    int                      type)
{
    return -1;
}

/*
 *   create a UCP packet using <@saddr, @sport> and <@daddr, @dport>, the
 *   UCP packet data is in @buf, TCP packet stored is in @packet
 *
 *   return the TCP packet size if OK, -1 on error
 */
int packet_udp(
    const u_int8_t           *buf, 
    int                      buf_len,
    u_int8_t                 *pkt,
    int                      pkt_len,
    u_int16_t                sport,
    u_int16_t                dport,
    u_int32_t                sip,       /* here is use in persude header for checksum */
    u_int32_t                dip,
    u_int8_t                 protocol)
{
    struct udphdr            *hdr;
    u_int8_t                 *ptr;
    struct psehdr            *psehdr;
    int                      pse_len;
    
    if (!buf || !pkt || buf_len <= 0 || pkt_len <= 0) {
	memset(_err_buf, 0, sizeof(_err_buf));
	snprintf(_err_buf, ERRBUFSIZE, "%s: invalid parameter\n", __FUNCTION__);
	return -1;
    }

    if (pkt_len < buf_len + sizeof(struct udphdr)) {
	memset(_err_buf, 0, sizeof(_err_buf));
	snprintf(_err_buf, ERRBUFSIZE, "%s: not enough room\n", __FUNCTION__);
	return -1;
    }
    
    /* copy buf to pkt + udp header len, use memmove */
    memcpy(pkt + sizeof(struct udphdr), buf, buf_len);

    memset(pkt, 0, sizeof(struct udphdr));
    hdr = (struct udphdr*)pkt;
    hdr->source = htons(sport);
    hdr->dest   = htons(dport);
    hdr->len    = htons(buf_len + sizeof(struct udphdr));

    
    /* UDP checksum */
    pse_len = buf_len + sizeof(struct psehdr) + sizeof(struct udphdr);
    ptr = malloc(pse_len);
    if (!ptr) {
	memset(_err_buf, 0, sizeof(_err_buf));
	snprintf(_err_buf, ERRBUFSIZE, "%s: not enough room\n", __FUNCTION__);
	return -1;
    }
    memset(ptr, 0, pse_len);
    
    psehdr = (struct psehdr*)ptr;
    psehdr->saddr    = sip;
    psehdr->daddr    = dip;
    psehdr->protocol = protocol;
    psehdr->len      = hdr->len;
    
    memcpy(ptr + sizeof(struct psehdr), pkt, buf_len + sizeof(struct udphdr));

    hdr->check = packet_checksum((u_int16_t*)ptr, pse_len);
    
    free(ptr);
    return (buf_len + sizeof(struct udphdr));
}

/*
 *    create a ARP packet using <@smac, @sip, @dmac, @dip> and @type,
 *    type is ARP type
 *
 *    return packet size if OK, -1 on error
 */
int packet_arp(
    u_int8_t                 *pkt,
    int                      pkt_len,
    const char               *smac,
    u_int32_t                sip,
    const char               *dmac,
    u_int32_t                dip,
    int                      type)
{
    struct ether_arp   *arp = NULL;
    struct ether_addr  saddr, daddr;

    if (!pkt) {
	memset(_err_buf, 0, sizeof(_err_buf));
	snprintf(_err_buf, ERRBUFSIZE, "%s: invalid parameter\n", __FUNCTION__);
	return -1;
    }

    if (pkt_len < sizeof(struct ether_arp)) {
	memset(_err_buf, 0, sizeof(_err_buf));
	snprintf(_err_buf, ERRBUFSIZE, "%s: no enough room\n", __FUNCTION__);
	return -1;
    }
    
    if (smac) {
	if (ether_aton_r(smac, &saddr) == NULL) {
	    memset(_err_buf, 0, sizeof(_err_buf));
	    PACKET_ERROR;
	    return -1;
	}
    }

    if (dmac) {
	if (ether_aton_r(dmac, &daddr) == NULL) {
	    memset(_err_buf, 0, sizeof(_err_buf));
	    PACKET_ERROR;
	    return -1;
	}
    }

    memset(pkt, 0, pkt_len);
    arp = (struct ether_arp*)pkt;
    arp->arp_hrd = htons(ARPHRD_ETHER);
    arp->arp_pro = htons(ETHERTYPE_IP);
    arp->arp_hln = ETH_ALEN;
    arp->arp_pln = IP_ALEN;
    arp->arp_op  = htons(type);

    if (smac)
	memcpy(arp->arp_sha, &saddr, ETH_ALEN);
    memcpy(arp->arp_spa, &sip, IP_ALEN);
    if (dmac)
	memcpy(arp->arp_tha, &daddr, ETH_ALEN);
    memcpy(arp->arp_tpa, &dip, IP_ALEN);

    return (sizeof(struct ether_arp));
}


    
