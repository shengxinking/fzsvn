/*
 *
 *
 *
 */

#include <vector>
#include <iostream>
#include <string>
#include <algorithm>

using namespace std;

int
main(void)
{
    vector<string>      vec;
    vector<string>      dupped;
    string              str;

    // get input string
    while(cin >> str) {
	if (str == "Quit")
	    break;

	vec.push_back(str);
    }


    // output vector, avoid print dupped string
    for (vector<string>::iterator citr = vec.begin();
	 citr != vec.end(); ++citr) {
	string  val = *citr;

	// find is there dupped string before citr
	vector<string>::const_iterator itr = find(vec.begin(), citr, val);

	// print it if there is no dupped string befero citr
	if (itr == citr)
	    cout << val << endl;
    }

    // sorted and output
    sort(vec.begin(), vec.end());
    for (vector<string>::iterator citr = vec.begin();
	 citr != vec.end(); ++citr) {
	string  val = *citr;

	// find is there dupped string before citr
	vector<string>::const_iterator itr = find(vec.begin(), citr, val);

	// print it if there is no dupped string befero citr
	if (itr == citr)
	    cout << val << endl;
    }

    return 0;
}
