/*
 *
 *
 *
 */

#ifndef __ACCESS_HPP__
#define __ACCESS_HPP__

#include <iostream>

using namespace std;

class A {

public:
    // members
    void print() const;

protected:
    void debug() const;
};

inline void A::print() const 
{
    cout << __PRETTY_FUNCTION__ << endl;
}

inline void A::debug() const
{
    cout << __PRETTY_FUNCTION__ << endl;
}


class X : public A {
    
public:
    void print() const;

protected:
    void debug() const;

};

inline void X::print() const
{
    cout << __PRETTY_FUNCTION__ << endl;
}

inline void X::debug() const
{
    cout << __PRETTY_FUNCTION__ << endl;
}

class Y : protected A {
    
public:
    void print() const;

protected:
    void debug() const;

};

inline void Y::print() const
{
    cout << __PRETTY_FUNCTION__ << endl;
}

inline void Y::debug() const
{
    cout << __PRETTY_FUNCTION__ << endl;
}

class Z : private A {
    
public:
    void print() const;

protected:
    void debug() const;

};

inline void Z::print() const
{
    cout << __PRETTY_FUNCTION__ << endl;
}

inline void Z::debug() const
{
    cout << __PRETTY_FUNCTION__ << endl;
}


#endif
