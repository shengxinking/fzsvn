/*
 *
 *
 */

#include <stdio.h>

struct A {
    int         i;
    char        c;
    int         j;
    short       k;
    char        d;
};


int main()
{
    struct A         a;

    printf("struct A's size is: %d\n", (int)sizeof(a));
    
    return 0;
}
