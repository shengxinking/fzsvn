/*
 *
 *
 *
 */

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <strings.h>

int main(void)
{
    char         c = 'a';
    char         *str1 = "abc";

    switch (c) {
    case 'a':
	printf ("a\n");
	break;

    case 'b':
	if (strcasecmp(str1, "abc") == 0)
	    printf("str1 is abc\n");
	else if (strcasecmp(str1, "ccc") == 0)
	    printf("str1 is ccc\n");
	else
	    printf("str1 is not abc\n");
	
	break;
    default:
	break;
    }

    return 0;
}
	
