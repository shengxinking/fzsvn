/*
 *
 *
 *
 */

#ifndef __MANAGER_HPP__
#define __MANAGER_HPP__

#include "Employee.hpp"

class Manager : public Employee {

public:
    
    // constructor
    Manager();
    Manager(const string& first_name, const string& lastname, int level);

    // destructor
    virtual ~Manager();

    // operator
    friend ostream& operator << (ostream& os, const Manager& m);

    void print() const;

private:
    
    int              level;
};

inline Manager::Manager()
{
    cout << __PRETTY_FUNCTION__ << endl;
}

inline Manager::Manager(const string& fname, const string& lname, int _level)
    : Employee(fname, lname), level(_level)
{
    cout << __PRETTY_FUNCTION__ << endl;
}

inline Manager::~Manager()
{
    cout << __PRETTY_FUNCTION__ << endl;
}

inline ostream& operator << (ostream& os, const Manager& m)
{
    m.Employee::print();
    cout << m.level << endl;
    
    return os;
}

inline void Manager::print() const
{
    cout << __PRETTY_FUNCTION__ << endl;
//    Employee::print();
    cout << level << endl;
}

#endif
