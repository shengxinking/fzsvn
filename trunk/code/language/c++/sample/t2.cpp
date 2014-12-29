/*
 *
 *
 *
 */

#include <iostream>

using namespace std;

class A {
    public:
    
    virtual void f1() {cout << "in A::f1()" << endl; }

    void print() {cout << "in A::print()" << endl; }

};

class B : public A {
    public:

    void f1() {cout << "in B::f1()" << endl; }

};

int main()
{
    A*      a = new B();

    a->f1();
    a->print();

    return 0;
}
