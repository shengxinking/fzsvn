/*
 *  test __attribute__ ((__packed)) gcc option
 *  
 *  write by Forrest.zhang
 */

#include <stdio.h>

struct no_packed {
    char      a;
    int       b;
    short     c;
    char      d;
    short     e;
    char      f;
};

struct packed {
    char      a;
    int       b;
    short     c;
    char      d;
    short     e;
    char      f;
} __attribute__ ((__packed__));

struct no_packed        var1 = {'a', 1, 1, 'a', 1, 'a'};
struct packed           var2 = {'b', 2, 2, 'b', 2, 'b'};

int main(void)
{
    int                     ret  = 0;

    printf("size of non-packed is %d\n", sizeof(var1));
    printf("size of packed is %d\n", sizeof(var2));

    return ret;
}



