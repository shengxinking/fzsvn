/*
 *
 *
 *
 */

#include <iostream>
#include <string>

using namespace std;

enum e_test_t {E1 = 0, E2, E3};

int
main(void)
{
    cout << "size of bool is: " << sizeof(bool) << endl;
    cout << "size of character is: " << sizeof(char) << endl;
    cout << "size of short integer is: " << sizeof(short) << endl;
    cout << "size of integer is: " << sizeof(int) << endl;
    cout << "size of long integer is: " << sizeof(long int) << endl;
    cout << "size of long long integer is: " << sizeof(long long int) << endl;
    cout << "size of float is: " << sizeof(float) << endl;
    cout << "size of double is: " << sizeof(double) << endl;
    cout << "size of long double is: " << sizeof(long double) << endl;

    cout << "size of enum type is: " << sizeof(e_test_t) << endl;

    cout << "size of pointer to string is: " << sizeof(string*) << endl;

    return 0;
}
