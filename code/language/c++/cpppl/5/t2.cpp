/*
 *  test align for variable
 *
 *  write by Forrest.zhang
 */

#include <iostream>

using namespace std;

int
main(void)
{
    char        c = 0;
    int*        pi;
    char        c1;
    short       c2;
    char*       pc = c;

    cout << "char's address: " << &c << endl;
    cout << "int*'s address: " << &pi << endl;
    cout << "short's address: " << &c2 << endl;
    cout << "char*'s address: " << &pc << endl;

    return 0;
}

