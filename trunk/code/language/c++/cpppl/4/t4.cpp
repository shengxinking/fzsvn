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
    for (int i = 'a'; i <= 'z'; ++i)
	cout << "char: " << char(i) 
	     << " integer: " << dec << i 
	     << " hex integer: "<< showbase << hex << i << endl;

    for (int i = '0'; i <= '9'; ++i)
	cout << "char: " << char(i) 
	     << " integer: " << dec << i 
	     << " hex integer: "<< showbase << hex << i << endl;

    return 0;
}
