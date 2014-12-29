/*
 *
 *
 *
 */

#ifndef __EMPLOYEE_HPP__
#define __EMPLOYEE_HPP__

#include <string>
#include <iostream>

using namespace std;

class Employee {

public:
    // constructor
    Employee();
    Employee(const string& _first_name, const string& _last_name);

    // destructor
    virtual ~Employee();

    friend ostream& operator << (ostream& os, const Employee& e);

    virtual void print() const;
    
private:
    string    first_name;
    string    last_name;
};

inline Employee::Employee()
{
    cout << __PRETTY_FUNCTION__ << endl;
}

inline Employee::Employee(const string& _first_name, const string& _last_name)
    :first_name(_first_name), last_name(_last_name)
{
    cout << __PRETTY_FUNCTION__ << endl;
}

inline Employee::~Employee()
{
    cout << __PRETTY_FUNCTION__ << endl;
}

inline ostream& operator << (ostream& os, const Employee& e)
{
    os << e.first_name << " " << e.last_name;

    return os;
}

inline void Employee::print() const
{
    cout << __PRETTY_FUNCTION__ << endl;
    
    cout << first_name << " " << last_name << endl;
}

#endif
