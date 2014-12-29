/*
 * 
 *
 */

#include <stdlib.h>

int main(void)
{
    void         *p;

    p = malloc(10);

    printf("malloc %d bytes\n", sizeof(p));

    free(p);

    return 0;
}
