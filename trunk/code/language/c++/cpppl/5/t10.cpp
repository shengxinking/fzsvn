/*
 *
 *
 *
 */

#include <iostream>

using namespace std;

static char*    month[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
			   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", 0};

static void 
print_month(char** str)
{
    char**         ptr = str;
    if (str) 
	while(*ptr)
	    cout << *ptr++ << endl;
}

int 
main(void)
{
    char** ptr = 0;
    ptr = month;

    while(*ptr)
	cout << *ptr++ << endl;

    cout << "run in function" << endl;
    print_month(month);

    return 0;
}
