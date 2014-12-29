/*
 * 	test virtual destructor of C++
 *
 * 	write by Forrest.zhang
 */
#include <iostream>
#include <string>

class A {

public :
	A(const std::string &msg) { std::cout << "A constructor: " << msg << std::endl; };
	~A() { std::cout << "A destructor" << std::endl; };
	void print(void) const { std::cout << "A print" << std::endl; };
};

class B : public A {

public :
	B(const std::string &msg) : A(msg) { std::cout << "B constuctor" << std::endl; };
	virtual ~B() { std::cout << "B destructor" << std::endl; };
	virtual void print(void) const { std::cout << "B print" << std::endl; };
};

class C : public B {

public :
	C(const std::string &msg) : B(msg) { std::cout << "C constructor" << std::endl; };
	~C() { std::cout << "C destructor" << std::endl; };
	void print(void) const { std::cout << "C print" << std::endl; };
};

int main(void)
{
	A       *p = new C("hello c pointer");
	p->print();

	delete p;

	C       c("hello c");
	B       &b = c;
	
	b.print();

	return 0;
}
