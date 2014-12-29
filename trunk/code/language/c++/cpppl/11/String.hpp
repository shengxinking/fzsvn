/*
 *
 *
 *
 */

#ifndef __STRING_HPP__
#define __STRING_HPP__

#include <cstring>
#include <iostream>

using namespace std;

class String {

public:

    // constructor
    String();
    String(const char* str);
    String(const String& str);

    // destructor
    ~String();

    // members
    String& operator = (const char* str);
    String& operator = (const String& str);
    friend ostream& operator << (ostream& os, const String& str);

private:
    struct    Srep;
    Srep*     rep;
};

struct String::Srep {
    char*     s;
    int       n;
    int       nrefs;

    // constructor
    Srep(int n, const char* str);

    // destructor
    ~Srep();

    // members
    Srep* clone();
    void assign(int n, const char* str);

private:

    // prohibit copy and assign
    Srep(const Srep&);
    Srep& operator=(const Srep&);
};

inline String::Srep::Srep(int _n, const char* str)
    :n(_n), nrefs(1)
{
    s = new char[_n + 1];
    strcpy(s, str);
}

inline String::Srep::~Srep()
{
    delete[] s;
}

inline String::Srep* String::Srep::clone()
{
    if (nrefs == 1) return this;
    
    nrefs--;
    return new Srep(n, s);
}

inline void String::Srep::assign(int _n, const char* str)
{
    if (n != _n) {
	delete[] s;
	n = _n;
	s = new char[_n + 1];
    }
    strcpy(s, str);
}

inline String::String()
{
    rep = new Srep(0, "");
}

inline String::String(const char* str)
{
    rep = new Srep(strlen(str), str);
}

inline String::String(const String& str)
{
    str.rep->nrefs++;
    rep = str.rep;
}

inline String::~String()
{
    if (--rep->nrefs == 0) delete rep;
}

inline String& String::operator = (const char* str)
{
    if (rep->nrefs == 1)
	rep->assign(strlen(str), str);
    else {
	rep->nrefs--;
	rep = new Srep(strlen(str), str);
    }

    return *this;
}

inline String& String::operator = (const String& str)
{
    str.rep->nrefs++;
    if (--rep->nrefs == 0) delete rep;
    rep = str.rep;

    return *this;
}

inline ostream& operator << (ostream& os, const String& str)
{
    os << "content is: " << str.rep->s << "       ";
    os << "reference is: " << str.rep->nrefs;

    return os;
}

#endif
