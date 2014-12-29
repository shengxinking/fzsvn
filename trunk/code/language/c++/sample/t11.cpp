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
    X(const string&);

    string* operator -> ();

    friend ostream& operator << (ostream&, const X&);

private:
    string         m_str;
};

inline X::X(const string& str)
    :m_str(str)
{
}

inline string* X::operator -> ()
{
    return &m_str;
}

inline ostream& operator << (ostream& os, const X& x)
{
    os << x.m_str;

    return os;
}

int main(void)
{
    X                    x("hello");

    cout << x << endl;
    
    string*  pstr = x.operator->();

    *pstr = "nihao";

    cout << x << endl;

    return 0;
}
