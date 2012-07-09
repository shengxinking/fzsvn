/*
 *  the test program of class Date
 *
 *
 *  write by Forrest
 */

#include "Date.hpp"

#include <iostream>


int main(void)
{
    Date*             today = 0;
    try {
	today = new Date(1, Date::Month(0), 2005);
    }
    catch (Date::BadDate& e) {
	e.print();
    }

    cout << ">>===== test constructor" << endl;
    if (today) {
	cout << today->string_rep() << endl;
	cout << ">>===== test SUCCEED" << endl;
    }
    
    
    cout << "\n>>===== test add_day add_month, add_year" << endl;
    {
	Date          date1(29, Date::feb, 2004);
	cout << "first date" << date1.string_rep() << endl;
	date1.add_year(4);
	cout << "after 1 year: " << date1.string_rep() << endl;

	Date          date2(31, Date::dec, 2004);
	cout << "first date " << date2.string_rep() << endl;
	date2.add_month(2);
	cout << "after 2 months: " << date2.string_rep() << endl;

	Date          date3(20, Date::feb, 2004);
	cout << "first date " << date3.string_rep() << endl;
	date3.add_day(30);
	cout << "after 30 days: " << date3.string_rep() << endl;
    }
    
    cout << ">>===== test SUCCEED" << endl;

    return 0;
}

    
