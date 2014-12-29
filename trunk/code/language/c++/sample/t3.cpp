/*
 *
 *
 *
 */

#include <string>
#include <iostream>

using namespace std;

int main()
{
    string   s("/Storage/Logs");

    cout << "before erase /" << endl;
    cout << s << endl;

    if ( !s.empty() && s.at(s.size() - 1) == '/')
	s.erase(s.size() - 1);

    cout << "after erase /" << endl;
    cout << s << endl;

    return 0;
}
