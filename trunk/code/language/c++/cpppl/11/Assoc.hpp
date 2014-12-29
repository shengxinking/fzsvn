/*
 *
 *
 *
 *
 */

#ifndef __ASSOC_HPP__
#define __ASSOC_HPP__

#include <iostream>
#include <string>
#include <vector>

using namespace std;

class Assoc {

public:
    Assoc();

    double& operator[](string&);

    friend ostream& operator << (ostream &, const Assoc&);

private:
    Assoc(const Assoc&);
    Assoc& operator=(const Assoc&);
	
    struct Pair;
    vector<Pair>  m_vec;
};

struct Assoc::Pair {
    string        name;
    double        val;

    Pair(const string& n="", double v = 0);
};

inline Assoc::Pair::Pair(const string& n, double v)
    : name(n), val(v)
{
}

inline Assoc::Assoc()
{
}

inline double& Assoc::operator [] (string& name)
{
    cout << "return double&" << endl;
    vector<Pair>::iterator p = m_vec.begin();
    for (; p != m_vec.end(); ++p)
	if (name == p->name) 
	    return p->val;

    m_vec.push_back(Pair(name, 0));
    
    return m_vec.back().val;
}


inline ostream& operator << (ostream& os, const Assoc& as)
{
    vector<Assoc::Pair>::const_iterator p;
    for (p = as.m_vec.begin(); p != as.m_vec.end(); ++p)
	os << "name: " << p->name << ", value: " << p->val << endl;

    return os;
}


#endif
