/*
 *
 *
 *
 */

#include "String.hpp"

int main(void)
{
    String       s1("nihao");

    String       s2;

    cout << s1 << endl;
    cout << s2 << endl;
    
    s2 = s1;

    cout << s1 << endl;
    cout << s2 << endl;

    s2 = "jerry";
    s1 = "tom";

    cout << s1 << endl;
    cout << s2 << endl;

    return 0;
}
