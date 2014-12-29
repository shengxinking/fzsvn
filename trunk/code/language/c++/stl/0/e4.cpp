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
  int            x = 5;

  int sum = sum_all(&x, &x);
  cout << "sum is " << sum << endl;

  return 0;
}
