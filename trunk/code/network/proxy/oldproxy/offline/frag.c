#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "spp_frag3.h"
#include "spp_stream4.h"
#include "cap.h"

int waf_cb(session_info* info ,char* buf)
{
	printf(" %s:%d->",inet_ntoa(*(struct in_addr *)(&info->tuple.sip)),info->tuple.sport);
	printf(" %s:%d : %s\n",inet_ntoa(*(struct in_addr* ) (&info->tuple.dip)),info->tuple.dport, buf);
	return 0;
}

int main()
{
	InitPcap( 0 );
	SetPktProcessor();
	Frag3GlobalInit(NULL);
	Frag3Init(NULL);
	Stream4Init(NULL);
	InitStream4Pkt();
	reg_waf_cb(waf_cb);
	InterfaceThread(NULL);
	return 0;
}
