/*
 *
 *
 *
 */

#include "Add.hpp"

#include <vector>
#include <complex>
#include <iostream>

using namespace std;

int main(void)
{
    vector<dcomplex>         vec;

    vec.push_back(dcomplex(1, 1));
    vec.push_back(dcomplex(2, 2));
    vec.push_back(dcomplex(3, 3));
    
    vector<dcomplex>::const_iterator  itr1 = vec.begin();
    for (; itr1 != vec.end(); ++itr1)
	cout << *itr1 << endl;

    Add                     add(dcomplex(2, 3));

    for_each(vec.begin(), vec.end(), add);
    vector<dcomplex>::const_iterator  itr2 = vec.begin();
    for (; itr2 != vec.end(); ++itr2)
	cout << *itr2 << endl;

    cout << "test operator () " << endl;

    dcomplex            c(1, 2);
    Add                 add1( 2, 1);

    cout << c << endl;
    add1(c);
    cout << c << endl;

    return 0;
}
