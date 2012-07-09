/*
 *
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

int main(void)
{
    if (open(".tmpfile", O_RDONLY | O_CREAT | O_EXCL, S_IRWXU) < 0) {
	printf("create file .tmpfile error:");
	perror("");
	exit(0);
    }

    sleep(15);

    if (unlink(".tmpfile") < 0) {
	printf("unlink file .tmpfile error:");
	perror("");
	exit(0);
    }

    exit(0);
}
