/*
 *
 *
 *  write by Forrest.zhang
 */

#include "Satellite.hpp"

int main(void)
{
    Satellite         s("task", "display", "name");
    Task*             t = &s;
    Display*          d = &s;

    s.next = 0;

    s.draw();
    //   s.print();

    t->print();
    
    d->draw();

    return 0;
}






