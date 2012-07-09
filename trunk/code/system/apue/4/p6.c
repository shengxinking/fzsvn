/*
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <utime.h>

int main(int argc, char** argv)
{
    struct stat		statbuf;
    struct utimbuf	timebuf;

    if (argc != 2) {
	printf("usage: %s <filename>\n", argv[0]);
	exit(1);
    }

    if (lstat(argv[1], &statbuf) < 0) {
	printf("can't get stat of file %s:", argv[1]);
	perror("");
	exit(1);
    }

    printf("access time is: %ld\n", statbuf.st_atime);
    printf("modify time is: %ld\n", statbuf.st_mtime);
    printf("change time is: %ld\n", statbuf.st_ctime);

    timebuf.actime = 100;
    timebuf.modtime = 100;
    if (utime(argv[1], &timebuf) < 0) {
	printf("can't change time of file %s:", argv[1]);
	fflush(stdout);
	perror("");
	exit(1);
    }

    exit(0);
}
