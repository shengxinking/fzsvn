/*
 *
 */

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char** argv)
{
    char	    *ptr;

    if (argc != 2) {
	printf("usage: %s <ENVIREMENT NAME>\n", argv[0]);
	exit(1);
    }

    if ( (ptr = getenv(argv[1])) != NULL)
	printf("%s = %s\n", argv[1], ptr);
    else
	printf("%s undefined\n", argv[1]);

    //    exit(0);
}
