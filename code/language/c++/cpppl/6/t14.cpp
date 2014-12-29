/*
 *
 *
 *
 */

#include <cassert>
#include <iostream>
#include <string>
using namespace std;

void strrev(char* s)
{
    assert(s);

    char* p = &s[strlen(s) - 1];
    char  c;
    while (p > s) {
	c = *s;
	*s = *p;
	*p = c;
	++s;
	--p;
    }
}


int main(void)
{
    char   s[] = "hello, jerry";

    cout << s << endl;
    strrev(s);
    cout << s << endl;

    return 0;
}
