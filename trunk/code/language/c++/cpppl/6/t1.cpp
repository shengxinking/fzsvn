/*
 *
 *
 *
 */

#include <iostream>

using namespace std;

static int 
f(const char* input_line, int max_length)
{
    int    quest_count = 0;
    int    i = 0;
    for (i = 0; i < max_length; i++)
	if (input_line[i] == '?')
	    quest_count++;
    
    return quest_count;
}

static int
g(const char* input_line, int max_length)
{
    char*       p = 0;
    int         quest_count = 0;

    for(p = const_cast<char*>(input_line); (p - input_line) < max_length; ++p)
	if (*p == '?')
	    quest_count++;
    
    return quest_count;
}


static int
h(const char* input_line, int max_length)
{
    int   i = 0;
    int   quest_count = 0;

    while(i < max_length)
	if (input_line[i++] == '?')
	    quest_count++;
    
    return quest_count;
}

int main(void)
{
    char*      p = "ni hao ???";

    cout << f(p, 10) << endl;
    cout << g(p, 10) << endl;
    cout << h(p, 10) << endl;

    return 0;
}
