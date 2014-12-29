/*
 *
 *
 *
 */

#include <iostream>
#include <string>

using namespace std;


int
main(void)
{
    char     str[] = "a short string";
    string   s = "a short string";

    cout << "string is: " << str << endl;
    cout << "size of c style string is: " << sizeof(str) << endl;
    cout << "size fo c++ style string is: " << s.size() << endl;

    return 0;
}
