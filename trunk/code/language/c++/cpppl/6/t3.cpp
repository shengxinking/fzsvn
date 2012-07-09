/*
 *
 *
 *
 */

#include <map>
#include <iostream>
#include <string>

using namespace std;

struct Value {
    int      count;
    double   sum;
};

int main(void)
{
    map<string, Value>    pairs;
    string                name;
    double                value;
    double                total_sum = 0;
    int                   total_count = 0;

    while(cin >> name) { 
	pairs[name].count++;
	cin >> value;
	pairs[name].sum += value;
    }

    map<string, Value>::const_iterator itr = pairs.begin();
    for (; itr != pairs.end(); itr++) {
	cout << itr->first << " sum is " 
	     << itr->second.sum << ", average is" 
	     << itr->second.sum / itr->second.count << endl;
	total_sum += itr->second.sum;
	total_count += itr->second.count;
    }


    cout << "total sum is " << total_sum 
	 << ", total average is " << total_sum / total_count << endl;


    return 0;
}
