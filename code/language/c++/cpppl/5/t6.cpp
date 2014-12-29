/*
 *
 *
 *
 */

#include <iostream>

using namespace std;

static void
f(char  c)
{
    cout << __PRETTY_FUNCTION__ << endl;
}

static void
g(char&  c)
{
    cout << __PRETTY_FUNCTION__ << endl;
}


static void
h(const char&  c)
{
    cout << __PRETTY_FUNCTION__ << endl;
}

int
main(void)
{
    f('a');
    f(49);
    f(3300);
    char   c = 'c';
    f(c);
    const char uc = 'u';
    f(uc);

    g('a');
    g(49);
    g(3300);
    g(c);
    g(uc);

    h('a');
    h(49);
    h(3300);
    h(c);
    h(uc);
}
