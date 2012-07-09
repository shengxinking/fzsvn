/*
 *
 *
 *
 */

#include <string>
#include <iostream>

using namespace std;

struct Date {
    int     day;
    int     month;
    int     year;
};

istream& operator >> (istream& is, Date& date)
{
    const int bufsize = 6;
    char      buf[bufsize] = {0};

    is.getline(buf, bufsize, '/');
    date.day = atoi(buf);

    is.getline(buf, bufsize, '/');
    date.month = atoi(buf);
    
    is >> date.year;

    return is;
}

ostream& operator << (ostream& os, const Date& date)
{
    os << date.day << "/" << date.month << "/" << date.year;

    return os;
}

Date& init(Date& d, int day, int month, int year)
{
    d.day = day;
    d.month = month;
    d.year = year;

    return d;
}

int
main(void)
{
    Date         date;
    
    cin >> date;
    cout << date << endl;

    return 0;
}
