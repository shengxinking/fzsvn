/*
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char** argv, char** envp)
{
    int		    i;
    
    i = 1;
    while (argv[i] != NULL) { 
	printf("args %d is: %s.\n", i, argv[i]);
	++i;
    }

    i = 0;
    while (envp[i] != NULL) {
	printf("environ variable: %s.\n", envp[i]);
	i++;
    }

    exit(0);
}
