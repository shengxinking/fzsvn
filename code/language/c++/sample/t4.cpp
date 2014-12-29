/*
 *
 *
 *
 */

struct X {
    const int  a;
    const int& r;

    X(int i, int& j) : a(i), r(j) {}
};

int main()
{
    int  i = 5;
    X    a(1, i);

    return 0;
}
