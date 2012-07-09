/*
 *	the C++ static library
 *
 *	write by Forrest.zhang
 */

#ifndef __TEST_HPP__
#define __TEST_HPP__

class Test {

public :
	Test() {};
	~Test();

	void print() const;
};

inline Test::~Test()
{
}

#endif


