/*
 *
 *
 *
 */

class Point {
public:
  friend Point operator + (const Point& X, const Point& Y);
  friend Point* operator ++ (Point* X);
  friend Point* operator != (const Point* 
