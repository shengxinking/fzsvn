/*
 *  this is a simple capture program using libpcap
 *
 *  write by Forrest.zhang
 */

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pcap.h>

int main(void)
{
    char                   *device;
    pcap_t                 *pp;
    struct bpf_program     fcode;
    char                   errbuf[PCAP_ERRBUF_SIZE + 1] = {0};
    struct pcap_pkthdr     pkthdr;
    char                   *pkt;
    
    device = pcap_lookupdev(errbuf);
    if (!device) {
	printf("no device found: %s\n", errbuf);
	return -1;
    }
    printf("device %s found\n", device);

    pp = pcap_open_live(device, 8000, 1, 500, errbuf);
    if (!pp) {
	printf("can't open device %s: %s\n", device, errbuf);
	return -1;
    }
    
    pcap_compile(pp, &fcode, NULL, 0, 0);
    
    pcap_setfilter(pp, &fcode);

    do {
	pkt = (char*)(pcap_next(pp, &pkthdr));
    } while (0);
    
    pcap_close(pp);

    return 0;
}
