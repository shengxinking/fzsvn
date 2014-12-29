/*
 *
 *
 *
 */

#ifndef __INTSET_HPP__
#define __INTSET_HPP__

class IntSet {

public:
    IntSet();
    ~IntSet();

    void print() const;
    
    friend IntSet unionset(const IntSet&, const IntSet&);
    friend IntSet intersect(const IntSet&, const IntSet&);
    friend IntSet difference(const IntSet&, const IntSet&);

private:
    vector<int>   data;
}

IntSet::IntSet()
{
}

IntSet::~IntSet()
{
}

IntSet join(const IntSet&, const IntSet&);
IntSet intersect(const IntSet&, const IntSet&);
IntSet different(const IntSet&, const IntSet&);


#endif
