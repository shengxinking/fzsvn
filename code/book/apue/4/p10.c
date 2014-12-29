/*
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char** argv)
{
    struct stat		    buf;
    int			    i;

    for (i = 1; i < argc; ++i) {
	if (lstat(argv[i], &buf) < 0) {
	    printf("can't stat file %s\n", argv[i]);
	    perror("");
	    continue;
	}

	printf("major = %d, minor = %d\n", major(buf.st_dev), minor(buf.st_dev));

	if (S_ISCHR(buf.st_mode) || S_ISBLK(buf.st_mode))
	    printf("st_rdev = %d, major = %d, minor = %d\n", buf.st_rdev,
		    major(buf.st_rdev), 
		    minor(buf.st_rdev));
    }

    exit(0);
}
