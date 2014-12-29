/*
 * this is a Math Exception test program for Exception test
 *
 *
 * write by Forrest.zhang
 */

#ifndef __MATHERR_HPP__
#define __MATHERR_HPP__

#include <string>
#include <iostream>

using namespace std;

class Matherr {

public:
    // constructor
    Matherr(const string& msg);
    Matherr(const Matherr& e);
    
    // destructor
    virtual ~Matherr();

    // member function
    virtual void print_debug() const;

protected:
    string       msg;
};

class Overflow : public Matherr
{
public:
    // constructor
    Overflow(const string& msg);
    Overflow(const Overflow& e);

    // destructor
    ~Overflow();

    // member function
    void print_debug();
};

class Underflow : public Matherr {
public:
    // constructor
    Underflow(const string& msg);
    Underflow(const Underflow& e);

    // destructor
    ~Underflow();

    // member function
    void print_debug();
};

inline Matherr::Matherr(const string& _msg)
    : msg(_msg)
{
    cout << __PRETTY_FUNCTION__ << endl;
}

inline Matherr::Matherr(const Matherr& e)
{
    cout << __PRETTY_FUNCTION__ << endl;
    msg = e.msg;
}

inline Matherr::~Matherr()
{
    cout << __PRETTY_FUNCTION__ << endl;
}

inline void Matherr::print_debug() const
{
    cout << __PRETTY_FUNCTION__ << endl;
    cout << msg << endl;
}

inline Overflow::Overflow(const string& msg)
    : Matherr(msg)
{
    cout << __PRETTY_FUNCTION__ << endl;
}

inline Overflow::Overflow(const Overflow& e)
    : Matherr(e.msg)
{
    cout << __PRETTY_FUNCTION__ << endl;
}

inline Overflow::~Overflow()
{
    cout << __PRETTY_FUNCTION__ << endl;
}

inline void Overflow::print_debug()
{
    cout << __PRETTY_FUNCTION__ << endl;
    cout << msg << endl;
}

inline Underflow::Underflow(const string& msg)
    :Matherr(msg)
{
    cout << __PRETTY_FUNCTION__ << endl;
}

inline Underflow::Underflow(const Underflow& e)
    : Matherr(e.msg)
{
    cout << __PRETTY_FUNCTION__ << endl;
}

inline Underflow::~Underflow()
{
    cout << __PRETTY_FUNCTION__ << endl;
}

inline void Underflow::print_debug()
{
    cout << __PRETTY_FUNCTION__ << endl;
    cout << msg << endl;
}

#endif
