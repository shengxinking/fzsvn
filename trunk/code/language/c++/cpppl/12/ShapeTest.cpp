/*
 *
 *
 *
 */

#include "Shape.hpp"
#include "Circle.hpp"

int main(void)
{
    Shape*        ps = new Circle(0, 0, 5);

    delete ps;
}
