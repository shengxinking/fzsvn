/*
 *  class Date declaration, class Date is a concrete class
 *
 *  write by Forrest
 */

#ifndef __DATE_HPP__
#define __DATE_HPP__

#include <string>
#include <sstream>
#include <iostream>

using namespace std;

class Date {
    
public:

    enum Month { jan = 1, feb, mar, apr, may, jun, jul, aug, sep, oct, nov, dec };    
    class BadDate;

    // constructor and destructor
    Date(int dd = 0, Month mm = Month(0), int yy = 0);
    ~Date();

    
    // check status function
    int day() const;
    int month() const;
    int year() const;

    string string_rep() const;
    void   cstring_rep() const;


    // change status function
    Date& add_day(int);
    Date& add_month(int);
    Date& add_year(int);
    static void set_default(int dd = 1, Month mm = jan, int yy = 1970);


    // friend functions
    friend bool diff(const Date&, const Date&);
    friend Date next_weekday(const Date&);
    friend Date next_sunday(const Date&);


private:
    int                d;
    Month              m;
    int                y;

    static Date        default_date;

    // for string_rep functions, it support lazy evaluation
    struct Cache;
    Cache*             cache;
    void compute_cache_value() const;
};

/*
 *  the exception class for Data
 */
class Date::BadDate {
    public:
    BadDate(const string& m);
    void print() const;

    private:
    string             msg;
};

/*
 *  cache the Date represent string.
 */
struct Date::Cache {
    bool   valid;
    string rep;
};


/*
 *  BadDate construct
 */
inline Date::BadDate::BadDate(const string& m)
{
    msg = m;
}

/*
 *  print the BadDate's message
 */
inline void Date::BadDate::print() const
{
    cout << msg << endl;
}

/*
 *  set default date of class Date
 */
inline void Date::set_default(int dd, Month mm, int yy)
{
    default_date = Date(dd, mm, yy);
}

/*
 *  return the day of Date
 */
inline int Date::day() const
{
    return d;
}

/*
 *  return the month of Date
 */
inline int Date::month() const 
{
    return m;
}

/*
 *  return the year of Date
 */
inline int Date::year() const
{
    return y;
}



// the utility function of Date

bool diff (const Date&, const Date&);
bool leapyear(int y);
int  month_days(Date::Month mon, int year);
Date next_weekday(Date d);
Date next_saturday(Date d);

#endif
