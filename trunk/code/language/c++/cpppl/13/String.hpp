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

template <class T> class String {

public:

    // constructor
    String();
    String(const T* str, int n);
    String(const String& str);

    // destructor
    ~String();

    // members
    String& operator = (const String& str);
//    friend template<class T> ostream& operator << (ostream& os, const String<T>& str);

private:
    struct    Srep;
    Srep*     rep;
};

template <class T> struct String<T>::Srep {
    T*        s;
    int       n;
    int       nrefs;

    // constructor
    Srep(int n, const T* str);

    // destructor
    ~Srep();

    // members
    //   Srep* clone();
    void assign(int n, const T* str);

private:

    // prohibit copy and assign
    Srep(const Srep&);
    Srep& operator=(const Srep&);
};

template<class T> String<T>::Srep::Srep(int _n, const T* str)
    :n(_n), nrefs(1)
{
    s = new T[_n];
    for (int i = 0; i < _n; i++)
	s[i] = str[i];
}

template<class T> String<T>::Srep::~Srep()
{
    delete[] s;
}
/*
template<class T> String<T>::Srep* String<T>::Srep::clone()
{
    if (nrefs == 1) return this;
    
    nrefs--;
    return new Srep(n, s);
}*/

template<class T> void String<T>::Srep::assign(int _n, const T* str)
{
    if (n != _n) {
	delete[] s;
	n = _n;
	s = new char[_n];
    }

    for (int i = 0; i < _n; i++)
	s[i] = str[i];
}

template<class T> String<T>::String()
{
    rep = new Srep(1, T(0));
}

template<class T> String<T>::String(const T* str, int n)
{
    rep = new Srep(n, str);
}

template<class T> String<T>::String(const String& str)
{
    str.rep->nrefs++;
    rep = str.rep;
}

template<class T> String<T>::~String()
{
    if (--rep->nrefs == 0) delete rep;
}

template<class T> String<T>& String<T>::operator = (const String& str)
{
    str.rep->nrefs++;
    if (--rep->nrefs == 0) delete rep;
    rep = str.rep;

    return *this;
}
/*
template<class T> ostream& operator << (ostream& os, const String<T>& str)
{
    os << "content is: ";
    for (int i = 0; i < str.n; i++)
	os << str.s[i];

    os << "reference is: " << str.rep->nrefs;

    return os;
}
*/
#endif
