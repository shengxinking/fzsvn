/*
 *  class Complex handle the complex in math
 *
 *
 *  write by Forrest
 */

#ifndef __COMPLEX_HPP__
#define __COMPLEX_HPP__

class Complex {

public:

    Complex(double r = 0.0, double i = 0.0);
    ~Complex();
    Complex(const Complex&);

    // get status members
    double real() const;
    double image() const;

    // the operator
    Complex operator + (Complex);
    Complex operator - (Complex);

private:
    double     real;
    double     image;
};

inline Complex::Complex(double r, double i)
    : real(r), image(i)
{
}

inline Complex::Complex(const Complex& c)
{
    real = c.real;
    image = c.image;
}

inline Complex::~Complex()
{
}

inline double Complex::real() const
{
    return real;
}

inline double Complex::image() const
{
    return image;
}

inline Complex Complex::operator + (Complex c)
{
    return Complex( (this->real + c.real), (this->image + c.image));
}

inline Complex Complex::operator - (Complex c)
{
    return Complex( (this->real - c.real), (this->image - c.image));
}

#endif

