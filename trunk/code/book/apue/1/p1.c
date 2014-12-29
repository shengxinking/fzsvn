/*
 *  write by jbug
 */
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
    DIR*	    dp;
    struct dirent*  dirp;

    if (argc != 2) {
	printf("usage: %s <filename>\n", argv[0]);
	exit(1);
    }

    if ( (dp = opendir(argv[1])) == NULL) {
	printf("error: can't open directory: %s\n", argv[1]);
	perror("reason:");
	exit(1);
    }

    while ( (dirp = readdir(dp)) != NULL)
	printf("\t%s\n", dirp->d_name);

    closedir(dp);

    exit(0);
}

