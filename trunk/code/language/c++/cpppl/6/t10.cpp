/*
 *
 *
 *
 */

#include <iostream>

using namespace std;

size_t my_strlen(const char* s)
{
    int count = 0;
    if (s)
	while (*s++ != 0)
	    ++count;

    return count;
}

size_t my_strcpy(const char* s, char* d, size_t len)
{
    size_t count = 0;
    if (s && d)
	while ( (count < (len - 1)) 
		&& (*d++ = *s++) != 0)
	    ++count;
    *d = 0;

    return count;
}

int my_strcmp(const char* s1, const char* s2)
{
    assert(s1 && s2);
    do 
	if (int d = *s1 - *s2)
	    return d;
    while (*s1++ && *s2++);
    
    return 0;
}

int
main(void)
{
    const char*          s1 = "hello";
    const char*          s2 = "hello";

    char                 buf[10] = {0};

    cout << my_strlen(s1) << endl;

    my_strcpy(s1, buf, 10);
    
    cout << buf << endl;

    cout << my_strcmp(s1, s2) << endl;

    return 0;
}
