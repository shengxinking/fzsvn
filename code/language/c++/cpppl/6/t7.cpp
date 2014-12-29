/*
 *
 *
 *
 */

#include <iostream>

using namespace std;

static int gi = 0;

static int
f(int a, int b)
{
    cout << "(a b gi) = (" << a << ' ' << b << ' ' 
	 << gi << ")" << endl;

    return a + b + gi;
}

int main(void)
{
    f(++gi, ++gi);
    f(1, 2) + f(3, 4);
    
    if (f(5, 6) == ++gi);
    
    gi & 32/gi;

    return 0;
}


