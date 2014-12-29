/*
 *
 *
 *
 */

#include <iostream>
#include <string>

using namespace std;

struct month_days {
    string      name;
    int         ndays;
};

static month_days  month[] = {
    {"Jan", 31}, {"Feb", 28}, {"Mar", 31}, {"Apr", 30}, {"May", 31}, {"Jun", 30},
    {"Jul", 31}, {"Aug", 31}, {"Sep", 30}, {"Oct", 31}, {"Nov", 30}, {"Dec", 31}
};

int
main(void)
{
    char*       month[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", 
			   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    int         days_of_month[] = {31, 28, 31, 30, 31, 30, 
				   31, 31, 30, 31, 30, 31};

    cout << "month  days" << endl;
    for (int i = 0; i < sizeof(days_of_month) / sizeof(int); i++)
	cout << month[i] << "    " << days_of_month[i] << endl;

    cout << "month  days" << endl;
    for (int i = 0; i < sizeof(days_of_month) / sizeof(int); i++)
	cout << ::month[i].name << "    " << ::month[i].ndays << endl;

    return 0;
}
