/*
 *
 *
 *
 */

#include "String.hpp"


int main(void)
{
    String<char>       s1("nihao", 5);

    String<char>       s2;

    s2 = s1;

    return 0;
}
