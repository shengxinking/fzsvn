/*
 *
 *
 *
 */

#include <iostream>
#include <string>

using namespace std;

enum context_t { C_COMMENT = 1, CPP_COMMENT, CODE, QUOTE };

static context_t get_context_type(char c);
static void handle_c_comment();
static void handle_cpp_comment();
static void handle_quote(char c);

int 
main(void)
{
    context_t      type;
    char           c;
    
    while (cin.get(c)) {
	type = get_context_type(c);
    
	switch (c) {
	case C_COMMENT:
	    handle_c_comment();
	    break;
	case CPP_COMMENT:
	    handle_cpp_comment();
	    break;
	case QUOTE:
	    handle_quote(c);
	    break;
	default:
	    cout << c;
	}
    }
    
    return 0;
}

context_t
get_context_type(char c)
{
    switch (c) {
    case '/':
	if (cin.get(c) && c == '*')
	    return C_COMMENT;
	else if (c == '/')
	    return CPP_COMMENT;
	else {
	    cout << "/" << c;
	    return CODE;
	}
    case '\'':
	cout << c;
	return QUOTE;
    case '"':
	cout << c;
	return QUOTE;
    default:
	return CODE;
    }
}

void
handle_c_comment()
{
    char       c;

    while (cin.get(c))
	if (c == '*') {
	    if (cin.get(c) && c == '/')
		return;
	    else
		cin.putback(c);
	}
}

void
handle_cpp_comment()
{
    char       c;

    while ( cin.get(c) && c != '\n')
	;

    return;
}


void
handle_quote(char c)
{
    char       i;

    while (cin.get(i))
	switch(i) {
	case '\\':
	    cin.get(i);
	    cout << i;
	    break;
	default:
	    if (i == c) {
		cout << i;
		return;
	    }
	    else
		cout << c;
	}
}

