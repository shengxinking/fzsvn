/*
 * @file	test.c
 * @brief	test program for netfilter APIs
 *
 * @author	Forrest.zhang
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "netfilter.h"
#include "debug.h"
#include "sock.h"

int main(void)
{
	struct sockaddr_in oaddr, saddr;
	socklen_t len;
	int fd, clifd;

	fd = sock_tcpsvr(0, 8000, 0);
	if (fd < 0) {
		ERR("create socket error\n");
		return -1;
	}

	len = sizeof(saddr);
	while (1) {
		clifd = accept(fd, (struct sockaddr *)&saddr, &len);
		if (clifd > 0) {
			if (nf_origin_dst(clifd, &oaddr)) {
				ERR("get origin address error\n");
//				close(clifd);
//				break;
			}

			printf("client(%d) %u.%u.%u.%u:%u->%u.%u.%u.%u:%u connect\n", clifd,
				NIPQUAD(saddr.sin_addr), ntohs(saddr.sin_port),
				NIPQUAD(oaddr.sin_addr), ntohs(oaddr.sin_port));

			close(clifd);
		}
		else 
			break;
	}

	close(fd);

	return 0;
}




