/*
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char** argv)
{
    struct stat		    buf;

    if (argc != 2) {
	printf("usage: %s <filename>", argv[0]);
	exit(1);
    }

    if (stat(argv[1], &buf) < 0) {
	printf("can't get stat of file %s\n", argv[1]);
	perror("");
	exit(1);
    }
    
    if (chmod(argv[1], (buf.st_mode & ~S_IXGRP) | S_ISGID | S_ISVTX) < 0) {
	printf("can't chmod of file %s\n", argv[1]);
	perror("");
	exit(1);
    }

    sleep(2);
    
    if (chmod(argv[1], 0644) < 0) {
	printf("can't chmod of file %s\n", argv[1]);
	perror("");
	exit(1);
    }

    exit(0);
}
