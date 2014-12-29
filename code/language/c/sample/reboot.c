/*
 *  this program test reboot in linux
 *
 *  write by Forrest.zhang
 */

#include <sys/reboot.h>
#include <stdio.h>

int main(void)
{
    printf("reboot system now! ...\n");
    
    reboot(RB_AUTOBOOT);

    return 0;
}

