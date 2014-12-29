/*
 *
 *
 *
 */

#include <iostream>
using namespace std;

class X {
    public:
    static const int c1 = 7;
    static int c2 ;
    const int c3;
    int&      c4;
    static const float c5 = 7.1;
};

const int X::c1;
int X::c2 = 15;

int main(void)
{
    X   x;

    cout << X::c1 << endl;
    cout << X::c5 << endl;
    cout << X::c2 << endl;

    return 0;
}
