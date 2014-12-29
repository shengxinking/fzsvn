/*
 *  test get command parameter from a command string
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    char*      cmd;
    char*      args[1024];
    char*      s;
    char*      cmdpath;
    char*      command = "/sbin/agetty tty2 9600";
    int        i;

    cmd = strdup(command);
    if (cmd == NULL) {
	printf("strdup() failed\n");
	return -1;
    }
    
    for (s = cmd, i = 0; (s = strsep(&cmd, " \t")) != NULL;) {
	if (*s != '\0') {
	    args[i] = s;
	    s++;
	    i++;
	}
    } 
    args[i] = NULL;
    
    cmdpath = args[0];
    printf("Start %s.\n", cmdpath);
    
    i = 0;
    while(args[i]) {
	printf("%d: %s\n", i, args[i]);
	i++;
    }

    return 0;
}




