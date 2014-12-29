/*
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main(void)
{
    printf("real uid: %d, real gid: %d\n", getuid(), getgid());
    printf("effective uid: %d, effective gid: %d\n", geteuid(), getegid());

    exit(0);
}
