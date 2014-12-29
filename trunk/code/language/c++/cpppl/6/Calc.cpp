/*
 *
 *
 *
 */

#include <iostream>
#include <map>
#include <string>

using namespace std;

enum Token {
    NAME, NUMBER, END,
    PLUS = '+', MINUS = '-', MUL = '*', DIV = '/',
    PRINT = ';', ASSIGN = '=', LP = '(', RP = ')'
};

// global variable for calc program
static Token                 curr_token;
static map<string, double>   table;
static string                string_value;
static double                number_value;
static int                   nerrors;

// utils function for calc program
static double expr(bool get);
static double term(bool get);
static double prim(bool get);
static Token  get_token(void);
static int    error(const string& msg);

/*
 * calc + and - math operator
 * return the result
 */
double expr(bool get)
{
    double  left = term(get);

    while (1) {
	switch(curr_token) {
	case PLUS:
	    left += term(true);
	    break;
	case MINUS:
	    left -= term(true);
	    break;
	default:
	    return left;
	}
    }
}

double term(bool get)
{
    double left = prim(get);

    while (1) {
	switch(curr_token) {
	case MUL:
	    left *= prim(true);
	    break;
	case MINUS:
	    if (double d = prim(true)) {
		left /= d;
		break;
	    }
	    return error("divide by 0");
	default:
	    return left;
	}
    }
}


double prim(bool get)
{
    if (get) get_token();

    switch(curr_token) {
    case NUMBER: {
	double d = number_value;
	get_token();
	return d;
    }
    case NAME: {
	double& d = table[string_value];
	if (get_token() == ASSIGN) d = expr(true);
	return d;
    }
    case MINUS:
	return -prim(true);
    case LP: {
	double d = expr(true);
	if (curr_token != RP) return error(") expected");
	return d;
    }
    default:
	return error("primary expected");
    }
}


Token get_token(void)
{
    char         ch = 0;
    
    do {
	if (!cin.get(ch)) return curr_token = END;
    } while (ch != '\n' && isspace(ch));
    
    switch(ch) {
    case 0:
	return curr_token = END;
    case ';': case '*': case '/': case '+':
    case '-': case '(': case ')': case '=': 
	return curr_token= Token(ch);
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    case '.':
	cin.putback(ch);
	cin >> number_value;
	return curr_token = NUMBER;
    case '\n':
	return curr_token = PRINT;
    default:
	if (isalpha(ch)) {
	    cin.putback(ch);
	    cin >> string_value;
	    return curr_token = NAME;
	}
	error("bad token");
	return curr_token = PRINT;
    }
}


int error(const string& msg)
{
    nerrors++;
    cout << "error: " << msg << endl;
    return 1;
}


int main(void)
{
    table["pi"] = 3.1415926;
    table["e"] = 2.71828;
    
    while (cin) {
	get_token();

	if (curr_token == PRINT) continue;
	if (curr_token == END) break;
	cout << expr(false) << endl;
    }

    cout << nerrors << " found" << endl;

    return nerrors;
}
