/*
 *
 *
 *
 */

#ifndef __SHAPE_HPP__
#define __SHAPE_HPP__

#include <iostream>
#include <string>

using namespace std;

class Shape {

public:
    
    // abstact interface
    Shape(const string& msg);
    virtual ~Shape();
    virtual void draw() = 0;
    virtual bool is_closed() const = 0;
};

inline Shape::Shape(const string& msg)
{
    cout << __PRETTY_FUNCTION__ << endl;
}

inline Shape::~Shape()
{
//    cout << __PRETTY_FUNCTION__ << endl;
}


#endif
    
