/*
 *
 *
 *  write by Forrest.zhang
 */

#include "Matherr.hpp"

static void f1()
{
    throw Matherr("throw a Matherr Exception");
}

static void f2()
{
    throw new Overflow("throw a Overflow Exception");
}

static void f3()
{
    throw Underflow("throw a Underflow Exception");
}

int main(void)
{
    try {
	f2();
    }
    catch (Matherr* e){
	e->print_debug();
	delete e;
    }
    catch (Overflow e) {
	e.print_debug();
	
    }
    
    
    cout << "catch OK" << endl;
    
    return 0;
}




