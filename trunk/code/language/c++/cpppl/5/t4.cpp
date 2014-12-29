/*
 *
 *
 *
 */

#include <iostream>

using namespace std;

static void
swap_by_pointer(int* i, int* j)
{
    if (i && j) {
	int         temp = *i;
	*i = *j;
	*j = temp;
    }
}


static void 
swap_by_reference(int& i, int& j)
{
    int       temp = i;
    i = j;
    j = temp;
}


int
main(void)
{
    int        i = 10;
    int        j = 20;

    cout << "before swaped: i = " << i << ", j = " << j << endl;
    swap_by_pointer(&i, &j);
    cout << "after swaped: i = " << i << ", j = " << j << endl;

    i = 123;
    j = 456;

    cout << "before swaped: i = " << i << ", j = " << j << endl;
    swap_by_reference(i, j);
    cout << "after swaped: i = " << i << ", j = " << j << endl;
    
    return 0;
}
