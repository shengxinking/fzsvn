/*
 *
 *
 *
 */

typedef unsigned char        uchar;
typedef const unsigned char  cuchar;
typedef int*                 pint;
typedef char*                pchar;
typedef int*                 p7int[7];
typedef p7int*               pp7int;
typedef p7int                p87int[8];

int
main(void)
{
    uchar          uc = 1;
    cuchar         ucc = 'a';
    int            i = 111;
    pint           pi = &i;
    p7int          p7i = {0, 0, 0, 0, 0, 0, 0};
    pp7int         pp7i = &p7i;
    p87int         p87i;

    i = 112;

    return 0;
}
