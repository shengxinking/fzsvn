/*
 *
 *
 *
 */

#ifndef __CIRCLE_HPP__
#define __CIRCLE_HPP__

#include "Shape.hpp"

#include <iostream>

using namespace std;

class Circle : public Shape {
    
public:
    
    // constructor
    Circle(int x, int y, int radius);

    // destructor
    virtual ~Circle();

    // public function
    void draw();
    bool is_closed() const;

private:
    int       x;
    int       y;
    int       radius;
};

inline Circle::Circle(int _x, int _y, int _radius)
    :Shape(""), x(_x), y(_y), radius(_radius)
{
    cout << __PRETTY_FUNCTION__ << endl;
}

inline Circle::~Circle()
{
    cout << __PRETTY_FUNCTION__ << endl;
}

inline void Circle::draw()
{
    cout << __PRETTY_FUNCTION__ << endl;
}

inline bool Circle::is_closed() const
{
    return true;
}

#endif
