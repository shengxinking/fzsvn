#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
int main()
{
     int i,sz;
     char buf[1024]="yelam server\n";
     long s,port=8000;
     long rd;
     struct sockaddr_in sa;

     sz=sizeof(sa);
     sa.sin_family=AF_INET;
     sa.sin_addr.s_addr=htonl(INADDR_ANY);
     sa.sin_port=htons(23);

     s=socket(PF_INET,SOCK_STREAM,0);
     bind(s,(struct sockaddr*)&sa,sz);
     listen(s,1);
     rd=accept(s,(struct sockaddr *)&sa,&sz); // accept() return a new socket
     for(;;){
	memset(buf,0x00,sizeof(buf));
     	read(rd,buf,sizeof(1024));
	printf("%s\n",buf);
     }
     return 0;
}

