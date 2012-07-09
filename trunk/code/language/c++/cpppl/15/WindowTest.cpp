/*
 *
 *
 *
 */

#include <iostream>

#include "Window.hpp"


int main(void)
{
    My_window*  win1 = new My_window();

    win1->set_color();
    win1->prompt();

    delete win1;

    return 0;
}
