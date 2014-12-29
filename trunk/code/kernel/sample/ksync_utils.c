/*
 *  tools control kernel ksync behave
 *
 *  write by Forrest.zhang
 */

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "ksync.h"
#include <netinet/in.h>

#define  SYNC_DEV             "/dev/ksync"

static void usage(void)
{
    printf("ksync_utils   tools to control ksync kernel module\n");
    printf("     stop                             stop ksync thread\n");
    printf("     master <address> <port> [delay]  start ksync thread as master, it send\n");
    printf("                                      udp packet to address:port every delay\n");
    printf("                                      second\n");
    printf("     slave <address> <port>           start ksync thread as slave, it receive\n");
    printf("                                      udp packet on address:port\n");
}

int main(int argc, char** argv)
{
    int            fd = -1;
    unsigned long  port = 0;
    unsigned long  delay = 0;
    struct in_addr addr;

    if (argc < 2) {
	usage();
	return -1;
    }

    fd = open(SYNC_DEV, O_RDONLY);
    if (fd < 0) {
	printf("open %s error: %s\n", SYNC_DEV, strerror(errno));
	return -1;
    }

    /* stop */
    if (strcmp(argv[1], "stop") == 0) {
	if (argc != 2) {
	    usage();
	    return -1;
	}

	ioctl(fd, IO_SET_MODE, SYNC_STOP);
    }

    /* master */
    if (strcmp(argv[1], "master") == 0) {
	if (argc != 4 && argc != 5) {
	    usage();
	    return -1;
	}
	
	if (inet_pton(AF_INET, argv[2], &addr) < 0) {
	    printf("invalid address: %s\n");
	    return -1;
	}

	port = atol(argv[3]);
	if (argc == 5)
	    delay = atol(argv[4]);
	
	ioctl(fd, IO_SET_ADDRESS, addr);
	ioctl(fd, IO_SET_PORT, port);
	if (delay > 0)
	    ioctl(fd, IO_SET_DELAY, delay);

	ioctl(fd, IO_SET_MODE, SYNC_MASTER);
	
    }

    /* slave */
    if (strcmp(argv[1], "slave") == 0) {
	if (argc != 4) {
	    usage();
	    return -1;
	}

	if (inet_pton(AF_INET, argv[2], &addr) < 0) {
	    printf("invalid address: %s\n");
	    return -1;
	}
	port = atol(argv[3]);

	ioctl(fd, IO_SET_ADDRESS, addr);
	ioctl(fd, IO_SET_PORT, port);

	ioctl(fd, IO_SET_MODE, SYNC_SLAVE);
    }

    close(fd);

    return 0;
}
