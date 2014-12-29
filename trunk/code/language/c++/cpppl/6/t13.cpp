/*
 *
 *
 *
 */

#include <cstddef>
#include <cassert>
#include <iostream>

using namespace std;

static char* my_strcat(const char* s1, const char* s2)
{
    assert(s1 && s2);

    const size_t   len = strlen(s1) + strlen(s2);
    char*          buf = new char[len + 1];

    int            i = 0;
    while (buf[i++] = *s1++); 
    i--;

    while (buf[i++] = *s2++);
    
    return buf;
}


int main(void)
{
    char*          s1 = "hello, ";
    char*          s2 = "ni hao";
    char*          s3 = my_strcat(s1, s2);

    cout << s3 << endl;

    delete[] s3;
    return 0;
}

