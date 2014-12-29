/*
 *
 *
 *
 */

#include <limits>
#include <iostream>

using namespace std;

int main(void)
{
    double          d = 8.0;
    int             j = 7;
    
    double          f = d / (j - j);

    cout << f << endl;

    unsigned int    ui = numeric_limits<unsigned int>::max();
    
    cout << ui << endl;

    ui += 1;

    cout << ui << endl;

    ui = numeric_limits<unsigned int>::min();

    ui -= 1;

    cout << ui << endl;

    return 0;
}
    
