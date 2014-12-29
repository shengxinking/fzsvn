/*
 *  
 *
 *  write by Forrest.zhang
 */

#ifndef __SATELLITE_HPP__
#define __SATELLITE_HPP__

#include <iostream>
#include <string>

using namespace std;

struct Link {
    Link*       next;
};

class Second : virtual Link
{
};

class Task : virtual public Link {

public:
    
    // constructor
    Task(const string& name);

    // destructor
    virtual ~Task();

    // member functions
    virtual void print() const;
    
protected:
    string       name;
};

inline Task::Task(const string& _name)
    : name(_name)
{
    cout << __PRETTY_FUNCTION__ << endl;
}

inline Task::~Task()
{
    cout << __PRETTY_FUNCTION__ << endl;
}

inline void Task::print() const
{
    cout << __PRETTY_FUNCTION__ << endl;
    cout << name << endl;
}

class Display : public Second {
    
public:
    
    // constructor
    Display(const string& name);

    // destructor
    virtual ~Display();

    // member functions
    virtual void draw() const;
    virtual void print() const;

protected:
    string         name;

};

inline Display::Display(const string& _name)
    :name(_name)
{
    cout << __PRETTY_FUNCTION__ << endl;
}

inline Display::~Display()
{
    cout << __PRETTY_FUNCTION__ << endl;
}

inline void Display::draw() const
{
    cout << __PRETTY_FUNCTION__ << endl;
    cout << name << endl;
}

inline void Display::print() const
{
    cout << __PRETTY_FUNCTION__ << endl;
    cout << name << endl;
}

class Satellite : public Task, public Display {

public:

    // constructor
    Satellite(const string& task, const string& display, const string& name);

    // destructor
    ~Satellite();

    // member functions
//    void print() const;
    void draw() const;

private:
    string       name;

};

inline Satellite::Satellite(const string& task, const string& display, const string& _name)
    :Task(task), Display(display), name(_name)
{
    cout << __PRETTY_FUNCTION__ << endl;
}

inline Satellite::~Satellite()
{
    cout << __PRETTY_FUNCTION__ << endl;
}
/*
inline void Satellite::print() const
{
    cout << __PRETTY_FUNCTION__ << endl;
    cout << name << endl;
}
*/
inline void Satellite::draw() const
{
    cout << __PRETTY_FUNCTION__ << endl;
    cout << name << endl;
}

#endif

