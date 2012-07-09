/*
 *  class Date implement
 *
 *  write by Forrest
 */

#include "Date.hpp"

Date Date::default_date(1, jun, 1970);

/*
 *  the constructor of Date, it'll throw a BadDate exception if
 *  the input date is illegal.
 */
Date::Date(int dd, Month mm, int yy)
{
    dd = dd ? dd : default_date.d;
    mm = mm ? mm : default_date.m;
    yy = yy ? yy : default_date.y;

    if (mm < jan || mm > dec)
	throw BadDate("incorrect month");
	
    if (dd < 1 || dd > month_days(mm, yy))
	throw BadDate("incorrect day");
    
    d = dd;
    m = mm;
    y = yy;

    cache = new Cache();
    cache->valid = false;
}

/*
 *  destructor of Date
 */
inline Date::~Date()
{
    if (cache)
	delete cache;
}

/*
 *  add @n days to date
 */
Date& Date::add_day(int n)
{
    if (n) {
	int   left = n;

	while ( (left + d) > month_days(m, y)) {
	    left -= (month_days(m, y) - d + 1);
	    d = 1;
	    add_month(1);
	}

	if (left)
	    d += left;
	
	cache->valid = false;
    }

    return *this;
}

/*
 *  add @n months to date
 */
Date& Date::add_month(int n)
{
    if (n) {
	int   mm;
	int   yy;

	yy = n / 12;
	mm = m + n % 12;

	// left n add m great than 12
	if (mm > 12) {
	    yy++;
	    mm -= 12;
	}

	// month first
	m = Month(mm);
	if (d > month_days(m, y))
	    d = month_days(m, y);

	add_year(yy);

	cache->valid = false;
    }

    return *this;
}

/*
 *  add @n year to date
 */
Date& Date::add_year(int n)
{
    if (n) {
	int  yy;
	yy = y + n;
	
	if (d == month_days(m, y))
	    if ( (!leapyear(y) && leapyear(yy)) ||
		 (leapyear(y) && !leapyear(yy)) )
		d = month_days(m, yy);
	y = yy;

	cache->valid = false;
    }
    
    return *this;
}

/*
 *  return a string represented date, it format is dd-mm-yyyy
 */
string Date::string_rep() const
{
    if (!cache->valid) {
	compute_cache_value();
	cache->valid = true;
    }

    return cache->rep;
}

/*
 *  regenerator the cache
 */
void Date::compute_cache_value() const
{
    ostringstream       os;
    os << d << "-" << m << "-" << y;
    cache->rep = os.str();
}

/*
 *  look @a and @b are same date or not
 */
bool diff (const Date& a, const Date& b)
{
    if (a.y != b.y ||
	a.m != b.m || 
	a.d != b.d)
	return true;
    else
	return false;
}

/*
 *  determine y is a leap year  
 *  a leap year con satisfy these rules:
 *
 *        1: Every year divisible by 4 is a leap year.
 *        2: But every year divisible by 100 is NOT a leap year
 *        3: Unless the year is also divisible by 400, then it is still a leap year.
 *
 *  return true if a leap year, false if not
 */
bool leapyear(int y)
{
    if ( (y % 4 == 0 && y % 100 != 0) ||
	 y % 400 == 0)
	return true;
    else
	return false;
}

/*
 *  determine the max days in month @mon of year @yy
 *  we add @yy parameter because of leap year
 *
 *  return the max days if OK, 0 if error
 */
int month_days(Date::Month mon, int yy)
{
    int max = 0;

    switch (mon) {
    case Date::feb:
	max = 28 + leapyear(yy);
	break;
    case Date::apr : case Date::jun: case Date::sep: case Date::nov:
	max = 30;
	break;
    case Date::jan: case Date::mar: case Date::may: case Date::jul: 
    case Date::aug: case Date::oct: case Date::dec:
	max = 31;
	break;
    default:
	break;
    }

    return max;
}

/*
 *  return next weekday of date, next weekday is Monday
 */
Date next_weekday(const Date& d)
{
    return Date(1, Date::Month(1), 1970);
}

/*
 *  return next saturday of date
 */
Date next_saturday(const Date& d)
{
    return Date(1, Date::Month(1), 1970);
}
