/*
 *
 *
 *  write by Forrest.zhang
 */

#include <string>
#include <iostream>
#include <exception>

using namespace std;

class E1 {
public:
    E1(const string& msg);

    void print_debug() const;

private:
    string           msg;
};

inline E1::E1(const string& _msg)
    :msg(_msg)
{
}

inline void E1::print_debug() const
{
    cout << msg << endl;
}

class E2 {
public:
    E2(const string& msg);

    void print_debug() const;

private:
    string           msg;
};

inline E2::E2(const string& _msg)
    :msg(_msg)
{
}

inline void E2::print_debug() const
{
    cout << msg << endl;
}

static void f1() throw (E1)
{
    throw E1("throw E1 in f1");
}

static void f2() throw (E1, bad_exception)
{
    throw E2("throw E2 in f2");
}

static void f3()
{
    cout << "in unexpected_handler" << endl;
}

static void f4()
{
    cout << "in terminate_haneler" << endl;
}

int main(void)
{
    set_unexpected(f3);
    set_terminate(f4);
    try {
	f2();
    }
    catch (E1& e) {
	e.print_debug();
    }
    catch (bad_exception) {
	cout << "caught a bad_exception" << endl;
    }
    catch (...) {
	cout << "caught a exception" << endl;
    }

    return 0;
}
