/*
 *
 *
 *
 */

#include <iostream>
#include <string>

using namespace std;

class X {

public:
    explicit X(int i);
    explicit X(const char* s);
    
    void print() const;

private:
    int      m_x;
    string   m_s;
};

X::X(int i)
    : m_x(i)
{
}

X::X(const char* s)
    : m_x(0), m_s(s)
{
}

void X::print() const
{
    cout << m_x << endl;
//    cout << m_s << endl;
}

void f(X x)
{
    x.print();
}

int main(void)
{
    X         x1(10);
    X         x2("hello");

    f(10);
    f("hello");

    return 0;
}
