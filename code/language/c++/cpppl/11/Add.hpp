/*
 *
 *
 *
 */

#ifndef __ADD_HPP__
#define __ADD_HPP__

#include <complex>

using namespace std;

typedef complex<double>  dcomplex;

class Add {
    
public:
    Add(const dcomplex&);
    Add(double r, double i);

    void operator () (dcomplex&) const;

private:
    dcomplex       m_val;
};


inline Add::Add(const dcomplex& c)
    : m_val(c)
{
}


inline Add::Add(double r, double i)
    : m_val(r, i)
{
}

inline void Add::operator () (dcomplex& c) const
{
    c += m_val;
}


#endif
