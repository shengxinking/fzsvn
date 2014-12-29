#include <iostream>
#include <string>
using namespace std;

char ch = 'e';
string s;
int count;
const double pi = 3.1415;

int
main(void)
{
    extern char ch;
    extern string s;
    extern int count;
    extern const double pi;
    
    cout << ch << endl;
    ch = 'a';
    cout << ch << endl;
    s = "Hello";
    count = 4;

    return 0;
}
