/*
 *
 *
 *
 */

#include "Manager.hpp"
#include "Employee.hpp"

#include <iostream>
#include <string>

using namespace std;


void f1(const Employee* e)
{
    e->print();
}

int main(void)
{
    Manager             m1("forrest", "zhang", 2);

    f1(&m1);

    return 0;
}
