/*
 *
 *
 *
 */

#ifndef _STL_UTILITY_CPP_
#define _STL_UTILITY_CPP_

#include <iosfwd>

namespace std {
    
    template <class T1, class T2> struct pair {
	typedef T1 first_type;
	typedef T2 second_type;

	pair()
	    : first(T1()), second(T2()) {}
	pair (const T1& U1, const T2& U2)
	    : first(U1), second(U2) {}
	template <class U1, class U2>
	    pair(const pair<U1, U2>& X)
		: first(X.first), second(X.second) {}
	
	T1   first;
	T2   second;
    };

    template <class T1, class T2> inline
	bool operator == (const pair<T1, T2>& X, const pair<T1, T2>& Y) 
    {
	return (X.first == Y.first && X.seond == Y.seond);
    }

    template <class T1, class T2> inline
	bool operator != (const pair<T1, T2>& X, const pair<T1, T2>& Y)
    {
	return !(X == Y);
    }
}
	
#endif
