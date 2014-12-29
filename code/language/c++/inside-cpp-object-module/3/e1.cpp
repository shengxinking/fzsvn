/*
 *  example 1 of chapter 3
 *  test the size of class without any data and it's virtual subclass
 *
 *  write by Forrest.zhang
 */

#include <iostream>

using namespace std;

class X {};
class Y : public virtual X {};
class Z : public virtual X {};
class A : public Y, public Z {};

int main(void)
{
    X             x;
    Y             y;
    Z             z;
    A             a;

    cout << "size of X yielded " << sizeof(x) << endl;
    cout << "size of Y yielded " << sizeof(y) << endl;
    cout << "size of Z yielded " << sizeof(z) << endl;
    cout << "size of A yielded " << sizeof(a) << endl;

    return 0;
}

