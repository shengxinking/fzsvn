/*
 *
 *
 *
 */


class X {

public:
    X(int);
    X(char*);
    X(const X&);

private:
    int      m_i;
    char*    m_str;
};

X::X(int i)
    : m_i(i), m_str(0)
{
    m_str = 0;
}

X::X(char* str)
    : m_i(0)
{
    m_str = str;
}

X::X(const X& x)
{
    m_i = x.m_i;
    m_str = x.m_str;
}

class Y {
   
public:
    Y(int);

private:
    int      m_i;
};

Y::Y(int i)
    : m_i(i)
{
}

class Z {
public:
    Z(X);

private:
    X       m_x;
};

Z::Z(X x)
    : m_x(x)
{
}

X f(X x)
{
    return x;
}

Y f(Y y)
{
    return y;
}

Z g(Z z)
{
    return z;
}


int main(void)
{
    f(1);
    f(X(1));
    f(Y(1));

    g("Mack");
    g(X("Doc"));
    g(Z("Suzy"));

    return 0;
}
    
