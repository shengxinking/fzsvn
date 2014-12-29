
/*
 *
 *
 *
 */

#include <string>
#include <iostream>

using namespace std;


static void g(const string* s)
{
    string* p = const_cast<string*>(s);
    *p = "in g";
}

static void h(const string& s)
{
    string& r = const_cast<string&>(s);
    r = "in h";
}

int main(void)
{
    string s2 = "s2";
    string s3 = "s3";

    cout << s2 << endl;
    g(&s2);
    cout << s2 << endl;

    cout << s3 << endl;
    h(s3);
    cout << s3 << endl;

    return 0;
}
