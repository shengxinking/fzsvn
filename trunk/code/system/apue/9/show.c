/*
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main(void)
{
    printf("pid = %d, gid = %d\n", getpid(), getpgrp());
    exit(0);
}
