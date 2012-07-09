/*
 *
 *
 *
 */

class X {

    public:
    X() { y = 100;};

    private:
    int            y;
    X(const X&);
    X& operator = (const X&);
};


int main(void)
{
    X               x;

    //    X y = x;
    X z;
    //    z = x;

    return 0;
}
    
    
