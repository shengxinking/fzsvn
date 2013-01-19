/*
 * write by jbug
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>


int main(int argc, char** argv)
{
    struct stat		    buf;
    
    if (argc != 2) {
	printf("usage: %s <filename>\n", argv[0]);
	exit(1);
    }

    if (lstat(argv[1], &buf)) {
	printf("error: stat file %s\n", argv[0]);
	perror("     : ");
	exit(1);
    }

    printf("st_mode: %o\n", buf.st_mode);
    printf("st_ino: %d\n", buf.st_ino);
    printf("st_dev: %d\n", buf.st_dev);
    printf("st_rdev: %d\n", buf.st_rdev);
    printf("st_nlink: %d\n", buf.st_nlink);
    printf("st_uid: %d\n", buf.st_uid);
    printf("st_gid: %d\n", buf.st_gid);
    printf("st_size: %d\n", buf.st_size);
    printf("st_atime: %d\n", buf.st_atime);
    printf("st_ctime: %d\n", buf.st_ctime);
    printf("st_mtime: %d\n", buf.st_mtime);
    printf("st_blksize: %d\n", buf.st_blksize);
    printf("st_blocks: %d\n", buf.st_blocks);
    
    
    if (S_ISREG(buf.st_mode)) printf("%s is a regular file\n", argv[1]);
    if (S_ISDIR(buf.st_mode)) printf("%s is a directory file\n", argv[1]);
    if (S_ISCHR(buf.st_mode)) printf("%s is a character file\n", argv[1]);
    if (S_ISBLK(buf.st_mode)) printf("%s is a block file\n", argv[1]);
    if (S_ISFIFO(buf.st_mode)) printf("%s is a FIFOs file\n", argv[1]);
    if (S_ISLNK(buf.st_mode)) printf("%s is a link file\n", argv[1]);
    if (S_ISSOCK(buf.st_mode)) printf("%s is a socket file\n", argv[1]);

    exit(0);
}
    
