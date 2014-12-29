/*
 *  test program of page 78 cpppl
 *
 *  write by Forrest.zhang
 */

#include <limits>
#include <iostream>
#include <string>

using namespace std;

int
main(void)
{
    cout << "char has " << numeric_limits<char>::digits << " bits"
	 << ", min is: " << int(numeric_limits<char>::min())
	 << ", max is:" << int(numeric_limits<char>::max()) << endl;
    cout << "short integer min is: " << numeric_limits<short>::min()
	 << ", max is:" << numeric_limits<short>::max() << endl;
    cout << "integer min is: " << numeric_limits<int>::min()
	 << ", max is:" << numeric_limits<int>::max() << endl;
    cout << "long integer min is: " << numeric_limits<long>::min()
	 << ", max is:" << numeric_limits<long>::max() << endl;
    cout << "long long integer min is: " << numeric_limits<long long>::min()
	 << ", max is:" << numeric_limits<long long>::max() << endl;
    cout << "float min is: " << numeric_limits<float>::min()
	 << ", max is:" << numeric_limits<float>::max() << endl;
    cout << "double min is: " << numeric_limits<double>::min()
	 << ", max is:" << numeric_limits<double>::max() << endl;
    cout << "long double min is: " << numeric_limits<long double>::min()
	 << ", max is:" << numeric_limits<long double>::max() << endl;

    return 0;
}
