/*
 *
 *
 *
 *
 *
 */

#include "Assoc.hpp"

#include <iostream>

using namespace std;

int main(void)
{
    string      buf;
    Assoc       vec;

    while(cin >> buf) vec[buf]++;

    cout << vec << endl;

    string      test = "hello";

    vec[test]++;
    
    cout << vec[test] << endl;

    return 0;
}
