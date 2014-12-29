/*
 *
 *
 *  write by Forrest.zhang
 */

#ifndef __WINDOW_HPP__
#define __WINDOW_HPP__

#include <iostream>

using namespace std;

class Window {

public:

    // destructor
    virtual ~Window();

    virtual void set_color() = 0;

    virtual void prompt() = 0;
};

inline Window::~Window()
{
    cout << __PRETTY_FUNCTION__ << endl;
}

class Window_with_border : public virtual Window {

public:
    
    // destructor
    virtual ~Window_with_border();

    void set_color();
    void prompt();
};

inline Window_with_border::~Window_with_border()
{
    cout << __PRETTY_FUNCTION__ << endl;
}

inline void Window_with_border::set_color()
{
    cout << __PRETTY_FUNCTION__ << endl;
}

inline void Window_with_border::prompt()
{
    cout << __PRETTY_FUNCTION__ << endl;
}

class Window_with_menu : public virtual Window {

public:

    // destructor
    ~Window_with_menu();

    void prompt();
};

inline Window_with_menu::~Window_with_menu()
{
    cout << __PRETTY_FUNCTION__ << endl;
}

inline void Window_with_menu::prompt()
{
    cout << __PRETTY_FUNCTION__ << endl;
}

class My_window : public Window_with_border, public Window_with_menu {
    
public:
    
    // destructor
    ~My_window();

    // members
    void prompt();

};

inline My_window::~My_window()
{
    cout << __PRETTY_FUNCTION__ << endl;
}

inline void My_window::prompt()
{
    cout << __PRETTY_FUNCTION__ << endl;
}

#endif

