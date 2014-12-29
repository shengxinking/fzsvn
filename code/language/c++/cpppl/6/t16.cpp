/*
 *  
 *
 *
 */

#include <iostream>

using namespace std;

enum DIGIT_BASE {BIN = 2, OCT = 8, DEC = 10, HEX = 16};

static bool
is_digit(char c, DIGIT_BASE base)
{
    switch(base) {
    case BIN:
	if (c == '0' || c == '1')
	    return true;
	else
	    return false;
    case OCT:
	if (c >= '0' && c <= '7')
	    return true;
	else
	    return false;
    case DEC:
	if (c >= '0' && c <= '9')
	    return true;
	else
	    return false;
    case HEX:
	if (c >= '0' && c <= '9' ||
	    c >= 'A' && c <= 'F' ||
	    c >= 'a' && c <= 'f')
	    return true;
	else
	    return false;
    default:
	return false;
    }
}

static int
char2int(char c)
{
    if (c >= '0' && c <= '9')
	return (c - '0');
    else if (c >= 'a' && c <= 'f')
	return (c - 'a' + 10);
    else
	return (c - 'A' + 10);
}

/*
 *  convert a number string to integer, suport 8-base, 
 *  10-base, 16-base
 */
static int
str2int(const char* s)
{
    assert(s);

    int     base = 10;
    int     positive = 1;
    int     digit = 0;
    int     sum = 0;

    // negetive
    if (*s == '-' && s++)
	positive = -1;

    switch (*s) {
    case '0':
	if (*(++s) == 'x' && ++s)
	    base = 16;
	else
	    base = 8;
	break;
    
    case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9':
	break;
    default:
	cout << "not number string" << endl;
	return 0;
    }

    while (true) {
	if (!is_digit(*s, (DIGIT_BASE)base))
	    break;

	digit = char2int(*s);
	sum = sum * base + digit;
	++s;
    }
    
    return sum * positive;
}


int 
main(void)
{
    char*    s1 = "1002ss";
    char*    s2 = "s0333";
    char*    s3 = "-0xff";

    cout << s1 << endl;
    cout << str2int(s1) << endl;
    cout << s2 << endl;
    cout << str2int(s2) << endl;
    cout << s3 << endl;
    cout << str2int(s3) << endl;

    return 0;
}
    
