/*
 *  the recv function using RAW socket
 * 
 *  write by Forrest.zhang
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <linux/if_ether.h>

int main(void)
{
	int              fd;
	int              nrecvs = 0;
	char             buf[1024] = {0};

	fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (fd < 0) {
		printf("create raw socket error: %s\n", strerror(errno));
		return -1;
	}

	while (1) {
		if (recvfrom(fd, buf, 1023, 0, NULL, NULL) > 0) {
			nrecvs++;
			printf("recv %d packet\n", nrecvs);
		}
	}

	printf("recv %d packets\n", nrecvs);

	return 0;
}






