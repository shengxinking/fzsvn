/*
 *
 *
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

int f2(void)
{
    struct stat buf;

    if (stat("/etc/issue", &buf)) {
	perror("stat error: ");
	return -1;
    }
    else
	printf("/etc/issue exist\n");
}
