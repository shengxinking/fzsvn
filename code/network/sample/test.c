/*
 *
 */

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <signal.h>

static void sig_alrm(int);

int main()
{
    struct sockaddr_in	sa;
    int			fd;
    char		buf[8192];
    
    bzero(&sa, sizeof(sa));

    if ( (fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	printf("create UDP socket error\n");
	perror(NULL);
	exit(1);
    }

    signal(SIGALRM, sig_alrm);

    while (1) {
	alarm(1);
	if (recvfrom(fd, buf, 8192, 0, NULL, NULL) < 0) {
	    if (errno == EINTR)
		printf("interrupt by alarm\n");
	    else
		printf("recv data error\n");
	}
	alarm(0);
    }

    return 0;
}

void sig_alrm(int signo)
{
    printf("receive SIGALRM\n");
    fflush(stdout);
    alarm(1);
    return;
}
