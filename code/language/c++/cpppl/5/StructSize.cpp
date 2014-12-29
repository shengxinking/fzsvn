/*
 *  this program test struct size for align in address edge
 *
 *  write by Forrest.zhang
 */

#include <iostream>

using namespace std;

struct S {
    char        b;
    bool        is;
    short       c;
    char        carr[3];
    int         l;
    int         s;
    char*       pc;
};

int main(void)
{
    S          s1;
    cout << "S size is: " << sizeof(S) << endl;

    return 0;
}
