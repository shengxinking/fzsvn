/*
 *
 *
 *
 */

#include <iostream>

using namespace std;

class A {

public:
    
    // members
    virtual void print() const;

};

inline void A::print() const
{
    cout << __PRETTY_FUNCTION__ << endl;
}

class B : public A {

public:

    // members
    void print() const;

};

inline void B::print() const
{
    cout << __PRETTY_FUNCTION__ << endl;
}


int main(void) 
{
    A*        pa = new B();

    B*        pb = dynamic_cast<B*>(pa);

    if (pb)
	cout << "can dynamic_cast from A* to B*" << endl;
    else
	cout << "can't dynamic_cast from A* to B*" << endl;

    return 0;
}
