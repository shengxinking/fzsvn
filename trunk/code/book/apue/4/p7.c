/*
 * write by jbug
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>

#define PATHMAX		1024

struct file_cnt {
    long	cnt_reg;
    long	cnt_dir;
    long	cnt_blk;
    long	cnt_chr;
    long	cnt_lnk;
    long	cnt_fifo;
    long	cnt_sock;
    long	cnt_err;
};

static void do_path(const char* path, struct file_cnt* cnt);

#define INC_REG(x)  (++(*x).cnt_reg)
#define INC_DIR(x)  (++(*x).cnt_dir)
#define INC_BLK(x)  (++(*x).cnt_blk)
#define INC_CHR(x)  (++(*x).cnt_chr)
#define INC_LNK(x)  (++(x)->cnt_lnk)
#define INC_FIFO(x) (++(x)->cnt_fifo)
#define INC_SOCK(x) (++(x)->cnt_sock)
#define INC_ERR(x)  (++(*x).cnt_err)

int main(int argc, char** argv)
{
    struct file_cnt	    count = {0};
    long		    total;

    if (argc != 2) {
	printf("usage: %s <filename>\n", argv[0]);
	exit(1);
    }

    do_path(argv[1], &count);

    total = count.cnt_reg + count.cnt_dir + count.cnt_blk + count.cnt_chr +
	count.cnt_lnk + count.cnt_fifo + count.cnt_sock;
    printf("regular files = %7ld, %5.2f %%\n", count.cnt_reg, count.cnt_reg*100.0/total);
    printf("directory files = %7ld, %5.2f %%\n", count.cnt_dir, count.cnt_dir*100.0/total);
    printf("block files = %7ld, %5.2f %%\n", count.cnt_blk, count.cnt_blk*100.0/total);
    printf("character files = %7ld, %5.2f %%\n", count.cnt_chr, count.cnt_chr*100.0/total);
    printf("link files = %7ld, %5.2f %%\n", count.cnt_lnk, count.cnt_lnk*100.0/total);
    printf("FIFOs files = %7ld, %5.2f %%\n", count.cnt_fifo, count.cnt_fifo*100.0/total);
    printf("sock files = %7ld, %5.2f %%\n", count.cnt_sock, count.cnt_sock*100.0/total);
    printf("can't handle files = %7ld\n", count.cnt_err);

    exit(0);
}

void do_path(const char* path, struct file_cnt* cnt)
{
    struct stat		buf;
    char		newpath[PATHMAX];
    int			len;
    DIR*		dp;
    struct dirent*	dirp;
    
    if (lstat(path, &buf) < 0) {
	printf("can't stat file: %s\n", path);
	INC_ERR(cnt);
	return;
    }

    if (S_ISDIR(buf.st_mode)) {
	strcpy(newpath, path);
	len = strlen(newpath);
	if (newpath[len -1] != '/') {
	    newpath[len] = '/';
	    newpath[len + 1] = 0;
	}
	INC_DIR(cnt);
//	printf("\n%s\n", newpath);

	if ( (dp = opendir(newpath)) == NULL) {
	    printf("can't open directory %s\n", newpath);
	    perror("");
	    INC_ERR(cnt);
	    return;
	}

	while ( (dirp = readdir(dp)) != NULL) {
	    if (strcmp(dirp->d_name, ".") == 0 ||
		    strcmp(dirp->d_name, "..") == 0)
		continue;
	    len = strlen(newpath);
	    strcat(newpath, dirp->d_name);
	    do_path(newpath, cnt);
	    newpath[len] = 0;
	}
	closedir(dp);
    }
    else if(S_ISREG(buf.st_mode))
	INC_REG(cnt);
    else if(S_ISBLK(buf.st_mode))
	INC_BLK(cnt);
    else if(S_ISCHR(buf.st_mode))
	INC_CHR(cnt);
    else if(S_ISFIFO(buf.st_mode))
	INC_FIFO(cnt);
    else if(S_ISLNK(buf.st_mode))
	INC_LNK(cnt);
    else if(S_ISSOCK(buf.st_mode))
	INC_SOCK(cnt);
    else {
	printf("unknown file type\n");
	INC_ERR(cnt);
    }
    
//    printf("%s\n", path);
    return;
}
