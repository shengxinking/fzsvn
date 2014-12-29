/*
 *
 *
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

static void usage(void)
{
    printf("\ntest_open <file name>\n");
}

int main(int argc, char** argv)
{
    int         fd;
    
    if (argc != 2) {
	usage();
	return -1;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
	printf("open %s failor\n", argv[1]);
	return -1;
    }

    sleep(100);

    close(fd);
    
    return 0;
}
