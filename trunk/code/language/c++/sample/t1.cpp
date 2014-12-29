/*
 *
 *
 *
 */

#include <iostream>
#include <sstream>
#include <bitset>

using namespace std;

int main()
{
    ostringstream  os;

    os << "dec: " << 15 << hex << " hex: " << 15 << endl;

    cout << os.str() << endl;

}
