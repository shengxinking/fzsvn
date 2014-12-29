/*
 *	@file	af_admin_test.c
 *	@brief	test program for AF_ADMIN socket
 *
 *	@author	Forrest.zhang
 */

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "af_admin.h"

static void _usage(void)
{
}

static int _parse_cmd(int argc, char **argv)
{
	return 0;
}

static int _init(void)
{
	return 0;
}


static void _release(void)
{
}

int main(int argc, char **argv)
{
	int fd;
	struct sockaddr_admin	admin, cli;
	struct af_message *msg;
	int len;
	char *buffer;
	
	fd = socket(AF_ADMIN, SOCK_DGRAM, 0);
	if (fd < 0) {
		printf("socket(AF_ADMIN) error: %s\n", strerror(errno));
		return -1;
	}

	memset(&admin, 0, sizeof(admin));
	admin.family = AF_ADMIN;
	admin.pid = 0;
	admin.group = 0;
	
	if (bind(fd, (struct sockaddr *)&admin, sizeof(admin))) {
		perror("bind: ");
		return -1;
	}

	len = sizeof(struct af_message) + strlen("Hello") + 1;
	buffer = malloc(len);
	if (!buffer) {
		perror("malloc: ");
		return -1;
	}
	
	msg = (struct af_message *)buffer;
	msg->type = 0;
	msg->len = 6;
	memcpy(msg->data, "Hello", 6);

	cli.family = AF_ADMIN;
	cli.pid = 0;
	cli.group = 0;
	
	if (sendto(fd, buffer, len, MSG_DONTWAIT, (struct sockaddr*)&cli, sizeof(cli)) != len) {
		perror("sendto %d\n");
		return -1;
	}
	
	printf("send Hello to svr\n");
	
	close(fd);

	return 0;
}

