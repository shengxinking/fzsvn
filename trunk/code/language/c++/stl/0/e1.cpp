/*
 *
 *
 */

#include <iostream>

using namespace std;

template <class T>
inline T sum_all(T* first, T* last)
{
  T             sum;
  
  for (sum = 0; first != last; ++first)
    sum += *first;
  
  return (sum);
}

int main(void)
{
  {
    int           a[5] = {1, 2, 3, 4, 5};
    int           sum;
    
    sum = sum_all(a, a + 5);
    cout << "integer sum is " << sum << endl;
  }

  {
    float         a[5] = {1.1, 2.2, 3.3, 4.4, 5.5};
    float         sum;
    
    sum = sum_all(a, a + 5);
    cout << "float sum is " << sum << endl;
  }
  /*  
  {
    void          a[5] = {1.1, 2.2, 3.3, 4.4, 5.5};
    void          sum;
    
    sum = sum_all(a, a + 5);
    cout << "void sum is " << sum << endl;
  }

  {
    char*         a[5];
    char*         sum;
    
    sum = sum_all(a, a + 5);
    cout << "char* sum is " << sum << endl;
  }
  */
  return 0;
}
