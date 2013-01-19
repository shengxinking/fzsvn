/*
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

static void sig_io(int);
static void sig_urg(int);

static int  fd;

int main(int argc, char** argv)
{
    int		mode;

    signal(SIGIO, sig_io);
    signal(SIGURG, sig_urg);

    if (fcntl(STDIN_FILENO, F_SETOWN, getpid()) < 0) {
	printf("fcntl SETOWN for pid error\n");
	perror(NULL);
	exit(0);
    }

/*    if (fcntl(STDIN_FILENO, F_SETOWN, -(getgid())) < 0) {
	printf("fcntl SETOWN for gid error\n");
	perror(NULL);
	exit(0);
    }
*/    
    if (fcntl(STDIN_FILENO, F_GETFL, mode) < 0) {
	printf("fcntl SETOWN for gid error\n");
	perror(NULL);
	exit(0);
    }
    
    mode = mode | O_ASYNC | O_NONBLOCK;

    if (fcntl(STDIN_FILENO, F_SETFL, mode) < 0) {
	printf("fcntl SETOWN for gid error\n");
	perror(NULL);
	exit(0);
    }
    
    if ( (fd = open(argv[1], O_RDWR)) < 0) {
	printf("open %s error\n", argv[1]);
	perror(NULL);
	exit(1);
    }
    
    while(1)
	;

    exit(0);
}

void sig_io(int signo)
{
    char	buf[1024];
    ssize_t	len;
    
    write(fd, "I recieved SIGIO\n", 18);
    len = read(STDIN_FILENO, buf, 1024);
    
    if (len < 0) {
	printf("read data from stdin error\n");
	perror(NULL);
	exit(1);
    }
    else if (len == 0) {
	write(fd, "end by user\n", 13);
	exit(0);
    }
    else {
	buf[len] = 0;
	write(fd, buf, len);
	write(fd, "\n", 1);
    }
}

void sig_urg(int signo)
{
    write(fd, "I received SIGURG\n", 18);
}
